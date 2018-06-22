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


#define DELIM_PATTERN_DMA               (u64)0
#define PADD_PATTERN_DMA                (u64)0x5555555555555555ll
#define BIT_PADDED                      23
#define SIZEOF_OVERHEAD                 8
//#define SIZEOF_OVERHEAD_HALF            8
#define SIZE_DMA_ALIGN                  128

#define IS_PADDED( TRACEP )  \
	    ( *(u64*)(TRACEP) & ( 1 << BIT_PADDED ) ) ? TRUE : FALSE
#define GET_FIELD_LENGTH( TRACEP )          *(u64*)(TRACEP) & 0xFFFF;
#define GET_FIELD_SEQNUM( TRACEP )          ( (*(u64*)(TRACEP)) >> 16 ) & 0xFFFF;
//#define GET_FIELD_TIMESTAMP( TRACEP )       ((*(u64*)(TRACEP)) >> 20);
//#define GET_FIELD_MATCHINDEX( TRACEP )      *(u64*)(TRACEP) & 0xFFFFF;

#define GET_PADDED_LENGTH( UNITLEN )     \
	    ( ( ( ( UNITLEN - 1 ) / SIZE_DMA_ALIGN ) + 1 ) * SIZE_DMA_ALIGN )
#define GET_NO_PADDED_LENGTH( UNITLEN )  ( ( UNITLEN ) + SIZE_DMA_UP_HEADER )

static const char* VERSION = "20141217";

static void printUsage( void )
{
	fprintf(stderr, "\n");
    fprintf(stderr, "bsl_trans version: %s", VERSION);
	fprintf(stderr, "\n");
    fprintf(stderr, "Usage: bsl_trans [args] \n" );
	fprintf(stderr, "\t-f <filename>\t\tinput BSL capture file\n"
					"\t-l <out file size>\t\tfor file split ( unit : MB )\n"
					"\t-n <packet counts per file>\t\tfor file split\n"
					"\t-i <time interval>\t\tfor file split ( unit : Min. )\n"
					"\t-c packet counts only - No format conversion\n"
					"Note: output filename extension is to be <input filename>.pcap\n"
					"\n");
	exit(0);
}


#define SIZE_FILENAME_LENGTH          256
#define SIZE_BSL_DELIM                4  //00000000

typedef struct entryflag
{
    unsigned char filename;
    unsigned char partlength;
    unsigned char partcount;
    unsigned char parttime;
    unsigned char countonly;
} EntryFlag;

typedef struct entryvalue
{
    char filename[SIZE_FILENAME_LENGTH];
    unsigned long partlength;
    unsigned long partcount;
    unsigned long parttime; //to be converted to msec
    unsigned long first_timestamp; //msec
    unsigned long offset_timestamp; //msec
    time_t        epoch;
} EntryValue;

typedef struct overhead
{
    unsigned char delim[4];
    boolean       ispadded;
    unsigned int  paylength;
    unsigned long timestamp; //msec
    unsigned int  matchindex;
} EntryOverhead;

static EntryFlag sEntryFlag = { 0, };
static EntryValue sEntryValue = { {0,}, };

static u64 transFileFormat( \
		int bslfd, \
		EntryValue* pEntryValue );
static u64 doTransPcap( EntryOverhead* overhead, int partlength, int partcount, int bslfd, pcap_dumper_t* pDumper );
static u64 getUnitLength( EntryOverhead* overhead, int bslfd, int* zzogari );
static char* getDateForFilename( EntryValue* pEntryValue );
static boolean checkSplit( EntryFlag* pEntryFlag, EntryValue* pEntryValue, u64 pktcount, EntryOverhead* pOverhead );

//debug
static void printEntryFlagValue( void );

