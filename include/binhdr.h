/* ------------------------------------------------------------------
 * Binary Header Dump - Shared Project Header
 * ------------------------------------------------------------------ */

#include <arpa/inet.h>
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

#ifndef BINHDR_H
#define BINHDR_H

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/* structure based on openwrt docs */

/*
  0                   1                   2                   3   
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 
 +---------------------------------------------------------------+
 |                            magic                              |
 +---------------------------------------------------------------+
 |                            res1                               |
 +---------------------------------------------------------------+
 |                fwdate                         |   fwvern...   |
 +---------------------------------------------------------------+
 |    ...fwvern                  |              ID...            |
 +---------------------------------------------------------------+
 |         ...ID                 |  hw_ver       |    s/n        |
 +---------------------------------------------------------------+
 |           flags               |           stable              |
 +---------------------------------------------------------------+
 |           try1                |           try2                |
 +---------------------------------------------------------------+
 |           try3                |           res3                |
 +---------------------------------------------------------------+
*/

#pragma pack(push, 1)
struct bin_header
{
    uint32_t magic;             /* magic */
    uint32_t res1;              /* reserved1 */
    uint8_t fwdate[3];          /* date: year:8, month:8, day:8 */
    uint8_t fwvern[3];          /* version informations a.b.c */
    uint32_t ID;                /* constant "U2ND" */
    uint8_t hw_ver;             /* depends on board */
    uint8_t sn;                 /* depends on board */
    uint16_t flags;             /* flags */
    uint16_t stable;            /* marks the firmware stable */
    uint16_t try1;              /* managed by bootloader */
    uint16_t try2;              /* managed by bootloader */
    uint16_t try3;              /* managed by bootloader */
    uint16_t res3;              /* reserved3 */
};
#pragma pack(pop)

#endif
