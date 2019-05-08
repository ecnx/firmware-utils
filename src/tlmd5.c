/* ------------------------------------------------------------------
 * Utility for TP-Link Firmware - Main Program File
 * ------------------------------------------------------------------ */

#include "tlmd5.h"

/* Show program usage message */
static void show_usage ( void )
{
    fprintf ( stderr, "usage: tlmd5 [-u] [-o offset] file\n\n"
        "  -u          optionally update checksum\n"
        "  -o offset   offset from file beginning\n"
        "  file        firmware file to be analysed\n" "\n" );
}

/* Copy firmware header string into buffer */
static void hdr_copy_string ( char *dest, size_t dest_size, const char *src, size_t src_size )
{
    size_t src_len;

    for ( src_len = 0; src_len < src_size; src_len++ )
    {
        if ( src[src_len] == '\0' )
        {
            break;
        }
    }

    if ( dest_size <= src_len )
    {
        return;
    }

    memcpy ( dest, src, src_len );
    dest[src_len] = '\0';
}

/* Print hex data */
static void hex_dump ( const char *prefix, const unsigned char *src, size_t len )
{
    size_t i;

    printf ( "%s", prefix );

    for ( i = 0; i < len; i++ )
    {
        printf ( "%.2x", src[i] );

        if ( i + 1 < len )
        {
            putchar ( '\x20' );
        }
    }

    putchar ( '\n' );
}

/* Process TP-Link firmware header V1 */
static void process_header_v1 ( unsigned char *base, size_t length, int readonly, int *needsync )
{
    int md5sum1_status;
    struct fw_header_v1 *header = ( struct fw_header_v1 * ) base;
    struct fw_header_v1 test;
    MD5_CTX ctx;
    char buffer[256];
    unsigned char md5_calc[MD5SUM_LEN];
    const char md5salt_normal[MD5SUM_LEN] = {
        0xdc, 0xd7, 0x3a, 0xa5, 0xc3, 0x95, 0x98, 0xfb,
        0xdd, 0xf9, 0xe7, 0xf4, 0x0e, 0xae, 0x47, 0x38,
    };
    const char md5salt_boot[MD5SUM_LEN] = {
        0x8c, 0xef, 0x33, 0x5b, 0xd5, 0xc5, 0xce, 0xfa,
        0xa7, 0x9c, 0x28, 0xda, 0xb2, 0xe9, 0x0f, 0x42,
    };

    /* dump trx header */
    hdr_copy_string ( buffer, sizeof ( buffer ), header->vendor_name,
        sizeof ( header->vendor_name ) );
    printf ( "vendor name   : %s\n", buffer );
    hdr_copy_string ( buffer, sizeof ( buffer ), header->fw_version,
        sizeof ( header->fw_version ) );
    printf ( "version       : %s\n", buffer );
    printf ( "hardware id   : 0x%.8x\n", ntohl ( header->hw_id ) );
    printf ( "hardware rev  : %u\n", ntohl ( header->hw_rev ) );
    printf ( "region        : %u\n", ntohl ( header->region ) );
    printf ( "reserved 2    : %u\n", ntohl ( header->unk2 ) );
    printf ( "reserved 3    : %u\n", ntohl ( header->unk3 ) );
    printf ( "load address  : 0x%.8x\n", ntohl ( header->kernel_la ) );
    printf ( "entry point   : 0x%.8x\n", ntohl ( header->kernel_ep ) );
    printf ( "total length  : %u (%s)\n", ntohl ( header->fw_length ),
        ntohl ( header->fw_length ) == length ? "ok" : "bad" );
    printf ( "kernel offset : %u\n", ntohl ( header->kernel_ofs ) );
    printf ( "kernel length : %u\n", ntohl ( header->kernel_len ) );
    printf ( "rootfs offset : %u\n", ntohl ( header->rootfs_ofs ) );
    printf ( "rootfs length : %u\n", ntohl ( header->rootfs_len ) );
    printf ( "boot offset   : %u\n", ntohl ( header->boot_ofs ) );
    printf ( "boot length   : %u\n", ntohl ( header->boot_len ) );
    printf ( "firmware ver. : %u.%u.%u\n", ntohs ( header->ver_hi ), ntohs ( header->ver_mid ),
        ntohs ( header->ver_lo ) );

    /* recaluclate and check md5 1 checksum */
    memcpy ( &test, header, sizeof ( struct fw_header_v1 ) );
    if ( !header->boot_len )
    {
        memcpy ( test.md5sum1, md5salt_normal, sizeof ( test.md5sum1 ) );
    } else
    {
        memcpy ( test.md5sum1, md5salt_boot, sizeof ( test.md5sum1 ) );
    }
    MD5_Init ( &ctx );
    MD5_Update ( &ctx, &test, sizeof ( test ) );
    MD5_Update ( &ctx, base + sizeof ( test ), length - sizeof ( test ) );
    MD5_Final ( md5_calc, &ctx );
    md5sum1_status = !memcmp ( header->md5sum1, md5_calc, MD5SUM_LEN );

    /* dump md5 checksums */
    hex_dump ( "md5 1 sum     : ", header->md5sum1, sizeof ( header->md5sum1 ) );
    hex_dump ( "md5 2 sum     : ", header->md5sum2, sizeof ( header->md5sum2 ) );

    if ( md5sum1_status )
    {
        printf ( "md5 1 status  : correct\n" );

    } else
    {
        printf ( "md5 1 status  : incorrect\n" );
    }

    printf ( "\n" );

    /* update crc32 checksum if needed */
    if ( !readonly && !md5sum1_status )
    {
        memcpy ( header->md5sum1, md5_calc, MD5SUM_LEN );
        *needsync = TRUE;
    } else
    {
        *needsync = FALSE;
    }
}

