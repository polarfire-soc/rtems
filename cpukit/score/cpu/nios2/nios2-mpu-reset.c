/* SPDX-License-Identifier: BSD-2-Clause */

/*
 * Copyright (c) 2011 embedded brains GmbH.  All rights reserved.
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

#include <rtems/score/nios2-utility.h>

void _Nios2_MPU_Reset( const Nios2_MPU_Configuration *config )
{
  uint32_t data_mpubase = (1U << config->data_region_size_log2)
    | NIOS2_MPUBASE_D;
  uint32_t inst_mpubase = 1U << config->instruction_region_size_log2;
  uint32_t mpuacc = NIOS2_MPUACC_WR;
  int data_count = config->data_region_count;
  int inst_count = config->instruction_region_count;
  int i = 0;

  _Nios2_MPU_Disable();

  for ( i = 0; i < data_count; ++i ) {
    uint32_t index = ((uint32_t) i) << NIOS2_MPUBASE_INDEX_OFFSET;

    _Nios2_Set_ctlreg_mpubase( data_mpubase | index );
    _Nios2_Set_ctlreg_mpuacc( mpuacc );
  }

  for ( i = 0; i < inst_count; ++i ) {
    uint32_t index = ((uint32_t) i) << NIOS2_MPUBASE_INDEX_OFFSET;

    _Nios2_Set_ctlreg_mpubase( inst_mpubase | index );
    _Nios2_Set_ctlreg_mpuacc( mpuacc );
  }
}
