/*
 *  COPYRIGHT (c) 2021.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems.h>
#include <tmacros.h>

#include <rtems/score/percpu.h>

const char rtems_test_name[] = "SPSTKALLOC 4";

static int thread_stacks_count = 0;

static rtems_task Init(
  rtems_task_argument ignored
)
{
  rtems_print_printer_fprintf_putc(&rtems_test_printer);
  TEST_BEGIN();
  rtems_test_assert(thread_stacks_count == 1);
  TEST_END();
  rtems_test_exit( 0 );
}

static uint8_t stack_memory[RTEMS_MINIMUM_STACK_SIZE * 4];

static int stack_offset_next;

static void *allocate_helper(size_t size)
{
  size_t  next;
  void   *alloc;

  next = stack_offset_next + size; 
  rtems_test_assert( next < sizeof(stack_memory) );

  alloc = &stack_memory[stack_offset_next];
  stack_offset_next = next;
  return alloc;
}

static void *thread_stacks_allocate_for_idle(
  uint32_t  cpu,
  size_t    stack_size
)
{
  rtems_test_assert(thread_stacks_count == 0);
  thread_stacks_count++;
  return allocate_helper(stack_size);
}

/*
 * Configure the IDLE thread stack allocators. This is a special
 * case where there is an IDLE thread stack allocator but no custom
 * allocator set for other threads.
 */
#define CONFIGURE_TASK_STACK_ALLOCATOR_FOR_IDLE thread_stacks_allocate_for_idle


/* NOTICE: the clock driver is explicitly disabled */
#define CONFIGURE_APPLICATION_DOES_NOT_NEED_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_SIMPLE_CONSOLE_DRIVER

#define CONFIGURE_MAXIMUM_TASKS            1

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_FLOATING_POINT

#define CONFIGURE_INITIAL_EXTENSIONS RTEMS_TEST_INITIAL_EXTENSION

#define CONFIGURE_INIT
#include <rtems/confdefs.h>
