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


#define DELIM_PATTERN_DMA               (u64)0x00000000
#define BIT_PADDED                      23
#define SIZEOF_OVERHEAD                 16
#define PATTERN_PADDING                 (u64)0x5555555555555555
#define SIZE_DMA_ALIGN                  128
#define IS_PADDED( TRACEP )  \
	    ( *(u64*)(TRACEP) & ( 1 << BIT_PADDED ) ) ? TRUE : FALSE

#define GET_LENGTH_FIELD( TRACEP )       *(u64*)(TRACEP) & 0xFFFF;
#define GET_PADDED_LENGTH( UNITLEN )     \
	    ( ( ( ( UNITLEN - 1 ) / SIZE_DMA_ALIGN ) + 1 ) * SIZE_DMA_ALIGN )
#define GET_NO_PADDED_LENGTH( UNITLEN )  ( ( UNITLEN ) + SIZE_DMA_UP_HEADER )


static const char* VERSION = "20141220";

static void printUsage( void )
{
	fprintf(stderr, "\n");
    fprintf(stderr, "bsl_trans version: %s", VERSION);
	fprintf(stderr, "\n");
    fprintf(stderr, "Usage: bsl_trans [args] \n" );
	fprintf(stderr, "\t-f <filename>\t\tinput bcaps capture file\n"
					"\t-l <out file size>\t\tfor file cutting ( unit:MB )\n"
					"\t-c <packet counts per file>\t\tfor file cutting\n"
					"Note: output filename extension is to be <input filename>.pcap\n"
					"\n");
	exit(0);
}

static u64 transFileFormat( \
		FILE* bsl_fp, \
		char* filename, \
		int partlength, \
		int partcount );
static u64 doTransPcap( u64* overhead, int partlength, int partcount, FILE* bsl_fp, pcap_dumper_t* pDumper );
static u64 getUnitLength( u64* overhead, FILE* bsl_fp, int* zzogari );

int main( int argc, char* argv[])
{
	int sel = 0;
	int partlength = 0;
	int partcount = 0;
	u64 totalcount = 0;
	char filename[128] = {0,};
	char ofilename[128] = {0,};
	void* tracep = NULL;
	FILE* bsl_fp = NULL;

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
		    
	if( ( bsl_fp = fopen( filename, "r" ) ) == NULL )
	{
		printf("%s:%d file open error\n", __FILE__, __LINE__ );
		return -1;
	}

	//translation
	totalcount = transFileFormat( bsl_fp, filename, partlength, partcount );

	printf("==== Total %llu packets captured ====\n\n", totalcount );

	fclose( bsl_fp );
	return 0;
}

static u64 transFileFormat( \
		FILE* bsl_fp, \
		char* filename, \
		int partlength, \
		int partcount )
{
	int filecount = 0;
	u64 packetcount = 0;
	u64 overhead = 0;
	char ofilename[128] = {0,};

	//ready for pcap dump file
	strcpy( ofilename, filename );
	if( ( partlength == 0 ) && ( partcount == 0 ) ) strcat( ofilename, ".pcap" );
	else sprintf( ofilename, "%s.%d.pcap", filename, filecount );

	pcap_dumper_t* pDumper;
	pcap_t*        pd;
	pd = pcap_open_dead( DLT_EN10MB, 1500 );

	do
	{
		pDumper = pcap_dump_open( pd, ofilename );
		if( pDumper == NULL )
		{
			fprintf( stderr, "%s: open error - %s\n", __FUNCTION__, ofilename );
			pcap_close( pd );
			return -1;
		}

		packetcount += doTransPcap( &overhead, partlength, partcount, bsl_fp, pDumper );

		//close
		pcap_dump_close( pDumper );
	}
	while( overhead != 0 ); //overhead 0 means final

	pcap_close( pd );

	return packetcount;
}

static u64 doTransPcap( u64* overhead, int partlength, int partcount, FILE* bsl_fp, pcap_dumper_t* pDumper )
{
	u64 length = 0;
	u64 pktcount = 0;
	int zzogari = 0;
	char pktbuf[32768];
	struct pcap_pkthdr pcap_header;
	struct timezone tz;
	int readbyte = 0;

	do
	{
		length = getUnitLength( overhead, bsl_fp, &zzogari );
		if( length == 0 )
		{
			*overhead = 0;
			return pktcount;
		}

		readbyte = fread( pktbuf, 1, length, bsl_fp );
		if( readbyte == 0 ) 
		{
			printf("%s: No read bytes\n", __FILE__);
			*overhead = 0;
			return pktcount;
		}
        //debug_s
        else if( readbyte != length )
        {
			printf("%s:%d read bytes = %d\n", __FILE__, __LINE__, readbyte);
			*overhead = 0;
			return 0;
        }
        //debug_e

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

#if 1
		pcap_header.len = length-zzogari;
		pcap_header.caplen = length-zzogari;
		gettimeofday( &pcap_header.ts, &tz ); // TODO: NOT realtime

		pcap_dump( (u_char*)pDumper, &pcap_header, (const u_char*)pktbuf );
#endif
		pktcount++;

	} while( 1 );

	return pktcount;
}

static u64 getUnitLength( u64* overhead, FILE* bsl_fp, int* zzogari )
{
	u64 readbyte = 0;
	u64 ioverhead[2] = {0,};
	u64 unitlength = 0;
	boolean ispadded;
	static int debugprint = 0;
    static u64 sreadbyte = 0;

	do
	{
		//read first 16 byte overhead
#if 0 //at 64bit OS
		readbyte = fread( ioverhead, 1, SIZEOF_OVERHEAD-4, bsl_fp );
#else
		readbyte = fread( ioverhead, 1, SIZEOF_OVERHEAD, bsl_fp );
#endif
		if( readbyte == 0 ) 
		{
			printf("%s:%d No read bytes\n", __FILE__, __LINE__);
			*overhead = 0;
			return 0;
		}
        //debug_s
        else if( readbyte != SIZEOF_OVERHEAD )
        {
			printf("%s:%d read bytes = %d\n", __FILE__, __LINE__, readbyte);
			*overhead = 0;
			return 0;
        }
        //debug_e

        sreadbyte += readbyte;

		if( ioverhead[0] == PATTERN_PADDING ) continue;

		if( ( ioverhead[0] >> 32 ) != DELIM_PATTERN_DMA ) 
		{
			if( ioverhead[0] != 0 || debugprint == 1 )
			{
				printf("%s:%d OVERHEAD MISMATCHED !!!! %016llX sreadbyte = %d ( 0x%X )\n", __FILE__, __LINE__, ioverhead[0], sreadbyte, sreadbyte );
			}
#if 0 //at 64bit OS
			fread( ioverhead, 1, 4, bsl_fp ); //flush 4 bytes
#endif
			continue;
		}
		else break;

	} while(1);

	unitlength = GET_LENGTH_FIELD( &ioverhead[0] );
	if( unitlength == 0 )
	{
		*overhead = 0;
		return 0;
	}

    sreadbyte += readbyte;
	return unitlength;// - SIZEOF_OVERHEAD;
}

