/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @ingroup rtems_sparse_disk
 *
 * @brief Sparse disk block device implementation.
 */

/*
 * Copyright (c) 2012 embedded brains GmbH.  All rights reserved.
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

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <rtems.h>
#include <rtems/blkdev.h>
#include <rtems/fatal.h>

#include "rtems/sparse-disk.h"

/*
 * Allocate RAM for sparse disk
 */
static rtems_sparse_disk *sparse_disk_allocate(
  const uint32_t          media_block_size,
  const rtems_blkdev_bnum blocks_with_buffer )
{
  size_t const key_table_size = blocks_with_buffer
                                * sizeof( rtems_sparse_disk_key );
  size_t const data_size      = blocks_with_buffer * media_block_size;
  size_t const alloc_size     = sizeof( rtems_sparse_disk )
                                + key_table_size + data_size;

  rtems_sparse_disk *const sd = (rtems_sparse_disk *) malloc(
    alloc_size );

  return sd;
}

/*
 * Initialize sparse disk data
 */
static rtems_status_code sparse_disk_initialize( rtems_sparse_disk *sd,
  const uint32_t                                                    media_block_size,
  const rtems_blkdev_bnum                                           blocks_with_buffer,
  const rtems_sparse_disk_delete_handler                            sparse_disk_delete,
  const uint8_t                                                     fill_pattern )
{
  rtems_blkdev_bnum i;

  if ( NULL == sd )
    return RTEMS_INVALID_ADDRESS;

  uint8_t     *data           = (uint8_t *) sd;
  size_t const key_table_size = blocks_with_buffer
                                * sizeof( rtems_sparse_disk_key );
  size_t const data_size      = blocks_with_buffer * media_block_size;

  memset( data, 0, sizeof( rtems_sparse_disk ) + key_table_size );

  sd->fill_pattern = fill_pattern;
  memset( (uint8_t *) ( data + sizeof( rtems_sparse_disk ) + key_table_size ),
          sd->fill_pattern,
          data_size );

  sd->delete_handler = sparse_disk_delete;

  rtems_mutex_init( &sd->mutex, "Sparse Disk" );

  data                  += sizeof( rtems_sparse_disk );

  sd->blocks_with_buffer = blocks_with_buffer;
  sd->key_table          = (rtems_sparse_disk_key *) data;

  data                  += key_table_size;

  for ( i = 0; i < blocks_with_buffer; ++i, data += media_block_size ) {
    sd->key_table[i].data = data;
  }

  sd->media_block_size = media_block_size;
  return RTEMS_SUCCESSFUL;
}

/*
 * Block comparison
 */
static int sparse_disk_compare( const void *aa, const void *bb )
{
  const rtems_sparse_disk_key *a = aa;
  const rtems_sparse_disk_key *b = bb;

  if ( a->block < b->block ) {
    return -1;
  } else if ( a->block == b->block ) {
    return 0;
  } else {
    return 1;
  }
}

static rtems_sparse_disk_key *sparse_disk_find_block(
  const rtems_sparse_disk *sparse_disk,
  rtems_blkdev_bnum        block
)
{
  rtems_sparse_disk_key key = { .block = block };

  return bsearch(
    &key,
    sparse_disk->key_table,
    sparse_disk->used_count,
    sizeof( rtems_sparse_disk_key ),
    sparse_disk_compare
  );
}

static rtems_sparse_disk_key *sparse_disk_get_new_block(
  rtems_sparse_disk      *sparse_disk,
  const rtems_blkdev_bnum block
)
{
  rtems_sparse_disk_key *key;

  if ( sparse_disk->used_count >= sparse_disk->blocks_with_buffer ) {
    return NULL;
  }

  key = &sparse_disk->key_table[ sparse_disk->used_count ];
  key->block = block;
  ++sparse_disk->used_count;
  qsort(
    sparse_disk->key_table,
    sparse_disk->used_count,
    sizeof( rtems_sparse_disk_key ),
    sparse_disk_compare
  );
  return sparse_disk_find_block( sparse_disk, block );
}

static int sparse_disk_read_block(
  const rtems_sparse_disk *sparse_disk,
  const rtems_blkdev_bnum  block,
  uint8_t                 *buffer,
  const size_t             buffer_size )
{
  size_t                 bytes_to_copy = sparse_disk->media_block_size;
  rtems_sparse_disk_key *key;

  if ( buffer_size < bytes_to_copy )
    bytes_to_copy = buffer_size;

  key = sparse_disk_find_block( sparse_disk, block );

  if ( NULL != key )
    memcpy( buffer, key->data, bytes_to_copy );
  else
    memset( buffer, sparse_disk->fill_pattern, buffer_size );

  return bytes_to_copy;
}

