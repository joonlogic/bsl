/******************************************************************************
 *
 * File Name:
 *
 *      bsldump.c
 *
 * Description:
 *
 *      This sample demonstrates how to manually setup an SGL DMA transfer.
 *
 * Revision History:
 *
 *      14-10-11 : Start
 *
 ******************************************************************************/


/******************************************************************************
 *
 * NOTE:
 *
 ******************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
// above 3 files are related to stat() function.

#include <fcntl.h>
#include <sys/timeb.h>

#include <stdlib.h>
#include <sys/mman.h>

#include "bsl_dbg.h"
#include "bsl_type.h"
#include "bsl_system.h"
#include "bsl_api.h"
#include "nss_register.h"
#include "nss_ctl.h"

#ifndef READ64
#define READ64(base, offset)            (*(unsigned long long*)(base+((offset)<<3)))
#endif

#ifndef WRITE64
#define WRITE64(base, offset, value)    (*(unsigned long long*)(base+((offset)<<3))=value)
#endif


#ifndef TRUE   
#define TRUE                   1   
#define FALSE                  0 
#endif  

#ifndef boolean   
#define boolean int 
#define true                   1
#define false                  0
#endif 

/**********************************************
 *               Definitions
 *********************************************/
#define FIRST_DMA_TIMEOUT_SEC           60                                       // Max time to wait for DMA completion
#define DMA_TIMEOUT_SEC                 20                                       // Max time to wait for DMA completion
#define UPDATE_DISPLAY_SEC              10                                       // Number of seconds between display updates


#define SIZE_FILENAME_LENGTH 64

typedef struct entryflag
{
	unsigned char filename;
	unsigned char filesize;
	unsigned char buffersizeMB;
	unsigned char bufferTimeoutSec;
	unsigned char dumpsec;
	unsigned char countonly;
} EntryFlag;

typedef struct entryvalue
{
	char filename[SIZE_FILENAME_LENGTH];
	unsigned long filesize;
	unsigned long buffersizeMB;
	unsigned long bufferTimeoutSec;
	unsigned long dumpsec; 
	time_t        startsec;
	time_t        c_dumpsec;    //running information
	unsigned long c_dumpsizeB;  //running information
} EntryValue;

typedef struct dmadescr
{
	unsigned long* page;
	unsigned int pagesize;
	unsigned long* next;
} __attribute__((packed)) DmaDescr_t;

#if 0 //move to nss_ctl.h
typedef struct dmaparams
{
	unsigned long long UserVa;       //User Address for Buffer
	unsigned long long BusAddr_Buff; //Bus Address for Buffer
	unsigned long long BusAddr_Desc; //Bus Address for Descriptors when SGL is used
	unsigned long long VirtAddr_Desc;
	unsigned int       SglSize_Desc; //Total Size for Descriptors
	unsigned int       ByteCount;    //Total Size for Buffer
	struct page**      PageList;
	unsigned int       Channel;      
} __attribute__((packed)) DmaParams_t;
#endif

const static char* VERSION = "20141010";
static EntryFlag sEntryFlag = { 0, };
static EntryValue sEntryValue = { {0,}, };

static void printEntryFlagValue( void );
static boolean checkQuitTime( void );
//static boolean checkTransferSize( void );
//static void writeRemains( PLX_DEVICE_OBJECT* pDevice, int fileno, int index_count, void* pUserBuffer );
static EnumResultCode dommap( int fd, char** map );
static EnumResultCode domunmap( int fd, char** map );


/******************************************************************************
 *
 * Function   :  printUsage
 *
 * Description:  print usage for help
 *
 *****************************************************************************/
static void printUsage( void )
{
	fprintf(stderr, "\n");
	fprintf(stderr, "bsldump version: %s", VERSION);
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: bsldump [args] \n" );
	fprintf(stderr, 
		"\t-s <file size>\t\tfor file dump ( unit : MB )\n"
		"\t-t <time duration>\t\tfor file dump ( unit : Sec. )\n"
		"\t-w <buffer waiting timeout>\tfor file dump ( unit : Sec. )\n"
		"\t-b <unit buffer size>\t\tdefault dual 32MB ( unit : MB )\n"
		"\t-c no file dump\t\tfor bps measurements\n"
		"Note: output filename extension is to be .<time_t>.bcap\n"
		"\n");
}


/******************************************************************************
 *
 * Function   :  main
 *
 * Description:  The main entry point
 *
 *****************************************************************************/