int main( int argc, char* argv[])
{
	int sel = 0;
	int partlength = 0;
	int partcount = 0;
	u64 totalcount = 0;
	char ofilename[128] = {0,};
    unsigned long offset_time = 0;
	void* tracep = NULL;
    int bslfd = 0;

	while( ( sel = getopt( argc, argv, "f:l:n:i:ch" ) ) != -1 )
	{
		switch( sel )
		{
			case 'f' : // to be processed filename
                sEntryFlag.filename = true;
				strcpy( sEntryValue.filename, optarg );
				break;
			case 'l' : // part size
                sEntryFlag.partlength = true;
				sEntryValue.partlength = atoi( optarg );
				sEntryValue.partlength <<= 20; //for unit MB
				break;
			case 'n' : // packets per file for saving
                sEntryFlag.partcount = true;
				sEntryValue.partcount = atoi( optarg );
				break;
			case 'i' : // time intervals per file for saving
                sEntryFlag.parttime = true;
				sEntryValue.parttime = atoi( optarg );
				sEntryValue.parttime *= 60 * 1000; 
                //unit:msec, 60 means 1min. 1000 means 1sec.
                //for debug force to 1sec. plz. erase 
				sEntryValue.parttime /= 60; 
				break;
			case 'c' : // packets count for saving
				sEntryFlag.countonly = 1;
				break;
			case 'h' :
			default :
				printUsage();
				break;
		}
	}

    printEntryFlagValue();

    if( strlen(sEntryValue.filename) == 0 )
    {
         printf("** Filename should be filled\n");
         printUsage();
         return -1;
    }
    else
    {
        const char* prefix_file = "bslcaps.";
        char* ptr_filename;
        int ret;

        ptr_filename = strstr( sEntryValue.filename, prefix_file );
        if( ptr_filename == NULL )
        {
             printf("** Filename %s is not recognized format ( bslcaps.nnnn.bcap )\n", sEntryValue.filename );
             printUsage();
             return -1;
        }
            
        offset_time = strtol( ptr_filename + strlen( prefix_file ), NULL, 10 );
        printf("START time_t = %llu\n", offset_time ); 
        fflush( stdout );
        sEntryValue.epoch = offset_time;
    }
		    
	if( ( bslfd = open( sEntryValue.filename, O_LARGEFILE, 0667 ) ) == -1 )
	{
		printf("%s:%d file open error, %s\n", __FILE__, __LINE__, sEntryValue.filename );
		return -1;
	}

	//translation
	totalcount = transFileFormat( bslfd, &sEntryValue );

	printf("==== Total %llu packets captured ====\n\n", totalcount );

	close( bslfd );
	return 0;
}

static u64 transFileFormat( \
		int bslfd, \
		EntryValue* pEntryValue )
{
	int filecount = 0;
	u64 packetcount = 0;
	EntryOverhead overhead = {0,};
	char ofilename[128] = {0,};

	//ready for pcap dump file
	strcpy( ofilename, pEntryValue->filename );
	sprintf( ofilename, "%s.%s.pcap", pEntryValue->filename, getDateForFilename( pEntryValue ) );

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

		packetcount += doTransPcap( &overhead, pEntryValue->partlength, pEntryValue->partcount, bslfd, pDumper );

		//close
		pcap_dump_close( pDumper );
        filecount++;
        sprintf( ofilename, "%s.%s.pcap", pEntryValue->filename, getDateForFilename( pEntryValue ) );
	}
	while( overhead.paylength != 0 ); //overhead.paylength = 0 means final

	pcap_close( pd );

	return packetcount;
}

