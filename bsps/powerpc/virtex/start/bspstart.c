/*  bsp_start()
 *
 *  This routine starts the application.  It includes application,
 *  board, and monitor specific initialization and configuration.
 *  The generic CPU dependent initialization has been performed
 *  before this routine is invoked.
 *
 *  INPUT:  NONE
 *
 *  OUTPUT: NONE
 *
 *  Author:	Thomas Doerfler <td@imd.m.isar.de>
 *              IMD Ingenieurbuero fuer Microcomputertechnik
 *
 *  COPYRIGHT (c) 1998 by IMD
 *
 *  Changes from IMD are covered by the original distributions terms.
 *  This file has been derived from the papyrus BSP:
 *
 *  Author:	Andrew Bray <andy@i-cubed.co.uk>
 *
 *  COPYRIGHT (c) 1995 by i-cubed ltd.
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *      for any purpose is hereby granted without fee, provided that
 *      the above copyright notice and this notice appears in all
 *      copies, and that the name of i-cubed limited not be used in
 *      advertising or publicity pertaining to distribution of the
 *      software without specific, written prior permission.
 *      i-cubed limited makes no representations about the suitability
 *      of this software for any purpose.
 *
 *  Modifications for spooling console driver and control of memory layout
 *  with linker command file by
 *              Thomas Doerfler <td@imd.m.isar.de>
 *  for these modifications:
 *  COPYRIGHT (c) 1997 by IMD, Puchheim, Germany.
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *      for any purpose is hereby granted without fee, provided that
 *      the above copyright notice and this notice appears in all
 *      copies. IMD makes no representations about the suitability
 *      of this software for any purpose.
 *
 *  Derived from c/src/lib/libbsp/no_cpu/no_bsp/startup/bspstart.c:
 *
 *  COPYRIGHT (c) 1989, 1990, 1991, 1992, 1993, 1994.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  Modifications for PPC405GP by Dennis Ehlin
 */

#include <rtems/counter.h>

#include <bsp.h>
#include <bsp/irq.h>
#include <bsp/irq-generic.h>
#include <bsp/bootcard.h>
#include <bsp/linker-symbols.h>

#include <libcpu/powerpc-utility.h>

#include RTEMS_XPARAMETERS_H

/* Symbols defined in linker command file */
LINKER_SYMBOL(virtex_exc_vector_base);

/*
 *  Driver configuration parameters
 */
uint32_t bsp_time_base_frequency = XPAR_CPU_PPC405_CORE_CLOCK_FREQ_HZ;

uint32_t _CPU_Counter_frequency(void)
{
  return bsp_time_base_frequency;
}

/*
 *  bsp_start
 *
 *  This routine does the bulk of the system initialization.
 */
void bsp_start( void )
{
  /*
   * Get CPU identification dynamically. Note that the get_ppc_cpu_type()
   * function store the result in global variables
   * so that it can be used latter...
   */
  get_ppc_cpu_type();
  get_ppc_cpu_revision();

  ppc_exc_initialize_with_vector_base(
    (uintptr_t) _ISR_Stack_area_begin,
    virtex_exc_vector_base
  );
  __asm__ volatile (
    ".machine \"push\"\n"
    ".machine \"any\"\n"
    "mtevpr %0\n"
    ".machine \"pop\"" : :
    "r" (virtex_exc_vector_base)
  );

  bsp_interrupt_initialize();
}