int
main( int argc, char* argv[] )
{
    unsigned int      SglPciAddress;
    int ret; 
	int fd = 0;
    unsigned char     *VaBar0 = 0;
    unsigned char     DmaChannel;
    unsigned short    UserInput;
    unsigned int      LoopCount;
    unsigned int      PollCount;
    unsigned int      OffsetDmaCmd;
    unsigned int      ElapsedTime_ms;
    void             *BarVa;

    unsigned int      written;
    unsigned int      writesize;
    unsigned long*    writeptr;
    int               sel;
	int cardid = 0;
	char* map = NULL;


#define PAGE_SIZE_JOON    ( getpagesize()<<3 ) //should be sync with driver
//#define PAGE_SIZE_JOON    ( getpagesize()<<2 ) //should be sync with driver
//#define PAGE_SIZE_JOON    ( getpagesize()<<0 ) //should be sync with driver
//#define PAGE_SIZE_JOON    ( 1<<16 ) //should be sync with driver

    //unsigned int SIZEOF_DMA_BUFFER = 0x40000000; //1G
    //unsigned int SIZEOF_DMA_BUFFER = 0x10000000; //256M
    //unsigned int SIZEOF_DMA_BUFFER = 0x200000; //2M
    unsigned int SIZEOF_DMA_BUFFER = PAGE_SIZE_JOON * 16;
    //unsigned int SIZEOF_DMA_BUFFER = PAGE_SIZE_JOON * 32; //1M
    const unsigned int SIZEOF_DESCR_UNIT = 0x14;

    while( ( sel = getopt( argc, argv, "s:w:t:b:ch" ) ) != -1 )
    {
        switch( sel )
        {
/*
            case 'f' : // to be processed filename
                sEntryFlag.filename = true;
                strcpy( sEntryValue.filename, optarg );
                break;
*/
            case 's' : // size
                sEntryFlag.filesize = true;
                sEntryValue.filesize = atoi( optarg );
                sEntryValue.filesize <<= 20; //for unit MB to B
                break;
            case 't' : // time duration for capture
                sEntryFlag.dumpsec = true;
                sEntryValue.dumpsec = atoi( optarg );
//                sEntryValue.dumpsec *= 1000; //for unit msec
                break;
            case 'b' : // unit buffer size
                sEntryFlag.buffersizeMB = true;
                sEntryValue.buffersizeMB = atoi( optarg );
                sEntryValue.buffersizeMB <<= 20; //for unit MB to B
                break;
            case 'w' :
				sEntryFlag.bufferTimeoutSec = true;
                sEntryValue.bufferTimeoutSec = atoi( optarg );
                break;
            case 'c' :
				sEntryFlag.countonly = true;
                break;
            case 'h' :
                printUsage();
                return 0;
            default :
                printUsage();
                break;
        }
    } 

	//set default value to unspecified item
	if( !sEntryFlag.buffersizeMB )
		sEntryValue.buffersizeMB = SIZEOF_DMA_BUFFER;
	else
		sEntryValue.buffersizeMB -= sEntryValue.buffersizeMB % getpagesize();

	SIZEOF_DMA_BUFFER = sEntryValue.buffersizeMB;

	if( !sEntryFlag.bufferTimeoutSec )
		sEntryValue.bufferTimeoutSec = DMA_TIMEOUT_SEC;

	int chan=0; 
	unsigned long* dmauaddr = 0;
	int i=0;
	int count = 0;
	int transfersize = 0;
	int descrcount = 0;
	DmaDescr_t* descrp;

    printf(
        "\n\n"
        "\t\t        BSL DMA DUMP Application\n"
        "\t\t                    Version %s \n\n", VERSION );


	printEntryFlagValue();

    /************************************
    *         Select Device
    ************************************/
     fd = OPEN_DEVICE( cardid );
     BSL_CHECK_DEVICE( fd );

     ret = dommap( fd, &map );
     BSL_CHECK_RESULT( ret, ret );

	
    /************************************
    *         For DMA Resources
    ************************************/
    //BLOCK 
	void* pUserBuffer[1] = {0,}; 
	//posix_memalign( &pUserBuffer[0], getpagesize(), SIZEOF_DMA_BUFFER ); 
	posix_memalign( &pUserBuffer[0], PAGE_SIZE_JOON, SIZEOF_DMA_BUFFER ); 
	if( pUserBuffer[0] == NULL )
	{
		printf( "%s: Error for calloc\n", __FUNCTION__ );
		return -1;
	}
	memset( pUserBuffer[0], 0x00, SIZEOF_DMA_BUFFER );
	printf("pUserBuffer[0] %p\n", pUserBuffer[0] );


    //BUFFER 0
    unsigned long BusAddr[1] = {0,};
    DmaParams_t  dmaParams = {0,};
	DmaChannel = 0; 
	dmaParams.UserVa = (unsigned long long)pUserBuffer[0];
	dmaParams.BusAddr_Buff = 0;
	dmaParams.ByteCount = SIZEOF_DMA_BUFFER;


	ret = ioctl( fd, CMD_DMA_TRANSFER_UBUFFER, &dmaParams );
	if (ret < 0) {
		printf("Failed buffer allocation ioctl!!!\n");
		goto _Exit_App;
	}
	else
	{
		printf("Ok (%d KB), BusAddr_Buff = %lX, BusAddr_Desc = %lx\n", 
				(dmaParams.ByteCount >> 10), dmaParams.BusAddr_Buff, dmaParams.BusAddr_Desc );
		BusAddr[0] = dmaParams.BusAddr_Desc; 
	}

    /**************************************************************
     * For File
     *************************************************************/

	int   f_filename = 0; 
	const char* dirname = "caps";
	const char* def_filename = "bslcaps";
	const char* file_ext = "bcap";
	char filename[256] = {0,};
	char extfilename[256] = {0,};
	char user_filename[256] = {0,};
	FILE* pcapfp;
	int fileno = -1;

	if( access( dirname, R_OK ) != 0 )
	{
		mkdir( dirname, 0755 );
		printf("Directory %s has been created....\n", dirname );
	}

	if( f_filename ) 
	{
		strcpy( filename, user_filename );
	}
	else
	{
		strcpy( filename, def_filename );
	}

	sprintf(extfilename, "%s/%s.%d.%s", dirname, filename, time(NULL), file_ext );
	printf("--> Captured Filename = %s\n", extfilename );

	if( 1 )
	{
		int flags = O_RDWR | O_CREAT | O_TRUNC | O_LARGEFILE | O_DIRECT;

		fileno = open( extfilename, flags, 0667 );
		if( ( fileno == -1 ) )
		{
			printf("%s: open error - %s\n",__FUNCTION__, extfilename );
			return 0;
		}
	}


	//*
	//* INSERT pthread function
	//*


/**
 * Now DMA Register Setting
 **/

	//1. DMA0 for listening
	
    writesize = SIZEOF_DMA_BUFFER;

	LoopCount = 0;

	sEntryValue.c_dumpsizeB = 0;

    int index_count = 0;
	unsigned long long cxar = 0;
	unsigned long long cxsr = 0;
	unsigned long long ccfr = 0;
	int portid = 1;

	cxar = dmaParams.BusAddr_Desc;
	cxsr = dmaParams.ByteCount | ( 0ll << 33 ) | 
		( 1ll << 36 ) | ( 1ll << 34 ); //36 : SGL, 34 : Run, 33 : RAM mode
	ccfr = 0x0F;

	printf("WRITE - c3ar %llx\n", cxar );
	printf("WRITE - c3sr %llx\n", cxsr );
	WRITE64( map, C3AR, cxar );
	WRITE64( map, C3SR, cxsr );

//	sleep(1);
//	printf("WRITE - ccfr %llx\n", ccfr );
//	WRITE64( map, OFFSET_REGISTER_PORT(portid)+CCFR, ccfr );
    sEntryValue.startsec = time(NULL);

	//Waiting ...
	//
	//
	//
	//

	i = 0;
	do {
		cxar = READ64( map, C3AR );
		cxsr = READ64( map, C3SR );
		printf("C3SR %016llx C3AR %016llx (count %d)\n", cxsr, cxar, i++ );
		if( cxsr & ( 1ll << 34 ) ) sleep(1);
		else break;
	} while(1);

	WRITE64( map, OFFSET_REGISTER_PORT(portid)+CCFR, 0ll );

	printf("%s: Press any key to continue.....\n", __func__ );
	getchar();

#if 1
	if( sEntryFlag.countonly == false )
	{
		written = write( fileno, pUserBuffer[0], writesize );
		if( written != writesize )
		{
			printf("%s:%d written %d is different from writesize=%d\n", __FUNCTION__, __LINE__, written, writesize );
			goto _Exit_App;
		}
	}
#endif

#if 0
	if( checkQuitTime() == true )
	{
		printf("%s:%d Elapsed Time %d seconds.\n", \
				__FUNCTION__, __LINE__, time(NULL) - sEntryValue.startsec );
		writeRemains( pDevice, fileno, index_count, pUserBuffer[index_count] );
		break;
	}
	else if( checkTransferSize() == true )
	{
		printf("%s:%d Transferred File Size %d Bytes.\n", \
				__FUNCTION__, __LINE__, sEntryValue.c_dumpsizeB );
		break;
	}
#endif


#if 0 //print contents
		while(0)
		{
			printf("BUFFER CONTENT FOR DMA SGL ------------------");
			static int count=0;
			if(count++ < 3 )
			{
				unsigned long* intp = pUserBuffer; //( unsigned long* )pciBuffer.UserAddr;
//					for( i=0; i<SIZEOF_DMA_PAGE*2/8; i++ )
				for( i=0; i<0x100/*pciBuffer.Size>>3*/; i++ )
				{
//						unsigned long* intp = ( unsigned long* )writeptr;
					if( i%2 == 0 ) printf("\n[%llX] ", intp+i);
					printf(" %016llX", *( intp + i ) );
				}
			}
		}
#endif

_Exit_App:
/* Now Release Memory */
	if( fileno != -1 ) close( fileno );

//	ret = ioctl( fd, CMD_DMA_CHANNEL_CLOSE, &dmaParams );
	free( pUserBuffer[0] );

    close(fd);
}

