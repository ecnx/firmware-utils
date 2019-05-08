/* ------------------------------------------------------------------
 * BCM Crc32 - Main Program File
 * ------------------------------------------------------------------ */

#include "bcmcrc32.h"

/* Show program usage message */
static void show_usage ( void )
{
    fprintf ( stderr, "usage: bcmcrc32 [-u] [-o offset] file\n\n"
        "  -u          optionally update checksum\n"
        "  -o offset   offset from file beginning\n"
        "  file        firmware file to be analysed\n" "\n" );
}

#define SDUMP(N, V) \
    hdr_dump_string(N, V, sizeof(V)); \
    printf("\n");

#define SDUMP2(N, V) \
    hdr_dump_string(N, V, sizeof(V));

/* Copy firmware header string into buffer */
static void hdr_dump_string ( const char *prefix, const unsigned char *src, size_t src_size )
{
    size_t src_len;
    char dest[256];

    for ( src_len = 0; src_len < src_size; src_len++ )
    {
        if ( src[src_len] == '\0' )
        {
            break;
        }
    }

    if ( src_len >= sizeof ( dest ) )
    {
        return;
    }

    memcpy ( dest, src, src_len );
    dest[src_len] = '\0';

    printf ( "%s: %s", prefix, dest );
}

/* Copy firmware header string into buffer */
static int hdr_copy_string ( char *dest, size_t dest_size, const unsigned char *src,
    size_t src_size )
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
        return -1;
    }

    memcpy ( dest, src, src_len );
    dest[src_len] = '\0';
    return 0;
}

