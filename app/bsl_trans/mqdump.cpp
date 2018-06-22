#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>
#include <pcap.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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
    fprintf(stderr, "mqtrans version: %s", VERSION);
	fprintf(stderr, "\n");
    fprintf(stderr, "Usage: mqtrans [args] \n" );
	fprintf(stderr, "\t-f <filename>\t\tinput MPQA capture file\n"
					"\t-l <out file size>\t\tfor file cutting ( unit:MB )\n"
					"\t-c <packet counts per file>\t\tfor file cutting\n"
					"Note: output filename extension is to be <input filename>.pcap\n"
					"\n");
	exit(0);
}

static u64 dumpFile( \
		int mqfd, \
		char* filename, \
		int partlength, \
		int partcount );
static u64 doTransPcap( u64* overhead, int partlength, int partcount, int mqfd );

int main( int argc, char* argv[])
{
	int sel = 0;
	int partlength = 0;
	int partcount = 0;
	u64 totalcount = 0;
	char filename[128] = {0,};
	char ofilename[128] = {0,};
	void* tracep = NULL;
    int mqfd = 0;

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
		    
	if( ( mqfd = open( filename, O_LARGEFILE, 0667 ) ) == -1 )
	{
		printf("%s:%d file open error, %s\n", __FILE__, __LINE__, filename );
		return -1;
	}

	//translation
	totalcount = dumpFile( mqfd, filename, partlength, partcount );

	printf("==== Total %llu packets captured ====\n\n", totalcount );

	close( mqfd );
	return 0;
}

static u64 dumpFile( \
		int mqfd, \
		char* filename, \
		int partlength, \
		int partcount )
{
	int filecount = 0;
	u64 packetcount = 0;
	u64 overhead = 0;
	char ofilename[128] = {0,};

    packetcount += doTransPcap( &overhead, partlength, partcount, mqfd );

	return packetcount;
}

static u64 doTransPcap( u64* overhead, int partlength, int partcount, int mqfd )
{
	u64 length = 0;
	u64 pktcount = 0;
	int zzogari = 0;
	char pktbuf[32768];
	struct pcap_pkthdr pcap_header;
	struct timezone tz;
	int readbyte = 0;
    long laddr = 0;

	do
	{
		readbyte = read( mqfd, pktbuf, 16 );
		if( readbyte == 0 ) 
		{
			printf("%s: No read bytes\n", __FILE__);
			*overhead = 0;
			return pktcount;
		}

        printf("[%016X] %08X %08X %08X %08X\n", laddr, *(unsigned long*)pktbuf, *(unsigned long*)&pktbuf[4]
, *(unsigned long*)&pktbuf[8], *(unsigned long*)&pktbuf[12] );
        laddr += 16;

#if 0
		//swap
		{
			unsigned short* pswap = (unsigned short*)pktbuf;
			for( int i=0; i<length/2; i++, pswap++ )
			{
				*pswap = ntohs(*pswap); 
			}
		}
#endif

		pktcount++;

	} while( 1 );

	return pktcount;
}

