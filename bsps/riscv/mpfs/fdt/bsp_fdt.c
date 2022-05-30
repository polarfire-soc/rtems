/*
 *  COPYRIGHT (c) 2021.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#include <bsp.h>
#include <bsp/fdt.h>

#include BSP_MPFS_DTB_HEADER_PATH

const void *bsp_fdt_get(void)
{
  return mpfs_dtb;
}
