/* ------------------------------------------------------------------
 * TRX Crc32 - Main Program File
 * ------------------------------------------------------------------ */

#include "trxcrc32.h"

/* Show program usage message */
static void show_usage ( void )
{
    fprintf ( stderr, "usage: trxcrc32 [-u] [-o offset] file\n\n"
        "  -u          optionally update checksum\n"
        "  -o offset   offset from file beginning\n"
        "  file        firmware file to be analysed\n" "\n" );
}

/* Program main function */
int main ( int argc, char *argv[] )
{
    int fd;
    int arg_off = 1;
    int readonly = TRUE;
    unsigned int crc32_calc;
    unsigned long offset = 0;
    size_t flags_off;
    size_t length;
    struct trx_header *header;
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

    header = ( struct trx_header * ) ( pmaddr + offset );

    if ( length - offset < sizeof ( struct trx_header ) || header->magic != TRX_MAGIC )
    {
        fprintf ( stderr, "Error: TRX header not found\n" );
        return 1;
    }

    /* dump trx header */
    printf ( "trx magic  : %.2x %.2x %.2x %.2x\n",
        ( ( unsigned char * ) ( &header->magic ) )[0],
        ( ( unsigned char * ) ( &header->magic ) )[1],
        ( ( unsigned char * ) ( &header->magic ) )[2],
        ( ( unsigned char * ) ( &header->magic ) )[3] );

    printf ( "trx length : %u\n", header->len );
    printf ( "trx crc32  : 0x%.8x\n", header->crc32 );
    printf ( "trx flags  : %u\n", header->flags );
    printf ( "trx ver.   : %u\n", header->version );
    printf ( "trx off #1 : %u\n", header->offsets[0] );
    printf ( "trx off #2 : %u\n", header->offsets[1] );
    printf ( "trx off #3 : %u\n", header->offsets[2] );
    /* offset 4 not applicable to trx v1 */
    if ( header->version > 1 )
    {
        printf ( "trx off #4 : %u\n", header->offsets[3] );
    }
    printf ( "\n" );

    flags_off = ( void * ) &header->flags - ( void * ) header;

    if ( flags_off > header->len )
    {
        munmap ( pmaddr, length );
        fprintf ( stderr, "Error: no data left to check with crc32\n" );
        return 1;
    }

    crc32_calc = crc32buf ( pmaddr + offset + flags_off, header->len - flags_off );
    printf ( "crc32 calc : 0x%.8x\n", crc32_calc );

    if ( header->crc32 == crc32_calc )
    {
        printf ( "crc status : correct\n" );

    } else
    {
        printf ( "crc status : incorrect\n" );
    }

    printf ( "\n" );

    /* update crc32 checksum if needed */
    if ( !readonly && header->crc32 != crc32_calc )
    {
        header->crc32 = crc32_calc;

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