static void printEntryFlagValue( void )
{
	printf("\n%s: %d ------------------------ \n", __FUNCTION__, __LINE__ );

//	printf("   filename = %s, %s\n", sEntryFlag.filename == true ? "TRUE" : "FALSE", sEntryValue.filename );
	printf("   filesize = %s, %ld\n", sEntryFlag.filesize == true ? "TRUE" : "FALSE", sEntryValue.filesize );
	printf("   capture sec = %s, %ld\n", sEntryFlag.dumpsec == true ? "TRUE" : "FALSE", sEntryValue.dumpsec );
	printf("   buffer timeout = %s, %ld\n", sEntryFlag.bufferTimeoutSec == true ? "TRUE" : "FALSE", sEntryValue.bufferTimeoutSec );
	printf("   unit buffer size = %s, %ld\n", sEntryFlag.buffersizeMB == true ? "TRUE" : "FALSE", sEntryValue.buffersizeMB );
	printf("   countonly = %s\n", sEntryFlag.countonly == true ? "TRUE" : "FALSE" );
}

static boolean checkQuitTime( void )
{
	if( sEntryFlag.dumpsec == false ) return false; //infinite
	if( sEntryValue.dumpsec < time(NULL) - sEntryValue.startsec ) return true;

	return false;
}

#if 0
static boolean checkTransferSize( void )
{
	if( sEntryFlag.filesize == false ) return false; //infinite
	if( sEntryValue.filesize < sEntryValue.c_dumpsizeB ) return true;

	return false;
}

