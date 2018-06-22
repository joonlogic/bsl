#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>
#include <pcap.h>

typedef unsigned long u64;

#ifndef TRUE
  #define TRUE                   1
  #define FALSE                  0
#endif

#ifndef boolean
  #define boolean int
#endif


#define DELIM_PATTERN_DMA               (u64)0xA5A5A5A5A5
#define BIT_PADDED                      23
#define SIZEOF_OVERHEAD                 16
#define SIZE_DMA_ALIGN                  128
#define IS_PADDED( TRACEP )  \
	    ( *(u64*)(TRACEP) & ( 1 << BIT_PADDED ) ) ? TRUE : FALSE

#define GET_LENGTH_FIELD( TRACEP )       *(u64*)(TRACEP) & 0xFFFF;
#define GET_PADDED_LENGTH( UNITLEN )     \
	    ( ( ( ( UNITLEN - 1 ) / SIZE_DMA_ALIGN ) + 1 ) * SIZE_DMA_ALIGN )
#define GET_NO_PADDED_LENGTH( UNITLEN )  ( ( UNITLEN ) + SIZE_DMA_UP_HEADER )


static const char* VERSION = "20111217";

static void printUsage( void )
{
	fprintf(stderr, "\n");
    fprintf(stderr, "mqbps version: %s", VERSION);
	fprintf(stderr, "\n");
    fprintf(stderr, "Usage: mqbps [args] \n" );
	fprintf(stderr, "\t-f <filename>\t\tinput bps display file\n"
					"\n");
	exit(0);
}

static u64 doCalcBps( \
		FILE* mqfp, \
		char* filename, \
		int partlength, \
		int partcount );

int main( int argc, char* argv[])
{
	int sel = 0;
	int partlength = 0;
	int partcount = 0;
	u64 totalcount = 0;
	char filename[128] = {0,};
	char ofilename[128] = {0,};
	void* tracep = NULL;
	FILE* mqfp = NULL;

	while( ( sel = getopt( argc, argv, "f:l:c:h" ) ) != -1 )
	{
		switch( sel )
		{
			case 'f' : // filename
				strcpy( filename, optarg );
				break;
			case 'l' : // zzogari size
				partlength = atoi( optarg );
				partlength <<= 20; //for unit MB
				break;
			case 'c' : // packets count for saving
				partcount = atoi( optarg );
				break;
			case 'h' :
			default :
				printUsage();
				break;
		}
	}

    if( strlen(filename) == 0 )
    {
         printf("** Filename should be filled\n");
         printUsage();
         return -1;
    }
		    
	if( ( mqfp = fopen( filename, "r" ) ) == NULL )
	{
		printf("%s:%d file open error\n", __FILE__, __LINE__ );
		return -1;
	}

	//translation
	totalcount = doCalcBps( mqfp, filename, partlength, partcount );


	fclose( mqfp );
	return 0;
}

static u64 doCalcBps( \
		FILE* mqfp, \
		char* filename, \
		int partlength, \
		int partcount )
{
	int filecount = 0;
	int prefilecount = 0;
	u64 packetcount = 0;
	u64 overhead = 0;
	char ofilename[128] = {0,};
	double increment = 0.;

	fprintf( stdout, "\n");

    fseek( mqfp, 0, SEEK_END );
    prefilecount = ftell( mqfp );
    rewind( mqfp );
    sleep(5);

	do
	{
		fseek( mqfp, 0, SEEK_END );
		filecount = ftell( mqfp );
		rewind( mqfp );

		increment = (double)( filecount - prefilecount );
        increment *= 8.0;
        increment /= 5.0; //5 seconds
		increment /= (double)1024.0; //kilo
		increment /= (double)1024.0; //mega
        prefilecount = filecount;

		fprintf( stdout, "\t------> %010f Mbps = %010f MBps\n", increment, increment/8. );
		fflush(stdout);
		sleep(5); 
	}
	while( 1 ); 


	return packetcount;
}