static u64 doTransPcap( EntryOverhead* overhead, int partlength, int partcount, int bslfd, pcap_dumper_t* pDumper )
{
	u64 length = 0;
	u64 pktcount = 0;
	u64 pktcount_local = 0;
	int zzogari = 0;
	char pktbuf[32768];
	struct pcap_pkthdr pcap_header;
	struct timezone tz;
	int readbyte = 0;

	do
	{
		length = getUnitLength( overhead, bslfd, &zzogari );
		if( length == 0 )
		{
			overhead->paylength = 0;
			return pktcount_local;
		}

		readbyte = read( bslfd, pktbuf, length );
		if( readbyte == 0 ) 
		{
			printf("%s: No read bytes\n", __FILE__);
			overhead->paylength = 0;
			return pktcount_local;
		}

        if( sEntryFlag.countonly == 0 )
        {
#if 0
            do //swap
            {
#define BSLSWAP32(val) \
	((unsigned int)((((unsigned int)(val) & (unsigned int)0x000000ffU) << 24) | \
	(((unsigned int)(val) & (unsigned int)0x0000ff00U) <<  8) | \
	(((unsigned int)(val) & (unsigned int)0x00ff0000U) >>  8) | \
	(((unsigned int)(val) & (unsigned int)0xff000000U) >> 24)))

                // index changing
                // plz refer as follows
                //
                // 0 1 2 3 4 5 6 7 8 9 a b c d e f ----> saved file address
                // -------------------------------
                // 4 5 6 7 0 1 2 3 c d e d 8 9 a b ----> DMAed contents
                //

                int i=0;
                unsigned char tmpbuf[8] = {0,};
                for( i=0; i<length; i+=8 )
                {
                    memcpy( tmpbuf, &pktbuf[i], 8 );
					*(unsigned int*)&pktbuf[i] = BSLSWAP32(*(unsigned int*)&pktbuf[i+4]);
					*(unsigned int*)&pktbuf[i+4] = BSLSWAP32(*(unsigned int*)&tmpbuf[0]);
//                    memcpy( &pktbuf[i], &tmpbuf[4], 4 );
//                    memcpy( &pktbuf[i+4], &tmpbuf[0], 4 );
                } 
            }while(0);
#endif

#if 1
            do //swap
            {
#define BSLSWAP64(val) \
	((unsigned long long)((((unsigned long long)(val) & (unsigned long long)0x000000ffULL) << 56) | \
	(((unsigned long long)(val) & (unsigned long long)0x0000ff00ULL) << 40) | \
	(((unsigned long long)(val) & (unsigned long long)0x00ff0000ULL) << 24) | \
	(((unsigned long long)(val) & (unsigned long long)0xff000000ULL) << 8 ) | \
	(((unsigned long long)(val) & (unsigned long long)0xff00000000ULL) >> 8) | \
	(((unsigned long long)(val) & (unsigned long long)0xff0000000000ULL) >> 24) | \
	(((unsigned long long)(val) & (unsigned long long)0xff000000000000ULL) >> 40) | \
	(((unsigned long long)(val) & (unsigned long long)0xff00000000000000ULL) >> 56)))

                int i=0;
                unsigned char tmpbuf[16] = {0,};
				for( i=0; i<length-zzogari; i+=8 ) {
					memcpy( tmpbuf, &pktbuf[i], 8 );
//					printf("[Before] %lX\n", *(unsigned long long*)&pktbuf[i] );
					*(unsigned long long*)&pktbuf[i] = BSLSWAP64(*(unsigned long long*)&pktbuf[i]);
//					printf("[After] %lX\n", *(unsigned long long*)&pktbuf[i] );
//					*(unsigned long long*)&pktbuf[i+8] = BSLSWAP64(*(unsigned long long*)&tmpbuf[0]); 
				} 
/*
				if( ( zzogari == 0 ) || ( zzogari > 8 ) ) {
					memcpy( tmpbuf, &pktbuf[i], 16 );
					*(unsigned long long*)&pktbuf[i] = BSLSWAP64(*(unsigned long long*)&pktbuf[i+8]);
					*(unsigned long long*)&pktbuf[i+8] = BSLSWAP64(*(unsigned long long*)&tmpbuf[0]); 
				}
				else {
					*(unsigned long long*)&pktbuf[i] = BSLSWAP64(*(unsigned long long*)&pktbuf[i]);
				}
*/
            }while(0);
#endif

            pcap_header.len = length-zzogari;
            pcap_header.caplen = length-zzogari;
            gettimeofday( &pcap_header.ts, &tz ); // TODO: NOT realtime

            pcap_dump( (u_char*)pDumper, &pcap_header, (const u_char*)pktbuf );
        }
		pktcount++;
		pktcount_local++;

        if( pktcount % 10000 == 0 ) 
        {
            printf("------> %010d packets in progress\r", pktcount );
            fflush(stdout);
        }

        if( checkSplit( &sEntryFlag, &sEntryValue, pktcount_local, overhead ) == true ) break; 

	} while( 1 );

	return pktcount_local;
}

static boolean checkSplit( EntryFlag* pEntryFlag, EntryValue* pEntryValue, u64 pktcount, EntryOverhead* pOverhead )
{
//    printEntryFlagValue(); //erase please

    if( pEntryFlag->countonly == true ) return false;

    if( pEntryFlag->parttime == true )
    {
        if( pEntryValue->offset_timestamp + pEntryValue->parttime > pOverhead->timestamp ) return false;
        pEntryValue->offset_timestamp = pOverhead->timestamp;
        return true;
    }
    if( pEntryFlag->partlength == true ) ; //not yet support
    if( pEntryFlag->partcount == true ) ; //not yet support

    return false;
}

static void printEntryFlagValue( void )
{
    static int index = 0;

    printf("%s: %d ------------------------ \n", __FUNCTION__, index++ );

    printf("   filename = %s, %s\n", sEntryFlag.filename == true ? "TRUE" : "FALSE", sEntryValue.filename );
    printf("   partlength = %s, %ld\n", sEntryFlag.partlength == true ? "TRUE" : "FALSE", sEntryValue.partlength );
    printf("   partcount = %s, %ld\n", sEntryFlag.partcount == true ? "TRUE" : "FALSE", sEntryValue.partcount );
    printf("   parttime = %s, %ld\n", sEntryFlag.parttime == true ? "TRUE" : "FALSE", sEntryValue.parttime );
    printf("   first_timestamp = %ld\n", sEntryValue.first_timestamp );
    printf("   offset_timestamp = %ld\n", sEntryValue.offset_timestamp );
}