static void writeRemains( PLX_DEVICE_OBJECT* pDevice, int fileno, int index_count, void* pUserBuffer )
{
	U64 TxBytesRead;
	U64 written;
	int dma_index = index_count+1;
	MQ_TransferredBytesRead( pDevice, dma_index, &TxBytesRead );

	fprintf( stderr, "%s: TxBytesRead = %ld\n", __FUNCTION__, TxBytesRead );

    if( sEntryFlag.countonly == true ) return;

	int flags;
	fcntl( fileno, F_GETFL, &flags );
	flags &= ~O_DIRECT;
	fcntl( fileno, F_SETFL, flags );

	written = write( fileno, pUserBuffer, TxBytesRead );
	if( written != TxBytesRead )
	{
		printf("%s:%d written %d is different from writesize=%d\n", __FUNCTION__, __LINE__, written, TxBytesRead );
		return;
	}
}
#endif

static EnumResultCode
dommap( int fd, char** map )
{
#ifdef _TARGET_
#ifndef FILESIZE
#define FILESIZE    0x10000
#endif
	*map = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	BSL_CHECK_EXP( ( map==MAP_FAILED ), ResultMmapFailure );
#endif

	return ResultSuccess;
}

static EnumResultCode
domunmap( int fd, char** map )
{
#ifdef _TARGET_
	int ret;
	ret = munmap( *map, FILESIZE );
	BSL_CHECK_EXP( ( ret<0 ), ResultMunmapFailure );

	*map = NULL;
#endif
	return ResultSuccess;
}



#if 0
//////////////////////////////////////////////////////////////////////////////////////
typedef struct dmadescr
{
	unsigned long* page;
	unsigned int pagesize;
	unsigned long* next;
} __attribute__((packed)) DmaDescr_t;

#define BSL_CHECK_IOCTL_GOTO(ret_ioctl, where) \
	if( ret_ioctl < 0 ) { \
		fprintf(stderr, "%s:%s:%d BSL_CHECK_IOCTL_GOTO ret_ioctl %d\n", \
					__FILE__, __func__, __LINE__, ret_ioctl ); \
		goto where; \
	}