static int sparse_disk_write_block(
  rtems_sparse_disk      *sparse_disk,
  const rtems_blkdev_bnum block,
  const uint8_t          *buffer,
  const size_t            buffer_size )
{
  size_t                 bytes_to_copy = sparse_disk->media_block_size;
  bool                   block_needs_writing = false;
  rtems_sparse_disk_key *key;
  size_t                 i;

  if ( buffer_size < bytes_to_copy )
    bytes_to_copy = buffer_size;

  /* we only need to write the block if it is different from the fill pattern.
   * If the read method does not find a block it will deliver the fill pattern anyway.
   */

  key = sparse_disk_find_block( sparse_disk, block );

  if ( NULL == key ) {
    for ( i = 0; ( !block_needs_writing ) && ( i < bytes_to_copy ); ++i ) {
      if ( buffer[i] != sparse_disk->fill_pattern )
        block_needs_writing = true;
    }

    if ( block_needs_writing ) {
      key = sparse_disk_get_new_block( sparse_disk, block );
    }
  }

  if ( NULL != key )
    memcpy( key->data, buffer, bytes_to_copy );
  else if ( block_needs_writing )
    return -1;

  return bytes_to_copy;
}

/*
 * Read/write handling
 */
static int sparse_disk_read_write(
  rtems_sparse_disk    *sparse_disk,
  rtems_blkdev_request *req,
  const bool            read )
{
  int                     rv = 0;
  uint32_t                req_buffer;
  rtems_blkdev_sg_buffer *scatter_gather;
  rtems_blkdev_bnum       block;
  uint8_t                *buff;
  size_t                  buff_size;
  unsigned int            bytes_handled;

  rtems_mutex_lock( &sparse_disk->mutex );

  for ( req_buffer = 0;
        ( 0 <= rv ) && ( req_buffer < req->bufnum );
        ++req_buffer ) {
    scatter_gather = &req->bufs[req_buffer];

    bytes_handled  = 0;
    buff           = (uint8_t *) scatter_gather->buffer;
    block          = scatter_gather->block;
    buff_size      = scatter_gather->length;

    while ( ( 0 <= rv ) && ( 0 < buff_size ) ) {
      if ( read )
        rv = sparse_disk_read_block( sparse_disk,
                                     block,
                                     &buff[bytes_handled],
                                     buff_size );
      else
        rv = sparse_disk_write_block( sparse_disk,
                                      block,
                                      &buff[bytes_handled],
                                      buff_size );

      ++block;
      bytes_handled += rv;
      buff_size     -= rv;
    }
  }

  rtems_mutex_unlock( &sparse_disk->mutex );

  if ( 0 > rv )
    rtems_blkdev_request_done( req, RTEMS_IO_ERROR );
  else
    rtems_blkdev_request_done( req, RTEMS_SUCCESSFUL );

  return 0;
}

/*
 * ioctl handler to be passed to the block device handler
 */
static int sparse_disk_ioctl( rtems_disk_device *dd, uint32_t req, void *argp )
{
  rtems_sparse_disk *sd = rtems_disk_get_driver_data( dd );

  if ( RTEMS_BLKIO_REQUEST == req ) {
    rtems_blkdev_request *r = argp;

    switch ( r->req ) {
      case RTEMS_BLKDEV_REQ_READ:
      case RTEMS_BLKDEV_REQ_WRITE:
        return sparse_disk_read_write( sd, r, r->req == RTEMS_BLKDEV_REQ_READ );
      default:
        break;
    }
  } else if ( RTEMS_BLKIO_DELETED == req ) {
    rtems_mutex_destroy( &sd->mutex );

    if ( NULL != sd->delete_handler )
      ( *sd->delete_handler )( sd );

    return 0;
  } else {
    return rtems_blkdev_ioctl( dd, req, argp );
  }

  errno = EINVAL;
  return -1;
}

void rtems_sparse_disk_free( rtems_sparse_disk *sd )
{
  free( sd );
}

rtems_status_code rtems_sparse_disk_create_and_register(
  const char       *device_file_name,
  uint32_t          media_block_size,
  rtems_blkdev_bnum blocks_with_buffer,
  rtems_blkdev_bnum media_block_count,
  uint8_t           fill_pattern )
{
  rtems_status_code  sc          = RTEMS_SUCCESSFUL;
  rtems_sparse_disk *sparse_disk = sparse_disk_allocate(
    media_block_size,
    blocks_with_buffer
  );

  if ( sparse_disk != NULL ) {
    sc = rtems_sparse_disk_register(
      device_file_name,
      sparse_disk,
      media_block_size,
      blocks_with_buffer,
      media_block_count,
      fill_pattern,
      rtems_sparse_disk_free
    );
  } else {
    sc = RTEMS_NO_MEMORY;
  }

  return sc;
}

rtems_status_code rtems_sparse_disk_register(
  const char                      *device_file_name,
  rtems_sparse_disk               *sparse_disk,
  uint32_t                         media_block_size,
  rtems_blkdev_bnum                blocks_with_buffer,
  rtems_blkdev_bnum                media_block_count,
  uint8_t                          fill_pattern,
  rtems_sparse_disk_delete_handler sparse_disk_delete )
{
  rtems_status_code sc;

  if ( blocks_with_buffer <= media_block_count ) {
    sc = sparse_disk_initialize(
      sparse_disk,
      media_block_size,
      blocks_with_buffer,
      sparse_disk_delete,
      fill_pattern
    );

    if ( RTEMS_SUCCESSFUL == sc ) {
      sc = rtems_blkdev_create(
        device_file_name,
        media_block_size,
        media_block_count,
        sparse_disk_ioctl,
        sparse_disk
      );
    }
  } else {
    sc = RTEMS_INVALID_NUMBER;
  }

  return sc;
}
