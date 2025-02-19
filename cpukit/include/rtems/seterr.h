/**
 * @file
 *
 * @ingroup RTEMSAPISetErrno
 *
 * @brief This header file defines macros to set ``errno`` and return minus
 *   one.
 */

/*
 *  COPYRIGHT (c) 1989-2006.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#ifndef _RTEMS_SETERR_H
#define _RTEMS_SETERR_H

#include <errno.h>

/**
 * @defgroup RTEMSAPISetErrno Set Error Number Support
 *
 * @ingroup RTEMSAPI
 *
 * @{
 */

/**
 *  This is a helper macro which will set the variable errno and return
 *  the specified value to the caller.
 *
 *  @param[in] _error is the error code
 *  @param[in] _value is the value to return
 */
#define rtems_set_errno_and_return_value( _error, _value ) \
  do { errno = ( _error ); return ( _value ); } while ( 0 )

/**
 *  This is a helper macro which will set the variable errno and return
 *  -1 to the caller.  This pattern is common to many POSIX methods.
 *
 *  @param[in] _error is the error code
 */
#define rtems_set_errno_and_return_minus_one( _error ) \
  rtems_set_errno_and_return_value( _error, -1 )

/**@}*/
#endif
/* end of include file */