/* Program main function */
int main ( int argc, char *argv[] )
{
    int fd;
    int arg_off = 1;
    int needsync = FALSE;
    int readonly = TRUE;
    unsigned int data_crc32;
    unsigned int rootfs_crc32;
    unsigned int kernel_crc32;
    unsigned int header_crc32;
    unsigned int total_size = 0;
    unsigned int loader_size = 0;
    unsigned int rootfs_size = 0;
    unsigned int kernel_size = 0;
    unsigned long offset = 0;
    size_t length;
    struct bcm_header_v1 *header;
    unsigned char *pmaddr;
    char str[32];

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

    header = ( struct bcm_header_v1 * ) ( pmaddr + offset );

    if ( length - offset < sizeof ( struct bcm_header_v1 )
        || header->magic[0] != 0x36 || header->magic[1] || header->magic[2] || header->magic[3] )
    {
        munmap ( pmaddr, length );
        fprintf ( stderr, "Error: bcm header not found\n" );
        return 1;
    }

    /* parse total size */
    if ( hdr_copy_string ( str, sizeof ( str ), header->total_size,
            sizeof ( header->total_size ) ) < 0 || sscanf ( str, "%u", &total_size ) <= 0 )
    {
        munmap ( pmaddr, length );
        fprintf ( stderr, "Error: failed to parse total size\n" );
        return 1;
    }

    /* parse loader size */
    if ( hdr_copy_string ( str, sizeof ( str ), header->loader_size,
            sizeof ( header->loader_size ) ) < 0 || sscanf ( str, "%u", &loader_size ) <= 0 )
    {
        munmap ( pmaddr, length );
        fprintf ( stderr, "Error: failed to parse loader size\n" );
        return 1;
    }

    /* parse rootfs size */
    if ( hdr_copy_string ( str, sizeof ( str ), header->rootfs_size,
            sizeof ( header->rootfs_size ) ) < 0 || sscanf ( str, "%u", &rootfs_size ) <= 0 )
    {
        munmap ( pmaddr, length );
        fprintf ( stderr, "Error: failed to parse rootfs size\n" );
        return 1;
    }

    /* parse kernel size */
    if ( hdr_copy_string ( str, sizeof ( str ), header->kernel_size,
            sizeof ( header->kernel_size ) ) < 0 || sscanf ( str, "%u", &kernel_size ) <= 0 )
    {
        munmap ( pmaddr, length );
        fprintf ( stderr, "Error: failed to parse kernel size\n" );
        return 1;
    }

    if ( length - offset < 256 + loader_size + kernel_size + rootfs_size )
    {
        munmap ( pmaddr, length );
        fprintf ( stderr, "Error: no data left to check with crc32\n" );
        return 1;
    }

    /* dump bcm header */
    printf ( "bcm magic   : %.2x %.2x %.2x %.2x\n",
        ( ( unsigned char * ) ( &header->magic ) )[0],
        ( ( unsigned char * ) ( &header->magic ) )[1],
        ( ( unsigned char * ) ( &header->magic ) )[2],
        ( ( unsigned char * ) ( &header->magic ) )[3] );

    SDUMP ( "bcm vendor  ", header->vendor );
    SDUMP ( "bcm version ", header->version );
    SDUMP ( "bcm board   ", header->board_id );
    SDUMP ( "bcm chip    ", header->chip_id );
    printf ( "cpu endian  : %s ENDIAN\n", header->endian_flag[0] == 0x31 ? "BIG" : "LITTLE" );

    SDUMP2 ( "total  size ", header->total_size );
    printf ( " (%s)\n", 256 + total_size == length - offset ? "ok" : "bad" );
    SDUMP ( "loader addr ", header->loader_addr );
    SDUMP ( "loader size ", header->loader_size );
    SDUMP ( "rootfs addr ", header->rootfs_addr );
    SDUMP ( "rootfs size ", header->rootfs_size );
    SDUMP ( "kernel addr ", header->kernel_addr );
    SDUMP ( "kernel size ", header->kernel_size );

  recalc:

    /* calculate checksums */
    data_crc32 = htonl ( crc32buf ( pmaddr + offset + 256, length - offset - 256 ) );
    rootfs_crc32 = htonl ( crc32buf ( pmaddr + offset + 256 + loader_size, rootfs_size ) );
    kernel_crc32 =
        htonl ( crc32buf ( pmaddr + offset + 256 + loader_size + rootfs_size, kernel_size ) );
    header_crc32 = htonl ( crc32buf ( pmaddr + offset, 236 ) );

    printf ( "data   crc  : 0x%.8x (%s)\n", ntohl ( header->data_crc32 ),
        header->data_crc32 == data_crc32 ? "correct" : "incorrect" );
    printf ( "rootfs crc  : 0x%.8x (%s)\n", ntohl ( header->rootfs_crc32 ),
        header->rootfs_crc32 == rootfs_crc32 ? "correct" : "incorrect" );
    printf ( "kernel crc  : 0x%.8x (%s)\n", ntohl ( header->kernel_crc32 ),
        header->kernel_crc32 == kernel_crc32 ? "correct" : "incorrect" );
    printf ( "sequence    : 0x%.8x\n", header->sequence );
    printf ( "root length : 0x%.8x\n", header->root_length );
    printf ( "header crc  : 0x%.8x (%s)\n", ntohl ( header->header_crc32 ),
        header->header_crc32 == header_crc32 ? "correct" : "incorrect" );
    printf ( "\n" );

    /* update crc32 checksum if needed */
    if ( !readonly && ( header->data_crc32 != data_crc32
            || header->rootfs_crc32 != rootfs_crc32
            || header->kernel_crc32 != kernel_crc32 || header->header_crc32 != header_crc32 ) )
    {
        header->data_crc32 = data_crc32;
        header->rootfs_crc32 = rootfs_crc32;
        header->kernel_crc32 = kernel_crc32;
        header->header_crc32 = header_crc32;

        needsync = TRUE;
        goto recalc;
        printf ( "Note: checksum has been updated.\n\n" );
    }

    /*
     * The above stores may or may not be sitting in cache at
     * this point, depending on other system activity causing
     * cache pressure.  Force the change to be durable (flushed
     * all the say to the Persistent Memory) using msync().
     */
    if ( needsync && msync ( ( void * ) pmaddr, length, MS_SYNC ) < 0 )
    {
        munmap ( pmaddr, length );
        perror ( "msync" );
        return 1;
    }

    /* unmap memory */
    munmap ( pmaddr, length );

    return 0;
}
