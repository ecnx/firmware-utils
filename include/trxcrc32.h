/* ------------------------------------------------------------------
 * TRX Crc32 - Shared Project Header
 * ------------------------------------------------------------------ */

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "crc32.h"

#ifndef TRXCRC32_H
#define TRXCRC32_H

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define TRX_MAGIC	0x30524448      /* "HDR0" */

/* structure based on openwrt docs */

/*
  0                   1                   2                   3   
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
 +---------------------------------------------------------------+
 |                     magic number ('HDR0')                     |
 +---------------------------------------------------------------+
 |                  length (header size + data)                  |
 +---------------+---------------+-------------------------------+
 |                       32-bit CRC value                        |
 +---------------+---------------+-------------------------------+
 |           TRX flags           |          TRX version          |
 +-------------------------------+-------------------------------+
 |                      Partition offset[0]                      |
 +---------------------------------------------------------------+
 |                      Partition offset[1]                      |
 +---------------------------------------------------------------+
 |                      Partition offset[2]                      |
 +---------------------------------------------------------------+
*/

struct trx_header
{
    uint32_t magic;             /* "HDR0" */
    uint32_t len;               /* Length of file including header */
    uint32_t crc32;             /* 32-bit CRC from flag_version to end of file */
    uint16_t flags;             /* 16-bit flags */
    uint16_t version;           /* 16-bit version */
    uint32_t offsets[4];        /* Offsets of partitions from start of header */
};

#endif
