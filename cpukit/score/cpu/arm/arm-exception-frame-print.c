/* SPDX-License-Identifier: BSD-2-Clause */

/*
 * Copyright (c) 2012-2013 embedded brains GmbH.  All rights reserved.
 *
 *  embedded brains GmbH
 *  Obere Lagerstr. 30
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

#include <inttypes.h>

#include <rtems/score/cpu.h>
#include <rtems/bspIo.h>

static void _ARM_VFP_context_print( const ARM_VFP_context *vfp_context )
{
#ifdef ARM_MULTILIB_VFP_D32
  if ( vfp_context != NULL ) {
    const uint64_t *dx = &vfp_context->register_d0;
    int i;

    printk(
      "FPEXC = 0x%08" PRIx32 "\nFPSCR = 0x%08" PRIx32 "\n",
      vfp_context->register_fpexc,
      vfp_context->register_fpscr
    );

    for ( i = 0; i < 32; ++i ) {
      uint32_t low = (uint32_t) dx[i];
      uint32_t high = (uint32_t) (dx[i] >> 32);

      printk( "D%02i = 0x%08" PRIx32 "%08" PRIx32 "\n", i, high, low );
    }
  }
#endif
}

void _CPU_Exception_frame_print( const CPU_Exception_frame *frame )
{
  printk(
    "\n"
    "R0   = 0x%08" PRIx32 " R8  = 0x%08" PRIx32 "\n"
    "R1   = 0x%08" PRIx32 " R9  = 0x%08" PRIx32 "\n"
    "R2   = 0x%08" PRIx32 " R10 = 0x%08" PRIx32 "\n"
    "R3   = 0x%08" PRIx32 " R11 = 0x%08" PRIx32 "\n"
    "R4   = 0x%08" PRIx32 " R12 = 0x%08" PRIx32 "\n"
    "R5   = 0x%08" PRIx32 " SP  = 0x%08" PRIx32 "\n"
    "R6   = 0x%08" PRIx32 " LR  = 0x%08" PRIxPTR "\n"
    "R7   = 0x%08" PRIx32 " PC  = 0x%08" PRIxPTR "\n"
#if defined(ARM_MULTILIB_ARCH_V4)
    "CPSR = 0x%08" PRIx32 " "
#elif defined(ARM_MULTILIB_ARCH_V7M)
    "XPSR = 0x%08" PRIx32 " "
#endif
    "VEC = 0x%08" PRIxPTR "\n",
    frame->register_r0,
    frame->register_r8,
    frame->register_r1,
    frame->register_r9,
    frame->register_r2,
    frame->register_r10,
    frame->register_r3,
    frame->register_r11,
    frame->register_r4,
    frame->register_r12,
    frame->register_r5,
    frame->register_sp,
    frame->register_r6,
    (intptr_t) frame->register_lr,
    frame->register_r7,
    (intptr_t) frame->register_pc,
#if defined(ARM_MULTILIB_ARCH_V4)
    frame->register_cpsr,
#elif defined(ARM_MULTILIB_ARCH_V7M)
    frame->register_xpsr,
#endif
    (intptr_t) frame->vector
  );

  _ARM_VFP_context_print( frame->vfp_context );
}