static void printOverhead( EntryOverhead* poverhead )
{
    static int index = 0;

    printf("%s: %d ------------------------ \n", __FUNCTION__, index++ );
    printf("    delim = %02X%02X%02X%02X%02X\n", poverhead->delim[0], poverhead->delim[1], poverhead->delim[2], poverhead->delim[3], poverhead->delim[4] ); 
    printf("    ispadded = %d\n", poverhead->ispadded );
    printf("    paylength = %d\n", poverhead->paylength );
    printf("    timestamp = %lu\n", poverhead->timestamp );
    printf("    matchindex = %d\n", poverhead->matchindex );
}

static u64 getUnitLength( EntryOverhead* poverhead, int bslfd, int* zzogari )
{
	u64 readbyte = 0;
	u64 ioverhead[2] = {0,};
	u64 unitlength = 0;
	boolean ispadded;
	static int isfirst = 1;
    static u64 sreadbyte = 0;

	do
	{
		//read first 16 byte overhead
#if 0 //at 64bit OS
		readbyte = fread( ioverhead, 1, SIZEOF_OVERHEAD-4, bslfp );
#else
		readbyte = read( bslfd, ioverhead, SIZEOF_OVERHEAD );
#endif
		if( readbyte == 0 ) 
		{
			printf("%s:%d No more bytes to be read\n", __FILE__, __LINE__);
			poverhead->paylength = 0;
			return 0;
		}

        sreadbyte += readbyte;

		if( ioverhead[0] == PADD_PATTERN_DMA ) continue;
		if( ( ioverhead[0] >> 32 ) != DELIM_PATTERN_DMA ) 
		{
			if( ioverhead[0] != 0 || isfirst == 0 )
			{
				printf("%s:%d OVERHEAD MISMATCHED %016llX, sreadbyte = %d ( 0x%X )\n", __FILE__, __LINE__, ioverhead[0], sreadbyte, sreadbyte );
			}
#if 0 //at 64bit OS
			fread( ioverhead, 1, 4, bslfp ); //flush 4 bytes
#endif
			continue;
		}
		/*
		else 
        {
            readbyte = read( bslfd, &ioverhead[1], SIZEOF_OVERHEAD );
            if( readbyte == 0 ) 
            {
                printf("%s:%d No read bytes\n", __FILE__, __LINE__);
                poverhead->paylength = 0;
                return 0;
            }
            sreadbyte += readbyte;
            break;
        }
		*/
		break;

	} while(1);

	unitlength = GET_FIELD_LENGTH( &ioverhead[0] );
	if( unitlength == 0 )
	{
		poverhead->paylength = 0;
        printf("%s:%d unitlength MISMATCHED !!!! %016llX %016llX, sreadbyte = %d ( 0x%X )\n", __FILE__, __LINE__, ioverhead[0], ioverhead[1], sreadbyte, sreadbyte );
		return 0;
	}

	if( unitlength%8 ) unitlength+=8;
	*zzogari = unitlength%8;

	unitlength -= ( unitlength %8 );
#if 0 //joon for bsl
	ispadded = IS_PADDED( &ioverhead[0] );

    //debug
    if( ispadded ) printf("***************** IS PADDED *********************\n");

	unitlength = ispadded ? unitlength + SIZE_DMA_ALIGN  : unitlength;

//	printf("%s: unitlength = %lld\n", __FUNCTION__, unitlength );

//	*zzogari = ispadded ? 0 : ( *zzogari + unitlength ) % SIZE_DMA_ALIGN;
#endif //joon for bsl

#if 0 //at 64bit OS
	return unitlength+4;// - SIZEOF_OVERHEAD;
#else
    sreadbyte += unitlength;

#if 0 //joon for bsl
    memcpy( poverhead->delim, ioverhead, SIZE_MQ_DELIM );
    poverhead->paylength = unitlength;
    poverhead->ispadded = ispadded;
    poverhead->timestamp = GET_FIELD_TIMESTAMP( &ioverhead[1] );
    poverhead->matchindex = GET_FIELD_MATCHINDEX( &ioverhead[1] );

    //debug_S
    printOverhead( poverhead );
    //debug_E
#endif //joon for bsl
    
    if( isfirst == 1 ) 
    {
        isfirst = 0;
        sEntryValue.first_timestamp = poverhead->timestamp;
        sEntryValue.offset_timestamp = poverhead->timestamp;
    }

	return unitlength;// - SIZEOF_OVERHEAD;
#endif
}

static char* getDateForFilename( EntryValue* pEntryValue )
{
    time_t filetime;
    static char datestr[128] = {0,};

    filetime = pEntryValue->epoch + ( pEntryValue->offset_timestamp - pEntryValue->first_timestamp ) / 1000; //1000 means msec
    memset( datestr, 0x00, sizeof( datestr ) );
    strftime( datestr, sizeof( datestr ), "%y%m%d.%H%M%S", localtime( &filetime ) );

    return datestr;
}