#define BSL_CHECK_NULL_GOTO(p, where) \
	if( p == 0 ) { \
		fprintf(stderr, "%s:%s:%d BSL_CHECK_IOCTL_GOTO ret_ioctl %d\n", \
					__FILE__, __func__, __LINE__, p ); \
		goto where; \
	}

int main(void)
{
	int fd = 0;
	int cardid = 0;
	int ret = 0;
	int descrcount = 0;

	ioc_io_buffer_t pciBuffer = {0,};
	ioc_io_buffer_t descrBuffer = {0,};
	void* pciUserp = 0;
	void* descrUserp = 0;
	DmaDescr_t* descrp = 0;

	const unsigned int SIZEOF_DMA_BUFFER = 0x20000000; //512M
	const unsigned int SIZEOF_DMA_PAGE = 0x400000; //4M

	// 
	fd = OPEN_DEVICE( cardid );
	BSL_CHECK_DEVICE( fd );

	ret = dommap( fd, &map );
	BSL_CHECK_RESULT( ret, ret );

	//buffer allocation for PCI memory
	pciBuffer.Size = SIZEOF_DMA_BUFFER;
	ret = ioctl( fd, CMD_DMA_MEM_ALLOC, &pciBuffer );
	BSL_CHECK_IOCTL_GOTO( ret, close_fd );

	//buffer mapping to user space for PCI memory
	pciUserp = mmap(0, pciBuffer.Size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, pciBuffer.CpuPhysical);
	BSL_CHECK_NULL_GOTO( pciUserp, close_fd );
	memset( pciUserp, 0, pciBuffer.Size );

	printf("UAddr(%llx) PAddr(%llx) VAddr(%llx) Size(%d)\n",
		pciUserp,
		pciBuffer.PhysicalAddr,
		pciBuffer.CpuPhysical,
		pciBuffer.Size );


	printf("[[[[Debug]]]]] sizeof(DmaDescr_t) = %d\n", sizeof(DmaDescr_t) );
	descrcount = pciBuffer.Size / SIZEOF_DMA_PAGE;
	descrBuffer.Size = descrcount * sizeof( DmaDescr_t );

	//buffer allocation for descriptor memory
	ret = ioctl( fd, CMD_DMA_MEM_ALLOC, &descrBuffer );
	BSL_CHECK_IOCTL_GOTO( ret, close_fd );

	//buffer mapping to user space for PCI memory
	descrUserp = mmap(0, descrBuffer.Size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, descrBuffer.CpuPhysical);
	BSL_CHECK_NULL_GOTO( descrUserp, close_fd );
	memset( descrUserp, 0, descrBuffer.Size );

	printf("UAddr(%llx) PAddr(%llx) VAddr(%llx) Size(%d)\n",
		descrUserp,
		descrBuffer.PhysicalAddr,
		descrBuffer.CpuPhysical,
		descrBuffer.Size );

	descrp = (DmaDescr_t*)descrUserp;

	for( i=0; i<descrcount; i++, descrp++ )
	{
		descrp->page = pciBuffer.PhysicalAddr + SIZEOF_DMA_PAGE*i;
		descrp->pagesize = SIZEOF_DMA_PAGE;
		descrp->next = descrBuffer.PhysicalAddr + ( sizeof( DmaDescr_t ) * ( i + 1 ) );
	}
	descrp->next = descrBuffer.PhysicalAddr; // Last Descriptor // don't care

	/*
	 * Now Register Setting
	 */

	//Abort DMA
//	WRITE64( 0, C0SR, 1ULL<<I_CXSR_ABORT );

	WRITE64( map, C0AR, descrBuffer.PhysicalAddr );
	unsigned long long cxsr = 0ULL;
	cxsr = SIZEOF_DMA_BUFFER;
	cxsr |= ( 1ULL << I_CXSR_RUN ) | ( 1ULL<<I_CXSR_SGMODE );

	WRITE64( map, C0SR, SIZEOF_DMA_BUFFER );

	while(1) {
		int key = 0;
		printf("\nPRESS ANY Key ( For quit 'q' ) ------------------\n");
		fflush(stdout);

		key = Plx_getch();
		if( key == 'q' ) break;

		printf("C0SR : %llx\n", READ64( map, C0SR ) );
	}

	ioctl( fd, CMD_DMA_MEM_FREE, &descrBuffer );
	ioctl( fd, CMD_DMA_MEM_FREE, &pciBuffer );
	munmap( descrUserp, descrBuffer.Size );
	munmap( pciUserp, pciBuffer.Size );

close_fd:
	domunmap( fd, &map );
	close( fd );

}
#endif //if 0
