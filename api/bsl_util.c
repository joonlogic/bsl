/********************************************************************
 *  FILE   : bsl_util.c
 *  Author : joon
 *
 *  Library : 
 *  Export Function :
 *       EnumResultCode bsl_toPcap( ... )
 *
 ********************************************************************
 *                    Change Log
 *
 *    Date          Author          Note
 *  -----------   -----------  ----------------------------------   
 *  2014.12.17       joon        This File is written first.
 *                                       
 ********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pcap.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "bsl_type.h"
#include "bsl_def.h"
#include "bsl_dbg.h"
#include "bsl_system.h"
#include "bsl_msgif.h"
#include "bsl_api.h"
#include "bsl_proto.h"

#include "../module/bsl_ctl.h"

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
#define SIZE_DMA_ALIGN                  128

#define IS_PADDED( TRACEP )  \
	    ( *(u64*)(TRACEP) & ( 1 << BIT_PADDED ) ) ? TRUE : FALSE
#define GET_FIELD_LENGTH( TRACEP )          *(u64*)(TRACEP) & 0xFFFF;
#define GET_FIELD_SEQNUM( TRACEP )          ( (*(u64*)(TRACEP)) >> 16 ) & 0xFFFF;

#define GET_PADDED_LENGTH( UNITLEN )     \
	    ( ( ( ( UNITLEN - 1 ) / SIZE_DMA_ALIGN ) + 1 ) * SIZE_DMA_ALIGN )
#define GET_NO_PADDED_LENGTH( UNITLEN )  ( ( UNITLEN ) + SIZE_DMA_UP_HEADER )

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

static EntryFlag sEntryFlag[SIZE_MAX_CARD][SIZE_MAX_PORT] = \
   {\
	  { {TRUE, FALSE, FALSE, FALSE, FALSE}, \
	    {TRUE, FALSE, FALSE, FALSE, FALSE} }, \
	  { {TRUE, FALSE, FALSE, FALSE, FALSE}, \
	    {TRUE, FALSE, FALSE, FALSE, FALSE} }, \
	  { {TRUE, FALSE, FALSE, FALSE, FALSE}, \
	    {TRUE, FALSE, FALSE, FALSE, FALSE} }, \
	  { {TRUE, FALSE, FALSE, FALSE, FALSE}, \
	    {TRUE, FALSE, FALSE, FALSE, FALSE} }, \
	  { {TRUE, FALSE, FALSE, FALSE, FALSE}, \
	    {TRUE, FALSE, FALSE, FALSE, FALSE} }, \
   };
static EntryValue sEntryValue[SIZE_MAX_CARD][SIZE_MAX_PORT] = { {0,}, };

static u64 transFileFormat( \
		int bslfd, \
		int cardid, 
		int portid,
		EntryValue* pEntryValue );
static u64 doTransPcap( EntryOverhead* overhead, int partlength, int partcount, int bslfd, int cardid, int portid, pcap_dumper_t* pDumper );
static u64 getUnitLength( EntryOverhead* overhead, int bslfd, int cardid, int portid, int* zzogari, int isfirst );
static char* getDateForFilename( EntryValue* pEntryValue );
static boolean checkSplit( EntryFlag* pEntryFlag, EntryValue* pEntryValue, u64 pktcount, EntryOverhead* pOverhead );

//debug
static void printEntryFlagValue( int cardid, int portid );

int bsl_toPcap( char* filename, int cardid, int portid )
{
	int sel = 0;
	int partlength = 0;
	int partcount = 0;
	u64 totalcount = 0;
	char ofilename[128] = {0,};
    unsigned long offset_time = 0;
	void* tracep = NULL;
    int bslfd = 0;
	const char* prefix_file = "bslcaps.";

	printf("%s: enter\n", __func__ );

	BSL_CHECK_NULL( filename, -1 );
	strcpy( sEntryValue[cardid][portid].filename, filename );

	printf("%s: filename %s\n", __func__, filename );

    if( strlen(sEntryValue[cardid][portid].filename) == 0 )
    {
         printf("** Filename should be filled\n");
         return -1;
    }
    else
    {
        char* ptr_filename;
        int ret;

        ptr_filename = strstr( sEntryValue[cardid][portid].filename, prefix_file );
        if( ptr_filename == NULL )
        {
             printf("** Filename %s is not recognized format ( bslcaps.nnnn.bcap )\n", sEntryValue[cardid][portid].filename );
             return -1;
        }

		printf("%s: ptr_filename %s\n", __func__, ptr_filename );
            
        offset_time = strtol( ptr_filename + strlen( prefix_file ), NULL, 10 );
        printf("START time_t = %llu\n", offset_time ); 
        fflush( stdout );
        sEntryValue[cardid][portid].epoch = offset_time;
    }
		    
	if( ( bslfd = open( sEntryValue[cardid][portid].filename, O_LARGEFILE, 0667 ) ) == -1 )
	{
		printf("%s:%d file open error, %s\n", __FILE__, __LINE__, sEntryValue[cardid][portid].filename );
		return -1;
	}

	//translation
	totalcount = transFileFormat( bslfd, cardid, portid, &sEntryValue[cardid][portid] );

	printf("==== Total %llu packets captured ====\n\n", totalcount );
	fflush(stdout);

	close( bslfd );
	return 0;
}

static u64 transFileFormat( \
		int bslfd, \
		int cardid, 
		int portid,
		EntryValue* pEntryValue )
{
	int filecount = 0;
	u64 packetcount = 0;
	EntryOverhead overhead = {0,};
	char ofilename[128] = {0,};
	const char* prefix_file = "/var/www/html/BSL/bslcaps.";

	//ready for pcap dump file
	strcpy( ofilename, pEntryValue->filename );
	sprintf( ofilename, "%s%s.[%1d][%1d].pcap", prefix_file, getDateForFilename( pEntryValue ), cardid, portid );

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

		packetcount += doTransPcap( &overhead, pEntryValue->partlength, pEntryValue->partcount, bslfd, cardid, portid, pDumper );

		//close
		pcap_dump_close( pDumper );
        filecount++;
		sprintf( ofilename, "%s%s.(%1d).pcap", prefix_file, getDateForFilename( pEntryValue ), portid );
	}
	while( overhead.paylength != 0 ); //overhead.paylength = 0 means final

	pcap_close( pd );

	return packetcount;
}

static u64 doTransPcap( EntryOverhead* overhead, int partlength, int partcount, int bslfd, int cardid, int portid, pcap_dumper_t* pDumper )
{
	u64 length = 0;
	u64 pktcount = 0;
	u64 pktcount_local = 0;
	int zzogari = 0;
	char pktbuf[32768];
	struct pcap_pkthdr pcap_header;
	struct timezone tz;
	int readbyte = 0;
	int isfirst = 1;

	do
	{
		length = getUnitLength( overhead, bslfd, cardid, portid, &zzogari, isfirst );
		if( length == 0 )
		{
			overhead->paylength = 0;
			return pktcount_local;
		}
		isfirst = 0;

		readbyte = read( bslfd, pktbuf, length );
		if( readbyte == 0 ) 
		{
			printf("%s: No read bytes\n", __FILE__);
			overhead->paylength = 0;
			return pktcount_local;
		}

        if( sEntryFlag[cardid][portid].countonly == 0 )
        {
#ifdef DO_SWAP_PLOAD
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
				for( i=0; i<length; i+=8 ) {
					*(unsigned long long*)&pktbuf[i] = BSLSWAP64(*(unsigned long long*)&pktbuf[i]);
				} 
            }while(0);
#endif

            pcap_header.len = length-zzogari;
            pcap_header.caplen = length-zzogari;
            gettimeofday( &pcap_header.ts, &tz ); // TODO: NOT realtime

            pcap_dump( (u_char*)pDumper, &pcap_header, (const u_char*)pktbuf );
        }
		pktcount++;
		pktcount_local++;

        if( pktcount % 100000 == 0 ) 
        {
            printf("------> %010d packets in progress\r\n", pktcount );
        }

        if( checkSplit( &sEntryFlag[cardid][portid], &sEntryValue[cardid][portid], pktcount_local, overhead ) == TRUE ) break; 
	} while( 1 );

	return pktcount_local;
}

static boolean checkSplit( EntryFlag* pEntryFlag, EntryValue* pEntryValue, u64 pktcount, EntryOverhead* pOverhead )
{
//    printEntryFlagValue(); //erase please

    if( pEntryFlag->countonly == TRUE ) return FALSE;

    if( pEntryFlag->parttime == TRUE )
    {
        if( pEntryValue->offset_timestamp + pEntryValue->parttime > pOverhead->timestamp ) return FALSE;
        pEntryValue->offset_timestamp = pOverhead->timestamp;
        return TRUE;
    }
    if( pEntryFlag->partlength == TRUE ) ; //not yet support
    if( pEntryFlag->partcount == TRUE ) ; //not yet support

    return FALSE;
}

static void printEntryFlagValue( int cardid, int portid )
{
    static int index = 0;

    printf("%s: %d ------------------------ \n", __FUNCTION__, index++ );

    printf("   filename = %s, %s\n", sEntryFlag[cardid][portid].filename == TRUE ? "TRUE" : "FALSE", sEntryValue[cardid][portid].filename );
    printf("   partlength = %s, %ld\n", sEntryFlag[cardid][portid].partlength == TRUE ? "TRUE" : "FALSE", sEntryValue[cardid][portid].partlength );
    printf("   partcount = %s, %ld\n", sEntryFlag[cardid][portid].partcount == TRUE ? "TRUE" : "FALSE", sEntryValue[cardid][portid].partcount );
    printf("   parttime = %s, %ld\n", sEntryFlag[cardid][portid].parttime == TRUE ? "TRUE" : "FALSE", sEntryValue[cardid][portid].parttime );
    printf("   first_timestamp = %ld\n", sEntryValue[cardid][portid].first_timestamp );
    printf("   offset_timestamp = %ld\n", sEntryValue[cardid][portid].offset_timestamp );
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

static u64 getUnitLength( EntryOverhead* poverhead, int bslfd, int cardid, int portid, int* zzogari, int isfirst )
{
	u64 readbyte = 0;
	u64 ioverhead[2] = {0,};
	u64 unitlength = 0;
	boolean ispadded;
    static u64 sreadbyte[SIZE_MAX_CARD][2] = { 0, }; //2 means number of ports

	if( isfirst ) sreadbyte[cardid][portid] = 0;

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

        sreadbyte[cardid][portid] += readbyte;
 
#ifndef DO_SWAP_PLOAD
		ioverhead[0] = ntohll(ioverhead[0]);
#endif

		if( ioverhead[0] >> 16 == PADD_PATTERN_DMA >> 16 ) continue;
		if( ( ioverhead[0] >> 48 ) != DELIM_PATTERN_DMA ) 
		{
			if( ioverhead[0] != 0 || isfirst == 0 )
			{
				printf("%s:%d OVERHEAD MISMATCHED %016llX, sreadbyte = %d ( 0x%X )\n", __FILE__, __LINE__, ioverhead[0], sreadbyte[cardid][portid], sreadbyte[cardid][portid] );
			}
#if 0 //at 64bit OS
			fread( ioverhead, 1, 4, bslfp ); //flush 4 bytes
#endif
			continue;
		}
		break;

	} while(1);

	unitlength = GET_FIELD_LENGTH( &ioverhead[0] );
	if( unitlength == 0 )
	{
		poverhead->paylength = 0;
        printf("%s:%d unitlength MISMATCHED !!!! %016llX %016llX, sreadbyte = %d ( 0x%X )\n", __FILE__, __LINE__, ioverhead[0], ioverhead[1], sreadbyte[cardid][portid], sreadbyte[cardid][portid] );
		return 0;
	}

	if( unitlength%8 ) {
		unitlength+=8;
		*zzogari = 8 - unitlength%8;
	}
	else 
		*zzogari = 0;

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
    sreadbyte[cardid][portid] += unitlength;

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
    
    if( isfirst )
    {
        isfirst = 0;
        sEntryValue[cardid][portid].first_timestamp = poverhead->timestamp;
        sEntryValue[cardid][portid].offset_timestamp = poverhead->timestamp;
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
