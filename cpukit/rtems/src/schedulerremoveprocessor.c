/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @ingroup RTEMSImplClassicScheduler
 *
 * @brief This source file contains the implementation of
 *   rtems_scheduler_remove_processor().
 */

/*
 * Copyright (c) 2016 embedded brains GmbH.  All rights reserved.
 *
 *  embedded brains GmbH
 *  Dornierstr. 4
 *  82178 Puchheim
 *  Germany
 *  <rtems@embedded-brains.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/rtems/scheduler.h>
#include <rtems/score/schedulerimpl.h>
#include <rtems/config.h>

#if defined(RTEMS_SMP)
typedef struct {
  const Scheduler_Control *scheduler;
  rtems_status_code        status;
} Scheduler_Processor_removal_context;

static bool _Scheduler_Check_processor_not_required(
  Thread_Control *the_thread,
  void           *arg
)
{
  Scheduler_Processor_removal_context *iter_context;
  Thread_queue_Context                 queue_context;
  ISR_lock_Context                     state_context;

  if ( the_thread->is_idle ) {
    return false;
  }

  iter_context = arg;

  _Thread_queue_Context_initialize( &queue_context );
  _Thread_Wait_acquire( the_thread, &queue_context );
  _Thread_State_acquire_critical( the_thread, &state_context );

  if (
    _Thread_Scheduler_get_home( the_thread ) == iter_context->scheduler
      && !_Processor_mask_Has_overlap(
        &the_thread->Scheduler.Affinity,
        _Scheduler_Get_processors( iter_context->scheduler )
      )
  ) {
    iter_context->status = RTEMS_RESOURCE_IN_USE;
  }

  _Thread_State_release_critical( the_thread, &state_context );
  _Thread_Wait_release( the_thread, &queue_context );
  return iter_context->status != RTEMS_SUCCESSFUL;
}

static bool _Scheduler_Check_no_helping(
  Thread_Control *the_thread,
  void           *arg
)
{
  Scheduler_Processor_removal_context *iter_context;
  ISR_lock_Context                     lock_context;
  const Chain_Node                    *node;
  const Chain_Node                    *tail;

  if ( the_thread->is_idle ) {
    return false;
  }

  iter_context = arg;

  _Thread_State_acquire( the_thread, &lock_context );
  node = _Chain_Immutable_first( &the_thread->Scheduler.Scheduler_nodes );
  tail = _Chain_Immutable_tail( &the_thread->Scheduler.Scheduler_nodes );

  do {
    const Scheduler_Node    *scheduler_node;
    const Scheduler_Control *scheduler;

    scheduler_node = SCHEDULER_NODE_OF_THREAD_SCHEDULER_NODE( node );
    scheduler = _Scheduler_Node_get_scheduler( scheduler_node );

    if ( scheduler == iter_context->scheduler ) {
      iter_context->status = RTEMS_RESOURCE_IN_USE;
      break;
    }

    node = _Chain_Immutable_next( node );
  } while ( node != tail );

  _Thread_State_release( the_thread, &lock_context );
  return iter_context->status != RTEMS_SUCCESSFUL;
}
#endif

rtems_status_code rtems_scheduler_remove_processor(
  rtems_id scheduler_id,
  uint32_t cpu_index
)
{
  const Scheduler_Control             *scheduler;
#if defined(RTEMS_SMP)
  Scheduler_Processor_removal_context  iter_context;
  ISR_lock_Context                     lock_context;
  Scheduler_Context                   *scheduler_context;
  Per_CPU_Control                     *cpu;
  Per_CPU_Control                     *cpu_self;
#endif

  scheduler = _Scheduler_Get_by_id( scheduler_id );
  if ( scheduler == NULL ) {
    return RTEMS_INVALID_ID;
  }

  if ( cpu_index >= rtems_configuration_get_maximum_processors() ) {
    return RTEMS_INVALID_NUMBER;
  }

#if defined(RTEMS_SMP)
  iter_context.scheduler = scheduler;
  iter_context.status = RTEMS_SUCCESSFUL;
  scheduler_context = _Scheduler_Get_context( scheduler );
  cpu = _Per_CPU_Get_by_index( cpu_index );

  _Objects_Allocator_lock();

  if ( cpu->Scheduler.control != scheduler ) {
    _Objects_Allocator_unlock();
    return RTEMS_INVALID_NUMBER;
  }

  /*
   * This prevents the selection of this scheduler instance by new threads in
   * case the processor count changes to zero.
   */
  _ISR_lock_ISR_disable( &lock_context );
  _Scheduler_Acquire_critical( scheduler, &lock_context );
  _Processor_mask_Clear( &scheduler_context->Processors, cpu_index );
  _Scheduler_Release_critical( scheduler, &lock_context );
  _ISR_lock_ISR_enable( &lock_context );

  _Thread_Iterate( _Scheduler_Check_processor_not_required, &iter_context );

  if (
    _Processor_mask_Is_zero( &scheduler_context->Processors ) &&
    iter_context.status == RTEMS_SUCCESSFUL
  ) {
    _Thread_Iterate( _Scheduler_Check_no_helping, &iter_context );
  }

  _ISR_lock_ISR_disable( &lock_context );
  _Scheduler_Acquire_critical( scheduler, &lock_context );

  if ( iter_context.status == RTEMS_SUCCESSFUL ) {
    Thread_Control *idle;
    Scheduler_Node *scheduler_node;

    cpu->Scheduler.control = NULL;
    cpu->Scheduler.context = NULL;
    idle = ( *scheduler->Operations.remove_processor )( scheduler, cpu );
    cpu->Scheduler.idle_if_online_and_unused = idle;

    scheduler_node = _Thread_Scheduler_get_home_node( idle );
    _Priority_Plain_extract(
      &scheduler_node->Wait.Priority,
      &idle->Real_priority
    );
    _Assert( _Priority_Is_empty( &scheduler_node->Wait.Priority ) );
    _Chain_Extract_unprotected( &scheduler_node->Thread.Wait_node );
    _Assert( _Chain_Is_empty( &idle->Scheduler.Wait_nodes ) );
    _Chain_Extract_unprotected( &scheduler_node->Thread.Scheduler_node.Chain );
    _Assert( _Chain_Is_empty( &idle->Scheduler.Scheduler_nodes ) );
  } else {
    _Processor_mask_Set( &scheduler_context->Processors, cpu_index );
  }

  cpu_self = _Thread_Dispatch_disable_critical( &lock_context );
  _Scheduler_Release_critical( scheduler, &lock_context );
  _ISR_lock_ISR_enable( &lock_context );
  _Thread_Dispatch_direct( cpu_self );
  _Objects_Allocator_unlock();
  return iter_context.status;
#else
  return RTEMS_RESOURCE_IN_USE;
#endif
}
