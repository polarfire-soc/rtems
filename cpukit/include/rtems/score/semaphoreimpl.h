/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @ingroup RTEMSScoreSyslockSemaphore
 *
 * @brief This header file provides the interfaces of the
 *   @ref RTEMSScoreSyslockSemaphore.
 */

/*
 * Copyright (c) 2015, 2017 embedded brains GmbH.  All rights reserved.
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

#ifndef _RTEMS_SCORE_SEMAPHOREIMPL_H
#define _RTEMS_SCORE_SEMAPHOREIMPL_H

#include <sys/lock.h>

#include <rtems/score/percpu.h>
#include <rtems/score/threadqimpl.h>

/**
 * @defgroup RTEMSScoreSyslockSemaphore System Lock Semaphore Support
 *
 * @ingroup RTEMSScore
 *
 * @brief The System Lock Semaphore Support helps to implement directives which
 *   use data structures compatible with the data structures defined by the
 *   Newlib provided <sys/lock.h> header file.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  Thread_queue_Syslock_queue Queue;
  unsigned int count;
} Sem_Control;

#define SEMAPHORE_TQ_OPERATIONS &_Thread_queue_Operations_priority

/**
 * @brief Gets the Sem_Control * of the semaphore.
 *
 * @param sem The Semaphore_Control * to cast to Sem_Control *.
 *
 * @return @a sem cast to Sem_Control *.
 */
static inline Sem_Control *_Sem_Get( struct _Semaphore_Control *_sem )
{
  return (Sem_Control *) _sem;
}

/**
 * @brief Acquires the semaphore queue critical.
 *
 * This routine acquires the semaphore.
 *
 * @param[in, out] sem The semaphore to acquire the queue of.
 * @param queue_context The thread queue context.
 *
 * @return The executing thread.
 */
static inline Thread_Control *_Sem_Queue_acquire_critical(
  Sem_Control          *sem,
  Thread_queue_Context *queue_context
)
{
  Thread_Control *executing;

  executing = _Thread_Executing;
  _Thread_queue_Queue_acquire_critical(
    &sem->Queue.Queue,
    &executing->Potpourri_stats,
    &queue_context->Lock_context.Lock_context
  );

  return executing;
}

/**
 * @brief Releases the semaphore queue.
 *
 * @param[in, out] sem The semaphore to release the queue of.
 * @param level The interrupt level value to restore the interrupt status on the processor.
 * @param queue_context The thread queue context.
 */
static inline void _Sem_Queue_release(
  Sem_Control          *sem,
  ISR_Level             level,
  Thread_queue_Context *queue_context
)
{
  _Thread_queue_Queue_release_critical(
    &sem->Queue.Queue,
    &queue_context->Lock_context.Lock_context
  );
  _ISR_Local_enable( level );
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* _RTEMS_SCORE_SEMAPHOREIMPL_H */