/* Process TP-Link firmware header V2 */
static void process_header_v2 ( unsigned char *base, size_t length, int readonly, int *needsync )
{
    int md5sum1_status;
    struct fw_header_v2 *header = ( struct fw_header_v2 * ) base;
    struct fw_header_v2 test;
    MD5_CTX ctx;
    char buffer[256];
    unsigned char md5_calc[MD5SUM_LEN];
    const char md5salt_normal[MD5SUM_LEN] = {
        0xdc, 0xd7, 0x3a, 0xa5, 0xc3, 0x95, 0x98, 0xfb,
        0xdc, 0xf9, 0xe7, 0xf4, 0x0e, 0xae, 0x47, 0x37,
    };
    const char md5salt_boot[MD5SUM_LEN] = {
        0x8c, 0xef, 0x33, 0x5b, 0xd5, 0xc5, 0xce, 0xfa,
        0xa7, 0x9c, 0x28, 0xda, 0xb2, 0xe9, 0x0f, 0x42,
    };

    /* dump trx header */
    hdr_copy_string ( buffer, sizeof ( buffer ), header->fw_version,
        sizeof ( header->fw_version ) );
    printf ( "version       : %s\n", buffer );
    printf ( "hardware id   : 0x%.8x\n", ntohl ( header->hw_id ) );
    printf ( "hardware rev  : %u\n", ntohl ( header->hw_rev ) );
    printf ( "reserved 1    : %u\n", ntohl ( header->unk1 ) );
    printf ( "reserved 2    : %u\n", ntohl ( header->unk2 ) );
    printf ( "reserved 3    : %u\n", ntohl ( header->unk3 ) );
    printf ( "load address  : 0x%.8x\n", ntohl ( header->kernel_la ) );
    printf ( "entry point   : 0x%.8x\n", ntohl ( header->kernel_ep ) );
    printf ( "total length  : %u (%s)\n", ntohl ( header->fw_length ),
        ntohl ( header->fw_length ) == length ? "ok" : "bad" );
    printf ( "kernel offset : %u\n", ntohl ( header->kernel_ofs ) );
    printf ( "kernel length : %u\n", ntohl ( header->kernel_len ) );
    printf ( "rootfs offset : %u\n", ntohl ( header->rootfs_ofs ) );
    printf ( "rootfs length : %u\n", ntohl ( header->rootfs_len ) );
    printf ( "boot offset   : %u\n", ntohl ( header->boot_ofs ) );
    printf ( "boot length   : %u\n", ntohl ( header->boot_len ) );
    printf ( "reserved 4    : %u\n", ntohs ( header->unk4 ) );
    printf ( "s version     : %u.%u\n", ntohs ( header->sver_hi ), ntohs ( header->sver_lo ) );
    printf ( "reserved 5    : %u\n", header->unk5 );
    printf ( "firmware ver. : %u.%u.%u\n", ntohs ( header->ver_hi ), ntohs ( header->ver_mid ),
        ntohs ( header->ver_lo ) );

    /* recaluclate and check md5 1 checksum */
    memcpy ( &test, header, sizeof ( struct fw_header_v2 ) );
    if ( !header->boot_len )
    {
        memcpy ( test.md5sum1, md5salt_normal, sizeof ( test.md5sum1 ) );
    } else
    {
        memcpy ( test.md5sum1, md5salt_boot, sizeof ( test.md5sum1 ) );
    }
    MD5_Init ( &ctx );
    MD5_Update ( &ctx, &test, sizeof ( test ) );
    MD5_Update ( &ctx, base + sizeof ( test ), length - sizeof ( test ) );
    MD5_Final ( md5_calc, &ctx );
    md5sum1_status = !memcmp ( header->md5sum1, md5_calc, MD5SUM_LEN );

    /* dump md5 checksums */
    hex_dump ( "md5 1 sum     : ", header->md5sum1, sizeof ( header->md5sum1 ) );
    hex_dump ( "md5 2 sum     : ", header->md5sum2, sizeof ( header->md5sum2 ) );

    if ( md5sum1_status )
    {
        printf ( "md5 1 status  : correct\n" );

    } else
    {
        printf ( "md5 1 status  : incorrect\n" );
    }

    printf ( "\n" );

    /* update crc32 checksum if needed */
    if ( !readonly && !md5sum1_status )
    {
        memcpy ( header->md5sum1, md5_calc, MD5SUM_LEN );
        *needsync = TRUE;
    } else
    {
        *needsync = FALSE;
    }
}

