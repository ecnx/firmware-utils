/* ------------------------------------------------------------------
 * Binary Header Dump - Main Program File
 * ------------------------------------------------------------------ */

#include "binhdr.h"

/* Show program usage message */
static void show_usage ( void )
{
    fprintf ( stderr, "usage: binhdr [-o offset] file\n\n"
        "  -o offset   offset from file beginning\n"
        "  file        firmware file to be analysed\n" "\n" );
}

/* Program main function */
int main ( int argc, char *argv[] )
{
    int fd;
    int arg_off = 1;
    unsigned long offset = 0;
    size_t length;
    struct bin_header *header;
    unsigned char *pmaddr;

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
    if ( ( fd = open ( argv[arg_off], O_RDONLY ) ) < 0 )
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
            mmap ( NULL, length, PROT_READ, MAP_SHARED, fd, 0 ) ) == MAP_FAILED )
    {
        close ( fd );
        perror ( "mmap" );
        return 1;
    }

    /* close file fd */
    close ( fd );

    header = ( struct bin_header * ) ( pmaddr + offset );

    if ( length - offset < sizeof ( struct bin_header ) || ntohl ( header->ID ) != 0x55324e44 )
    {
        fprintf ( stderr, "Error: bcm header not found\n" );
        return 1;
    }

    /* dump bin header */
    printf ( "hdr magic  : %c%c%c%c (%.2x %.2x %.2x %.2x)\n",
        ( ( unsigned char * ) ( &header->magic ) )[0],
        ( ( unsigned char * ) ( &header->magic ) )[1],
        ( ( unsigned char * ) ( &header->magic ) )[2],
        ( ( unsigned char * ) ( &header->magic ) )[3],
        ( ( unsigned char * ) ( &header->magic ) )[0],
        ( ( unsigned char * ) ( &header->magic ) )[1],
        ( ( unsigned char * ) ( &header->magic ) )[2],
        ( ( unsigned char * ) ( &header->magic ) )[3] );
    printf ( "hdr res1   : 0x%.8x\n", header->res1 );
    printf ( "hdr fwdate : %.2u.%.2u.%.2u\n", header->fwdate[2], header->fwdate[1],
        2000 + header->fwdate[0] );
    printf ( "hdr fwvern : %.2u.%.2u.%.2u\n", header->fwvern[0], header->fwvern[1],
        header->fwvern[2] );
    printf ( "hdr ID     : %c%c%c%c (%.2x %.2x %.2x %.2x)\n",
        ( ( unsigned char * ) ( &header->ID ) )[0], ( ( unsigned char * ) ( &header->ID ) )[1],
        ( ( unsigned char * ) ( &header->ID ) )[2], ( ( unsigned char * ) ( &header->ID ) )[3],
        ( ( unsigned char * ) ( &header->ID ) )[0], ( ( unsigned char * ) ( &header->ID ) )[1],
        ( ( unsigned char * ) ( &header->ID ) )[2], ( ( unsigned char * ) ( &header->ID ) )[3] );
    printf ( "hdr hw_ver : 0x%.2x\n", header->hw_ver );
    printf ( "hdr s/n    : 0x%.2x\n", header->sn );
    printf ( "hdr flags  : 0x%.4x\n", header->flags );
    printf ( "hdr stable : 0x%.4x\n", header->stable );
    printf ( "hdr try1   : 0x%.4x\n", header->try1 );
    printf ( "hdr try2   : 0x%.4x\n", header->try2 );
    printf ( "hdr try3   : 0x%.4x\n", header->try3 );
    printf ( "hdr res3   : 0x%.4x\n", header->res3 );

    /* unmap memory */
    munmap ( pmaddr, length );

    return 0;
}