/* Process TP-Link firmware header */
static int process_header ( unsigned char *base, size_t length, int readonly, int *needsync )
{
    unsigned int version;
    uint32_t *header_ver = ( uint32_t * ) base;

    /* at least 4 bytes long */
    if ( length < sizeof ( uint32_t ) )
    {
        return -1;
    }

    /* load header version from file */
    if ( *header_ver == HEADER_VERSION_V1 || ntohl ( *header_ver ) == HEADER_VERSION_V1 )
    {
        version = 1;

    } else if ( *header_ver == HEADER_VERSION_V2 || ntohl ( *header_ver ) == HEADER_VERSION_V2 )
    {
        version = 2;

    } else
    {
        /* unknown firmware header */
        return -1;
    }

    /* print header version if possible */
    printf ( "fw header ver : V%u\n", version );

    /* branch according to header version */
    if ( *header_ver == HEADER_VERSION_V1 || ntohl ( *header_ver ) == HEADER_VERSION_V1 )
    {
        /* firmware header V1 */
        if ( length <= sizeof ( struct fw_header_v1 ) )
        {
            return -1;
        }

        process_header_v1 ( base, length, readonly, needsync );

    } else if ( *header_ver == HEADER_VERSION_V2 || ntohl ( *header_ver ) == HEADER_VERSION_V2 )
    {
        /* firmware header V2 */
        if ( length <= sizeof ( struct fw_header_v2 ) )
        {
            return -1;
        }

        process_header_v2 ( base, length, readonly, needsync );
    }

    return 0;
}

/* Program main function */
int main ( int argc, char *argv[] )
{
    int fd;
    int arg_off = 1;
    int needsync = FALSE;
    int readonly = TRUE;
    unsigned long offset = 0;
    size_t length;
    unsigned char *pmaddr;

    /* validate arguments count */
    if ( arg_off >= argc )
    {
        show_usage (  );
        return 1;
    }

    /* enable update mode if needed */
    if ( !strcmp ( argv[arg_off], "-u" ) )
    {
        readonly = FALSE;
        arg_off++;
    }

    /* validate arguments count */
    if ( arg_off >= argc )
    {
        show_usage (  );
        return 1;
    }

    /* parse file offset if needed */
    if ( !strcmp ( argv[arg_off], "-o" ) )
    {
        if ( sscanf ( argv[arg_off + 1], "%lu", &offset ) <= 0 )
        {
            show_usage (  );
            return 1;
        }

        arg_off += 2;
    }

    /* validate arguments count */
    if ( arg_off >= argc )
    {
        show_usage (  );
        return 1;
    }

    /* open file for mapping */
    if ( ( fd = open ( argv[arg_off], readonly ? O_RDONLY : O_RDWR ) ) < 0 )
    {
        perror ( "open" );
        return 1;
    }

    /* obtain file size */
    if ( ( off_t ) ( length = lseek ( fd, 0, SEEK_END ) ) < 0 )
    {
        close ( fd );
        perror ( "lseek" );
        return 1;
    }

    /* restore position in file */
    if ( lseek ( fd, 0, SEEK_SET ) < 0 )
    {
        close ( fd );
        perror ( "lseek" );
        return 1;
    }

    /* validate offset parameter */
    if ( offset >= length )
    {
        close ( fd );
        fprintf ( stderr, "Error: file offset is out of range\n" );
        return 1;
    }

    /*
     * Map data from the file into our memory for read & write.
     * Use MAP_SHARED for Persistent Memory so that stores go
     * directly to the PM and are globally visible.
     */
    if ( ( pmaddr = ( unsigned char * )
            mmap ( NULL, length,
                PROT_READ | ( readonly ? 0 : PROT_WRITE ), MAP_SHARED, fd, 0 ) ) == MAP_FAILED )
    {
        close ( fd );
        perror ( "mmap" );
        return 1;
    }

    /* close file fd */
    close ( fd );

    if ( length - offset < sizeof ( uint32_t ) )
    {
        munmap ( pmaddr, length );
        fprintf ( stderr, "Error: TP-Link header not found\n" );
        return 1;
    }

    /* dump header, update checksum if ndeeded */
    if ( process_header ( pmaddr + offset, length - offset, readonly, &needsync ) < 0 )
    {
        fprintf ( stderr, "Error: TP-Link header not found\n" );
        return 1;
    }

    if ( !readonly && needsync )
    {
        /*
         * The above stores may or may not be sitting in cache at
         * this point, depending on other system activity causing
         * cache pressure.  Force the change to be durable (flushed
         * all the say to the Persistent Memory) using msync().
         */
        if ( msync ( ( void * ) pmaddr, length, MS_SYNC ) < 0 )
        {
            munmap ( pmaddr, length );
            perror ( "msync" );
            return 1;
        }

        printf ( "Note: checksum has been updated.\n\n" );
    }

    /* unmap memory */
    munmap ( pmaddr, length );

    return 0;
}
