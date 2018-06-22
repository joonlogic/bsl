/********************************************************************
 *  FILE   : bsl_device.c
 *  Author : joon
 *
 *  Library : 
 *  Export Function :
 *
 ********************************************************************
 *                    Change Log
 *
 *    Date          Author          Note
 *  -----------   -----------  ----------------------------------   
 *  2013.11.13       joon        This File is written first.
 *                                       
 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <sys/timeb.h>
#include <sys/mman.h>

#include "bsl_def.h"
#include "bsl_type.h"
#include "bsl_dbg.h"
#include "bsl_system.h"
#include "bsl_proto.h"
#include "bsl_msgif.h"
#include "bsl_register.h"
#include "bsl_ctl.h"

#define FILESIZE    0x10000

#ifdef _TARGET_
#define READ32(base, offset)            (*(unsigned int *)(base+(offset)))
#define WRITE32(base, offset, value)    (*(unsigned int *)(base+(offset))=value)
/*
#define READ64(base, offset)            (*(unsigned long long*)(base+(offset)))
#define WRITE64(base, offset, value)    (*(unsigned long long*)(base+(offset))=value)
*/
#define READ64(base, offset)            (*(unsigned long long*)(base+((offset)<<3)))
#define WRITE64(base, offset, value)    (*(unsigned long long*)(base+((offset)<<3))=value, usleep(1000))
#else //_TARGET_
#define READ32(base, offset)            0
#define WRITE32(base, offset, value)    
#define READ64(base, offset)            0ll
#define WRITE64(base, offset, value)    
#endif //_TARGET_

//static functions
static unsigned int getFsizeRandom( int min, int max, int mod );
static unsigned short getPayloadRandom( void );
static unsigned long get_gapvalue( unsigned int ipart, unsigned int fpart );
static unsigned int getTwosComplement( unsigned short first, unsigned short second );
static EnumResultCode bsl_device_delStream( int fd, int portid, int streamid );
#if 0 // unused
static unsigned long long getIFGFromRateSpec( T_Stream* stream, T_Frame*, int* fsize );
#endif
static double get_floatvalue( unsigned int ipart, unsigned int fpart );
#ifdef _VTSS_
unsigned short read_mdio_data(char* map, unsigned char phyaddr, unsigned char devaddr, unsigned short regaddr);
#else
unsigned short read_mdio_data(int fd, unsigned char phyaddr, unsigned char devaddr, unsigned short regaddr);
#endif
static unsigned short get_ip4_checksum( T_Protocol* proto, int* isoverride );
static unsigned short get_l4_checksum( T_Protocol* proto, int* isoverride );
static unsigned short calc_checksum( void* hp, int hlen );
void my_msleep(unsigned int msec);
static void stop_dma( char* map, int cardid, int portid );
static int start_dma( int fd, char* map, int cardid, int portid, unsigned int size, EnumCaptureMode ); 
static void* dma_thread( void* arg );
static T_PDR_Ip4* get_ip4_pdr( T_Protocol* proto );
static unsigned short getLastIp4ID( T_CustomIntegerTuple* tuple );								// 2017.2.27 by dgjung
static unsigned short getLastTcpUdpPort( T_CustomIntegerTuple* tuple );
extern int bsl_toPcap( char* filename, int cardid, int portid );
extern void toPacketTcp( T_Protocol* proto, T_Tcp* hp );

//dommap and domunmap is used as direct accessing the registers on user mode.
EnumResultCode 
dommap( int fd, char** map )
{
#ifdef _TARGET_
	*map = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	BSL_CHECK_EXP( ( map==MAP_FAILED ), ResultMmapFailure );
#endif
	
	return ResultSuccess;
}

EnumResultCode 
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

EnumResultCode
bsl_device_setPortMode( int fd, int portid, EnumPortOpMode mode )
{
	char* map = NULL;
	int ret;

	BSL_DEV(("%s: Enter. portid %d mode %d\n", __func__, portid, mode ));

	ret = dommap( fd, &map );
	BSL_CHECK_RESULT( ret, ret );

	WRITE64( map, OFFSET_REGISTER_PORT(portid)+PMSR, mode );

	domunmap( fd, &map );
	return ResultSuccess;
}
	
EnumResultCode
bsl_device_setPortActive( int fd, int portid, EnumPortActive enable )
{
	char* map = NULL;
	int ret;

	BSL_DEV(("%s: Enter. portid %d enable %d\n", __func__, portid, enable ));

	ret = dommap( fd, &map );
	BSL_CHECK_RESULT( ret, ret );

	enable != enable; //20150404 register map 1.5.1
	WRITE64( map, OFFSET_REGISTER_PORT(portid)+PECR, enable );

	BSL_DEV(("%s: OFFSET_REGISTER_PORT(portid)+PECR (%X) = %016llX\n", __func__, OFFSET_REGISTER_PORT(portid)+PECR, enable ));

	domunmap( fd, &map );
	return ResultSuccess;
}

EnumResultCode
//bsl_device_setLatency( int fd, int portid, int latency, int sequence )
bsl_device_setLatency( int fd, int portid, int latency, int sequence, int signature	)					// 2016.12.31 by dgjung
{
	char* map = NULL;
	int ret;
	unsigned long long pmsr = 0;

//	BSL_DEV(("%s: Enter. portid %d latency %d sequence %d\n", __func__, portid, latency, sequence ));
	BSL_DEV(("%s: Enter. portid %d latency %d sequence %d signature %d\n", __func__, portid, latency, sequence, signature ));			// 2016.12.31 by dgjung

	ret = dommap( fd, &map );
	BSL_CHECK_RESULT( ret, ret );

	pmsr = READ64( map, OFFSET_REGISTER_PORT(portid)+PMSR );
	if( latency ) pmsr |= ( 1 << 1 );
	else pmsr &= ~( 1 << 1 );

	if( sequence ) pmsr |= ( 1 << 2 );
	else pmsr &= ~( 1 << 2 );

	if( signature ) pmsr |= ( 1 << 3 );												// 2016.12.31 by dgjung
	else pmsr &= ~( 1 << 3 );

	WRITE64( map, OFFSET_REGISTER_PORT(portid)+PMSR, pmsr );

	domunmap( fd, &map );
	return ResultSuccess;
}

unsigned int getIntPartFrom( unsigned long long bunza, unsigned long long bunmo )
{
	long double value;

	if( bunmo == 0 ) return 0;

	value = bunza/bunmo;

	return (unsigned int)value; 
}

unsigned int getFracPartFrom( unsigned long long bunza, unsigned long long bunmo )
{
	long double value;
	unsigned long intpart;
	long double fracpart;
	unsigned int retval;

	if( bunmo == 0 ) return 0;

	value = bunza/bunmo;
	intpart = (unsigned long)value;
	fracpart = value - (long double)intpart;
	
	retval = fracpart*1000000000.;

	return retval;
}

EnumResultCode
bsl_device_getStats( int fd, T_LinkStats* stat )
{
	char* map = NULL;
	int ret;

	BSL_DEV(("%s: Enter. portid %d \n", __func__, stat->portid ));

	ret = dommap( fd, &map );
	BSL_CHECK_RESULT( ret, ret );

	stat->framesSent = READ64( map, stat->portid ? FSCR1 : FSCR0 );
	stat->framesSentRate = READ64( map, stat->portid ? FSRR1 : FSRR0 );
	stat->byteSent = READ64( map, stat->portid ? BSCR1 : BSCR0 );
	stat->bytesSentRate = READ64( map, stat->portid ? BSRR1 : BSRR0 );
	stat->validFrameReceived = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+RFCR );
	stat->validFramesRxRate = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+RFRR );
	stat->crcErrors = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+CEFC );
	stat->fragments = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+FEFC );

	stat->validByteReceived = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+RBCR );
	stat->bytesRxRate = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+RBRR );
	stat->fragmentsByteReceived = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+FEBC );
	stat->vlanTaggedFrames = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+VFCR );
	stat->crcErrorByteReceived = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+CEBC );
	stat->crcErrorFrameRxRate = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+CEFR );
	stat->fragmentErrorFrameRxRate = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+FEFR );
	stat->crcErrorByteRxRate = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+CEBR );
	stat->fragmentErrorByteRxRate = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+FEBR );
	stat->vlanByteReceived = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+VBCR );

	unsigned long long mmlr = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+MMLR );
	stat->latencyMaximum = mmlr >> 32;
	stat->latencyMinimum = (unsigned int)mmlr;
	unsigned long long totalLatencyValue = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+TLVR );
	unsigned long long totalPacketCount = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+TSCR );
//	stat->totalLatencyValue = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+TLVR );
//	stat->totalPacketCount = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+TSCR );
	stat->seqErrorPacketCount = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+SPCR );
	stat->undersizeFrameCount = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+UFCR );
	stat->oversizeFrameCount = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+OFCR );
	stat->signatureFrameCount = READ64( map, OFFSET_REGISTER_PORT(stat->portid)+SFCR );							// 2016.12.29 by dgjung

	stat->AverageLatIntPart = getIntPartFrom( totalLatencyValue, totalPacketCount );
	stat->AverageLatFracPart = getFracPartFrom( totalLatencyValue, totalPacketCount );

#ifndef _TARGET_
	int* statp = (int*)stat;
	int i;
	for( i=0; i<sizeof(T_LinkStats)/sizeof(int); i++ ) {
		*(statp+i) = i*10 + i%10; 
	}
#endif

	domunmap( fd, &map );
	return ResultSuccess;
}

char* capmap[SIZE_MAX_CARD][SIZE_MAX_PORT] = {NULL,};

EnumResultCode 
bsl_device_setCapture( 
		int fd, 
		int cardid, 
		int portid, 
		EnumCaptureMode mode,
		unsigned int size,
		int start )
{
	int ret = 0;

	BSL_DEV(("%s: Enter. cardid %d portid %d mode %d size %d start %d\n", \
				__func__, cardid, portid, mode, size, start ));

	ret = dommap( fd, &capmap[cardid][portid] );
	BSL_CHECK_RESULT( ret, ret );


	if( start == 0 ) {
		stop_dma( capmap[cardid][portid], cardid, portid );
	}
	else {
		ret = start_dma( fd, capmap[cardid][portid], cardid, portid, size, mode );
	}

	if( start == 0 ) {
		domunmap( fd, &capmap[cardid][portid] );
		close(fd);
	}

	return ret==0 ? ResultSuccess : ResultDmaError;
}

EnumResultCode 
bsl_device_setControlCommand( 
		int fd, 
		int portsel, 
		int portid, 
		int streamid,
		EnumCommand command, 
		unsigned long long clocks,
		unsigned int netmode,
		unsigned long long mac,
		unsigned int ip )
{
	char* map = NULL;
	int ret;
	unsigned long long wmcr = 0ll;

	BSL_DEV(("%s: Enter. portid %d command %d clocks %llu\n" \
				"netmode %d, mac %016llX, ip %08X", \
				__func__, portid, command, clocks, netmode, mac, ip ));

	ret = dommap( fd, &map );
	BSL_CHECK_RESULT( ret, ret );

	if( \
		( command == CommandStart ) || 
		( command == CommandStop ) || 
		( command == CommandPause ) || 
		( command == CommandSingleStep ) ) {

		wmcr = READ64( map, OFFSET_REGISTER_PORT(portid)+WMCR );

		if( portsel == 0 ) {
			wmcr &= ~(1ll << 2);
			WRITE64( map, OFFSET_REGISTER_PORT(portid)+WMCR, wmcr );
		}
		else {
			wmcr |= 1ll << 2;
			WRITE64( map, OFFSET_REGISTER_PORT(portid)+WMCR, wmcr );
		}
	}

	if( command == CommandStart ) {
		//regsec = (unsigned long long)timesec * BSL_HZ;
//		WRITE64( map, OFFSET_REGISTER_PORT(portid)+TIMR, clocks );
		WRITE64( map, OFFSET_REGISTER_PORT(portid)+CNTR, ValueCNTRStart );
//		sleep( 1 );
//		WRITE64( map, OFFSET_REGISTER_PORT(portid)+TIMR, 0 );
	}
	else if( command == CommandStop ) 
		WRITE64( map, OFFSET_REGISTER_PORT(portid)+CNTR, ValueCNTRStop );
	else if( command == CommandPause ) 
		WRITE64( map, OFFSET_REGISTER_PORT(portid)+CNTR, ValueCNTRPause );
	else if( command == CommandSingleStep ) 
		WRITE64( map, OFFSET_REGISTER_PORT(portid)+CNTR, ValueCNTRSingleStep );
	else if( command == CommandDeleteStream ) 
		bsl_device_delStream( fd, portid, streamid ); 
	else if( command == CommandResetStats ) { // independent of portid
#define WAR_STATS //20140727 reset until all clear
#ifdef WAR_STATS
		if( ( READ64( map, CNTR ) & ValueCNTRStart ) ||
			( READ64( map, OFFSET_REGISTER_PORT(1)+CNTR ) & ValueCNTRStart ) ) {
			WRITE64( map, SRCR, 1 );
			my_msleep( 1000 );
			WRITE64( map, SRCR, 0 );
		}
		else { 
			int limit = 10;
			do {
				WRITE64( map, SRCR, 1 );
				my_msleep( 1000 );
				WRITE64( map, SRCR, 0 );
				if( limit-- == 0 ) {
					printf("%s: limit 0 reached\n", __func__, limit );
					break;
				}
			} while( READ64( map, FSCR0 ) || READ64( map, FSCR1 ) );
		}
#else 
		WRITE64( map, SRCR, 1 );
		my_msleep( 1000 );
		WRITE64( map, SRCR, 0 );
#endif
	} 
	else if( command == CommandNetworkMode ) {
		WRITE64( map, OFFSET_REGISTER_PORT(portid)+NMSR, netmode );
		WRITE64( map, OFFSET_REGISTER_PORT(portid)+MASR, mac );
		WRITE64( map, OFFSET_REGISTER_PORT(portid)+IPSR, ip );
		BSL_DEV(("%s: NMSR  %016llX\n", __func__, (unsigned long long)netmode ));
		BSL_DEV(("%s: MASR  %016llX\n", __func__, (unsigned long long)mac ));
		BSL_DEV(("%s: IPSR  %016llX\n", __func__, (unsigned long long)ip ));
	}
	else
		fprintf( stderr, "%s: Never applied command %d\n", __func__, command );

	domunmap( fd, &map );
	return ResultSuccess;
}

EnumResultCode
bsl_device_delStream(
		int fd, 
		int portid, 
		int streamid )
{
	//TODO: implement!!
	return ResultSuccess;
}

EnumResultCode 
bsl_device_getLinkStatus( 
		int fd, 
		int* linkstatus )
{
	char* map = NULL;
	int ret;
	int port;
	unsigned short value;
#define PHY_LINK_STATUS_BIT 4

	BSL_DEV(("%s: Enter. \n", __func__ ));

	ret = dommap( fd, &map );
	BSL_CHECK_RESULT( ret, ret );

#if 0 //valid only for HTG
    for (port = 0; port < 2; port++) {
		value = read_mdio_data(fd, port, 1, 1);
		if (value & PHY_LINK_STATUS_BIT) {
			*linkstatus |= (1 << port);
		}
	}
#else //valid for BSC
	*linkstatus = ( ( READ64( map, LSR0 ) & 1ll ) << 0 ) | 
			( ( READ64( map, LSR1 ) & 1ll ) << 1 );
	BSL_DEV(("%s: *linkstatus  0x%08X\n", __func__, *linkstatus ));
#endif

	domunmap( fd, &map );

	return ResultSuccess;
}


EnumResultCode 
bsl_device_enableStream( 
		int fd, 
		int portid, 
		int streamid, 
		int enable )
{
	char* map = NULL;
	int ret;
	unsigned long long regval = 0l;
	unsigned int regaddr = SECR;

	BSL_DEV(("%s: Enter. portid %d streamid %d enable %d\n", \
				__func__, portid, streamid, enable ));

	ret = dommap( fd, &map );
	BSL_CHECK_RESULT( ret, ret );

	map +=  OFFSET_REGISTER_PORT64( portid );

	if( streamid == 0 ) { //all clear
		WRITE64( map, SECR, 0ll );
		WRITE64( map, SECR+2, 0ll );
		WRITE64( map, SECR+4, 0ll );
		WRITE64( map, SECR+6, 0ll );
		BSL_DEV(("%s: SECRs all cleared  %016llX\n", __func__, 0ll ));
	}

	regaddr += ( (streamid/64)*2 );
	streamid %= 64;

	regval = READ64( map, regaddr );
	regval = enable == 1 ? regval | ( 1ll << streamid ) : regval & ~( 1ll << streamid );

	WRITE64( map, regaddr, regval );
	BSL_DEV(("%s: SECR%d  %016llX\n", __func__, (regaddr-SECR)/2, regval ));

	map -=  OFFSET_REGISTER_PORT64( portid );
	domunmap( fd, &map );

	return ResultSuccess;
}

#if 0  //get from GUI
static unsigned long long getIFGFromRateSpec( T_Stream* stream, T_Frame* finfo, int* fsize )
{
	double fsize_avg = 0.;
	unsigned long long ifg = 0;
	double rateval = 0.;
	int i = 0;

	rateval = get_floatvalue( stream->control.rateControlIntPart, stream->control.rateControlFracPart );

	if( finfo->framesize.fsizeSpec == FrameSizeFixed )
		fsize_avg = finfo->framesize.sizeOrStep;
	else if( finfo->framesize.fsizeSpec == FrameSizeRandom ) {
		for( i=0; i<NUM_FRAMESIZE_RANDOM; i++ ) {
			fsize_avg += fsize[i];
		}
		fsize_avg /= NUM_FRAMESIZE_RANDOM;
	}
	else if( finfo->framesize.fsizeSpec == FrameSizeIncrement ) {
		fsize_avg = ( finfo->framesize.fsizeMin + finfo->framesize.fsizeMax ) / 2;
	}
	else {
		BSL_ERROR(("%s: Undefined FrameSizeSpec %d\n", 
					__func__, finfo->framesize.fsizeSpec ));
		fsize_avg = ( SIZE_FRAME_MIN + SIZE_FRAME_MAX ) / 2;
	}

	if( stream->control.rateControl == RateControlPercent ) {
		ifg = ( ( 100. * ( fsize_avg + 20. ) ) / rateval ) - 8. - fsize_avg;
	}
	else if( stream->control.rateControl == RateControlPacketPerSec ) {
		ifg = ( 10000000000ULL / ( 8 * rateval ) ) - fsize_avg - 8;
	}
	else if( stream->control.rateControl == RateControlBitPerSec ) {
		ifg = ( ( 10 * ( fsize_avg + 20 ) ) / rateval ) - 8 - fsize_avg;
	}
	else {
		BSL_ERROR(("%s: Undefined Rate Control %d\n", 
					__func__, stream->control.rateControl ));
		ifg = 0;
	}

	ifg += SIZE_PREAMBLE;
	return ifg;
}
#endif

//TODO: Check the distribution
static unsigned int getFsizeRandom( int min, int max, int mod )
{
	int base_random = rand();
	int range, remainder, bucket;
	unsigned int retval;

	if( RAND_MAX == base_random ) return getFsizeRandom( min, max, mod );

	range = max - min;
	remainder = RAND_MAX % range;
	bucket = RAND_MAX / range;

	if( base_random < RAND_MAX - remainder ) {
		retval = min + base_random/bucket;
		return retval%4 == mod ? retval : getFsizeRandom( min, max, mod );
	}
	else
		return getFsizeRandom( min, max, mod );
}

static unsigned short getPayloadRandom( void )
{
	int base_random = rand();
	int max=(1<<16)-1;

	return base_random > max ? getPayloadRandom() : base_random;
}

static unsigned int getTwosComplement( unsigned short first, unsigned short second )
{
	return first <= second ? second - first : ( (int)second - (int)first ) & 0x0001FFFF;
}

static unsigned long get_gapvalue( unsigned int ipart, unsigned int fpart )
{
	unsigned long quotient = 0;
	double remainder = 0.;
	double fval = 0.;

	//1. float value
	fval = get_floatvalue( ipart, fpart );

	//2. divide by 6.4ns
	quotient = fval/6.4;
	remainder = fval/6.4 - (double)quotient;

	//3. round up
	return quotient = remainder < 0.5 ? quotient : quotient + 1;
}

static double get_floatvalue( unsigned int ipart, unsigned int fpart )
{
	double fval = 0.;

	fval = (double)fpart;

	do {
		fval /= 10.;
	} while( fval >= 1 );

	fval += (double)ipart;

	return fval;
}

static void get_lastmac( T_EtherAddrTuple* tuple, unsigned char* lastaddr )
{
	unsigned long long start, last;
	unsigned long long repeat;
	unsigned long long max = 1ull << 48; //6bytes

	start = htonll( *(unsigned long long*)( tuple->addr ) ) >> 16;
	repeat = tuple->repeatCount;


	int shift = 0;
	if( tuple->step != 0 )
		while( ! ( tuple->step & ( 1 << shift ) ) ) { shift++; }

	unsigned long long max_repeat =  max / ( 1ll << shift );

	BSL_TRACE(("%s:DEBUG max_repeat %ld input_repeat %d shift %d\n", 
				__func__, max_repeat, tuple->repeatCount, shift ));

	repeat = repeat > max_repeat ? max_repeat : repeat;

	last = tuple->mode == EtherAddrModeIncrement ? 
			start + ( (max-1ll) & ( tuple->step * ( repeat - 1ll ) ) ) :
			start > ( (max-1ll) & ( tuple->step * ( repeat - 1ll ) ) ) ?
			start - ( (max-1ll) & ( tuple->step * ( repeat - 1ll ) ) ) :
			start + max - ( (max-1ll) & ( tuple->step * ( repeat - 1ll ) ) ) ;
	last &= ( max - 1ll );
	last <<= 16ll;
	last = ntohll( last );
	memcpy( lastaddr, &last, SIZE_ETHER_ADDR ); 
}

/* for reference
{
    BSL_CHECK_NULL( tuple, 0 );
    BSL_CHECK_NULL( tuple->repeat, 0 );

	unsigned short bound = 0;
	unsigned short start = (unsigned short)tuple->value;
	unsigned long long jump = (unsigned long long)tuple->step * (unsigned long long)(tuple->repeat - 1);

	int shift = 0;
	if( tuple->step != 0 )
		while( ! ( tuple->step & ( 1 << shift ) ) ) { shift++; }

	int max_repeat = 65536 / ( 1 << shift );
	int repeat = tuple->repeat;

	BSL_TRACE(("%s:DEBUG max_repeat %d input_repeat %d shift %d\n", 
				__func__, max_repeat, tuple->repeat, shift ));

	repeat = tuple->repeat > max_repeat ? max_repeat : tuple->repeat;

	bound = isincre ? 
			start + (unsigned short)( tuple->step * ( repeat - 1 ) ) :
			start > (unsigned short)( tuple->step * ( repeat - 1 ) ) ?
			start - (unsigned short)( tuple->step * ( repeat - 1 ) ) :
			start + 65536 - (unsigned short)( tuple->step * ( repeat - 1 ) );

	return bound; 
}
*/

static unsigned int get_lastip4( T_Ip4AddrTuple* iptuple )
{
	int ishost = 0;
	int isnet = 0;
	int isdecre = 0;
	int shift = 0;

	ishost = ( iptuple->mode == IpAddrModeIncrementHost ) || 
			 ( iptuple->mode == IpAddrModeDecrementHost ) ? 1 : 0;
	isnet  = ( iptuple->mode == IpAddrModeIncrementNetwork ) || 
			 ( iptuple->mode == IpAddrModeDecrementNetwork ) ? 1 : 0;
	isdecre = ( iptuple->mode == IpAddrModeDecrementHost ) || 
			  ( iptuple->mode == IpAddrModeDecrementNetwork ) ? 1 : 0;

	if( ishost ) {
		if( isdecre ) {
			return ( (~iptuple->mask) & ( iptuple->addr - (iptuple->repeat-1) ) ) |
					 ( (iptuple->mask) & ( iptuple->addr )) ;
		}
		else {
			return ( (~iptuple->mask) & ( iptuple->addr + (iptuple->repeat-1) ) ) |
					 ( (iptuple->mask) & ( iptuple->addr )) ;
		}
	}
	else if( isnet ) {
		//should be class A or B or C.
		if( iptuple->mask != 0 )
			while( ! ( iptuple->mask & ( 1 << shift ) ) ) { shift++; }

		if( isdecre ) {
			return ( (iptuple->mask) & ( iptuple->addr - ( (iptuple->repeat-1) << shift ) ) ) |
					 ( (~iptuple->mask) & ( iptuple->addr ) ) ;
		}
		else {
			return ( (iptuple->mask) & ( iptuple->addr + ( (iptuple->repeat-1) << shift ) ) ) |
					 ( (~iptuple->mask) & ( iptuple->addr ) ) ;
		}
	}
	else {
		BSL_ERROR(("%s: No Last Address Ip4 valid\n", __func__ ));
	}

	return 0;
}

static unsigned int get_lastip4offset( T_Ip4AddrTuple* iptuple, unsigned int lastip4 )
{
	int ishost = 0;
	int isnet = 0;
	int isdecre = 0;
	int shift = 0;

	ishost = ( iptuple->mode == IpAddrModeIncrementHost ) || 
			 ( iptuple->mode == IpAddrModeDecrementHost ) ? 1 : 0;
	isnet  = ( iptuple->mode == IpAddrModeIncrementNetwork ) || 
			 ( iptuple->mode == IpAddrModeDecrementNetwork ) ? 1 : 0;
	isdecre = ( iptuple->mode == IpAddrModeDecrementHost ) || 
			  ( iptuple->mode == IpAddrModeDecrementNetwork ) ? 1 : 0;

	if( ishost ) {
		/*
		return isdecre ? iptuple->addr - lastip4 : lastip4 - iptuple->addr;
		*/
		return iptuple->addr > lastip4 ? iptuple->addr - lastip4 : lastip4 - iptuple->addr;
	}

	if( isnet ) {
		//should be class A or B or C.
		if( iptuple->mask != 0 )
			while( ! ( iptuple->mask & ( 1 << shift ) ) ) { shift++; }

		/*
		return isdecre ? 
		*/
		return iptuple->addr > lastip4 ?
				( iptuple->addr - lastip4 ) >> shift : 
				( lastip4 - iptuple->addr ) >> shift;
	}

	return 0;
}


static void get_lastip6( T_Ip6AddrTuple* iptuple, unsigned char* lastaddr )
{
	unsigned long long start_u, start_l;
	unsigned long long last_u, last_l; 
	unsigned long long delta;

	int ishost = 0;
	int isnet = 0;
	int isdecre = 0;
	int shift = 0;
	unsigned long long masking = 0ll;
	unsigned long long max = 0ll;
	unsigned long long max_repeat = 0ll;
	unsigned long long repeat = 0ll;
	unsigned long long start = 0ll;
	unsigned long long bound = 0ll;

	if( iptuple->mode == IpAddrModeFixed ) {
		memcpy( lastaddr, iptuple->addr, 16 );
		return; 
	}

	ishost = ( iptuple->mode == IpAddrModeIncrementHost ) || 
			 ( iptuple->mode == IpAddrModeDecrementHost ) ? 1 : 0;
	isnet  = ( iptuple->mode == IpAddrModeIncrementNetwork ) || 
			 ( iptuple->mode == IpAddrModeDecrementNetwork ) ? 1 : 0;
	isdecre = ( iptuple->mode == IpAddrModeDecrementHost ) || 
			  ( iptuple->mode == IpAddrModeDecrementNetwork ) ? 1 : 0;

	start_u = htonll( *(unsigned long long*)( iptuple->addr ) );
	start_l = htonll( *(unsigned long long*)( iptuple->addr + 8 ) );
	delta = ( iptuple->repeat > 0 ? iptuple->repeat-1 : 0 ) * iptuple->step;

	if( ishost ) {
#if 0  //change algorithm 150829
		masking = (~((1ll<<(128-iptuple->mask))-1ll));
		last_u = start_u; //at host mode just lower 32bits can be changed.(96~127)
		if( isdecre ) {
			last_l = ( (~masking) & ( start_l - (iptuple->repeat-1)*iptuple->step ) ) |
					 ( masking & ( start_l )) ;
		}
		else {
			last_l = ( (~masking) & ( start_l + (iptuple->repeat-1)*iptuple->step ) ) |
					 ( masking & ( start_l )) ;
		}
#else
		last_u = start_u; //at host mode just lower 32bits can be changed.(96~127)
		max = 1ll << ( 128ll - iptuple->mask );
		masking = (~((1ll<<(128-iptuple->mask))-1ll));
		if( iptuple->step != 0 )
			while( ! ( iptuple->step & ( 1 << shift ) ) ) { shift++; }
		max_repeat = max / ( 1ll << shift );
		repeat = iptuple->repeat > max_repeat ? max_repeat : iptuple->repeat;
		
		start = start_l & (~masking);
		bound = !isdecre ?
				start + ( (max-1ull) & ( iptuple->step * ( repeat - 1ull ) ) ) :
				start > ( (max-1ull) & ( iptuple->step * ( repeat - 1ull ) ) ) ?
				start - ( (max-1ull) & ( iptuple->step * ( repeat - 1ull ) ) ) :
				start + max - ( (max-1ull) & ( iptuple->step * ( repeat - 1ull ) ) ) ;
		bound &= ( max - 1ll );

		last_l = bound | ( start_l & masking );

#endif
	}
	else if( isnet ) {
		//TODO: Not Yet coding for max_repeat
		//for network mode 4byte section can be changed.
		if( iptuple->mask < 64 ) {
			last_l = start_l; 
			shift = 64-iptuple->mask;
			masking = (~((1ll<<(64-iptuple->mask))-1ll));

			if( isdecre ) {
				last_u = ( masking & ( start_u - (((iptuple->repeat-1)*iptuple->step) << shift) ) ) | ( (~masking) & ( start_u )) ;
			}
			else {
				last_u = ( masking & ( start_u + (((iptuple->repeat-1)*iptuple->step) << shift) ) ) | ( (~masking) & ( start_u )) ;
			}

			if( iptuple->mask >= 32 ) {
				last_u = ( 0xFFFFFFFF00000000ll & start_u ) | 
						 ( 0x00000000FFFFFFFFll & last_u );
			}
		}
		else {
			last_u = start_u; 
			shift = 128-iptuple->mask;
			masking = (~((1ll<<(128-iptuple->mask))-1ll));

			if( isdecre ) {
				last_l = ( masking & ( start_l - (((iptuple->repeat-1)*iptuple->step) << shift) ) ) | ( (~masking) & ( start_l )) ;
			}
			else {
				last_l = ( masking & ( start_l + (((iptuple->repeat-1)*iptuple->step) << shift) ) ) | ( (~masking) & ( start_l )) ;
			}

			if( iptuple->mask >= 96 ) {
				last_l = ( 0xFFFFFFFF00000000ll & start_l ) | 
						 ( 0x00000000FFFFFFFFll & last_l );
			}
		} 
	}
	else {
		BSL_ERROR(("%s: No Last Address Ip6 valid\n", __func__ ));
	}

	last_u = ntohll( last_u );
	last_l = ntohll( last_l );
	memcpy( lastaddr, &last_u, 8 ); 
	memcpy( lastaddr+8, &last_l, 8 ); 
}


static unsigned int get_lastip6offset( T_Ip6AddrTuple* iptuple, unsigned char* lastip6 )
{
	unsigned long long start_u, start_l;
	unsigned long long last_u, last_l; 
	unsigned long long delta;
	int ishost = 0;
	int isnet = 0;
	int isdecre = 0;
	int shift = 0;

	ishost = ( iptuple->mode == IpAddrModeIncrementHost ) || 
			 ( iptuple->mode == IpAddrModeDecrementHost ) ? 1 : 0;
	isnet  = ( iptuple->mode == IpAddrModeIncrementNetwork ) || 
			 ( iptuple->mode == IpAddrModeDecrementNetwork ) ? 1 : 0;
	isdecre = ( iptuple->mode == IpAddrModeDecrementHost ) || 
			  ( iptuple->mode == IpAddrModeDecrementNetwork ) ? 1 : 0;

	start_u = htonll( *(unsigned long long*)( iptuple->addr ) );
	start_l = htonll( *(unsigned long long*)( iptuple->addr + 8 ) );
	last_u = htonll( *(unsigned long long*)( lastip6 ) );
	last_l = htonll( *(unsigned long long*)( lastip6 + 8 ) );

	if( ishost ) {
		return isdecre ? start_l - last_l : last_l - start_l;
	}

	if( isnet ) {
		unsigned long long* startp;
		unsigned long long* lastp;

		if( iptuple->mask < 64 ) {
			startp = &start_u;
			lastp = &last_u;
			shift = 64 - iptuple->mask;
		}
		else {
			startp = &start_l;
			lastp = &last_l;
			shift = 128 - iptuple->mask;
		}

		/*
		return isdecre ? 
		*/
		return *startp > *lastp ?
				( *startp - *lastp ) >> shift : 
				( *lastp - *startp ) >> shift;
	}

	return 0;
}

static int get_ip6hlen( T_Protocol* proto )
{
	T_PDR_Ip6* ip6pdr;
	int hlen = 0;
	int ip6pdrlen = 0;
	BSL_CHECK_NULL( proto, 0 );

	if( proto->l3.protocol == ProtocolIP6 ) {
		ip6pdr = (T_PDR_Ip6*)proto->l3.pdr;
		ip6pdrlen = proto->l3.length;
	}
	else if( proto->l3.protocol == ProtocolIP4OverIP6 ) {
		T_PDR* ip6pdr_msgp;
		ip6pdr_msgp = (T_PDR*)proto->l3.pdr;
		ip6pdr = (T_PDR_Ip6*)ip6pdr_msgp->pinfo;
		ip6pdrlen = ip6pdr_msgp->length;
	}
	else if( proto->l3.protocol == ProtocolIP6OverIP4 ) {
		T_PDR* ip6pdr_msgp;
		T_PDR* ip4pdr_msgp;
		unsigned int ip4pdrlen;
		T_PDR_Ip4* ip4pdr;
		ip4pdr_msgp = (T_PDR*)proto->l3.pdr;
		ip4pdrlen = ip4pdr_msgp->length;
		ip4pdr = (T_PDR_Ip4*)ip6pdr_msgp->pinfo;
		ip6pdr_msgp = (T_PDR*)( ((void*)ip4pdr) + ip4pdrlen );
		ip6pdr = (T_PDR_Ip6*)ip6pdr_msgp->pinfo;
		ip6pdrlen = ip6pdr_msgp->length;
	}
	else 
		return 0;

	hlen = sizeof( T_Ip6 );
	if( ip6pdr->nextheader != IP6_NO_NEXT_HEADER ) {
		int length = sizeof( T_PDR_Ip6 );
		T_PDR_Ip6NextHeader* nhpdr = (T_PDR_Ip6NextHeader*)(ip6pdr+1);
		while( ip6pdrlen > length ) {
			hlen += nhpdr->length;
			length += nhpdr->length;
			nhpdr = ((void*)nhpdr) + nhpdr->length + sizeof(nhpdr->length);
		}
	}

	return hlen; 
}

static int get_ip4hlen( T_Protocol* proto )
{
	T_PDR_Ip4* ip4pdr;
	BSL_CHECK_NULL( proto, 0 );

	if( proto->l3.protocol == ProtocolIP4 ) {
		ip4pdr = (T_PDR_Ip4*)proto->l3.pdr;
	}
	else if( proto->l3.protocol == ProtocolIP6OverIP4 ) {
		T_PDR* ip4pdr_msgp;
		ip4pdr_msgp = (T_PDR*)proto->l3.pdr;
		ip4pdr = (T_PDR_Ip4*)ip4pdr_msgp->pinfo;
	}
	else if( proto->l3.protocol == ProtocolIP4OverIP6 ) {
		T_PDR* ip6pdr_msgp;
		T_PDR* ip4pdr_msgp;
		unsigned int ip6pdrlen;
		T_PDR_Ip6* ip6pdr;
		ip6pdr_msgp = (T_PDR*)proto->l3.pdr;
		ip6pdrlen = ip6pdr_msgp->length;
		ip6pdr = (T_PDR_Ip6*)ip6pdr_msgp->pinfo;
		ip4pdr_msgp = (T_PDR*)( ((void*)ip6pdr) + ip6pdrlen );
		ip4pdr = (T_PDR_Ip4*)ip4pdr_msgp->pinfo;
	}
	else 
		return 0;

	return ip4pdr->hlen * 4;
}

static unsigned long long get_distance_ip6sip( T_Protocol* proto )
{
#if 0 //revised
	int hlen = 0;
	hlen += proto->l2.protocol == ProtocolEthernet ? LENGTH_HEADER_BASE_ETHERNET : 0;
	hlen += 
		proto->l2tag.protocol == ProtocolUnused ? 0 :
		proto->l2tag.protocol == ProtocolISL ? sizeof(T_Isl) :
		proto->l2tag.protocol == ProtocolVLAN ? sizeof(T_802_1Q_VLANTag) :
		proto->l2tag.protocol == ProtocolMPLS ? 0 : //TODO: insert MPLS size
		0;
	hlen += proto->l3 == ProtocolIP6 ? offsetof( T_Ip6, sip ) :
		proto->l3.protocol == ProtocolIP4OverIP6 ? offsetof( T_Ip6, sip ) :
		proto->l3.protocol == ProtocolIP6OverIP4 ? get_ip4hlen( proto ) + offsetof( T_Ip6, sip ) : 
		0;
	return hlen;
#else
	return
		( proto->l3.protocol == ProtocolIP6 ) || 
		( proto->l3.protocol == ProtocolIP4OverIP6 ) ? 
		proto->l3.offset + offsetof( T_Ip6, sip ) :
		proto->l3.protocol == ProtocolIP6OverIP4 ? 
		proto->l3.offset + get_ip4hlen( proto ) + offsetof( T_Ip6, sip ) : 0;
#endif
}

static int get_distance_ip4chksum( T_Protocol* proto )
{
#if 0 //revised
	int hlen = 0;
	hlen += proto->l2.protocol == ProtocolEthernet ? LENGTH_HEADER_BASE_ETHERNET : 0;
	hlen += 
		proto->l2tag.protocol == ProtocolUnused ? 0 :
		proto->l2tag.protocol == ProtocolISL ? sizeof(T_Isl) :
		proto->l2tag.protocol == ProtocolVLAN ? sizeof(T_802_1Q_VLANTag) :
		proto->l2tag.protocol == ProtocolMPLS ? 0 : //TODO: insert MPLS size
		0;
	hlen += proto->l3 == ProtocolIP4 ? offsetof( T_Ip4, checksum ) :
		proto->l3.protocol == ProtocolIP6OverIP4 ? offsetof( T_Ip4, checksum ) :
		proto->l3.protocol == ProtocolIP4OverIP6 ? get_ip6hlen( proto ) + offsetof( T_Ip4, checksum ) : 
		0;
	return hlen;
#else
	return
		( proto->l3.protocol == ProtocolIP4 ) || 
		( proto->l3.protocol == ProtocolIP6OverIP4 ) ? 
		proto->l3.offset + offsetof( T_Ip4, checksum ) :
		proto->l3.protocol == ProtocolIP4OverIP6 ? 
		proto->l3.offset + get_ip6hlen( proto ) + offsetof( T_Ip4, checksum ) : 0;
#endif
}

static unsigned short get_ip4_checksum( T_Protocol* proto, int* isoverride )
{
	int distance = 0;
	T_Ip4* hp = NULL;
	void* shp = NULL;
	int hlen = 0;
	unsigned short chksum = 0;
	T_PDR_Ip4* pdr = NULL;
	BSL_CHECK_NULL( proto, 0 );

	pdr = get_ip4_pdr( proto );
	if( pdr->checksum.type == ChecksumOverride ) {
		*isoverride = TRUE;
		return pdr->checksum.value;
	}

	*isoverride = FALSE;

	//request by GUI 
	return pdr->checksum.value;

	//0. clear checksum field
	distance = get_distance_ip4chksum( proto );
	*(unsigned short*)(proto->header + distance ) = 0x0000;


	//1. header length
	shp = (void*)(proto->header + distance);
	hp = (T_Ip4*)container_of( shp, T_Ip4, checksum );
	BSL_CHECK_NULL( hp, 0 );
	hlen = hp->hlen * 4;

	//2. calc checksum
	chksum = calc_checksum( (void*)hp, hlen );

	if( pdr->checksum.type == ChecksumValid ) 
		return chksum;
	else
		return chksum + 0x0101;
}

static unsigned long long get_ip4_classSel( T_Ip4AddrTuple* tuple )
{
	/* Class A : 0xFF000000
	 * Class B : 0xFFFF0000
	 * Class C : 0xFFFFFF00
	 * No Mask : 0x00000000 */

	BSL_CHECK_NULL( tuple, 0 );

	return tuple->mask == 0xFF000000 ? ( 1ll << 3 ) :
		tuple->mask == 0xFFFF0000 ? ( 1ll << 2 ) :
		tuple->mask == 0xFFFFFF00 ? ( 1ll << 1 ) :
		tuple->mask == 0x00000000 ? 0 : 
		0;
}

/* unused
static unsigned long long get_ip4_classnum( T_Ip4AddrTuple* tuple )
{
	BSL_CHECK_NULL( tuple, 0 );

	return tuple->mask == 0xFF000000 ? 1ll << 3 :
		tuple->mask == 0xFFFF0000 ? ( 1 << 3 ) | ( 1 << 2 ) :
		tuple->mask == 0xFFFFFF00 ? ( 1 << 3 ) | ( 1 << 2 ) | ( 1 << 1 ) :
		tuple->mask == 0xFFFFFFFF ? ( 1 << 3 ) | ( 1 << 2 ) | ( 1 << 1 ) | ( 1 << 0 ) :
		0;
}
*/

T_PDR_Ip6* get_ip6_pdr( T_Protocol* proto )
{	
	T_PDR_Ip6* pdr = NULL;
	BSL_CHECK_NULL( proto, 0 );

	if( proto->l3.protocol == ProtocolIP6 ) {
		pdr = (T_PDR_Ip6*)proto->l3.pdr;
	}
	else if( proto->l3.protocol == ProtocolIP4OverIP6 ) {
		T_PDR* ip6pdr_msgp = (T_PDR*)proto->l3.pdr;
		pdr = (T_PDR_Ip6*)ip6pdr_msgp->pinfo;
	}
	else if( proto->l3.protocol == ProtocolIP6OverIP4 ){
		T_PDR* ip4pdr_msgp;
		T_PDR* ip6pdr_msgp;
		unsigned int ip4pdrlen;
		T_PDR_Ip4* ip4pdr;
		ip4pdr_msgp = (T_PDR*)(proto->l3.pdr);
		ip4pdrlen = ip4pdr_msgp->length;
		ip4pdr = (T_PDR_Ip4*)ip4pdr_msgp->pinfo;
		ip6pdr_msgp = (T_PDR*)( ((void*)ip4pdr) + ip4pdrlen );
		pdr = (T_PDR_Ip6*)ip6pdr_msgp->pinfo;
	} 

	return pdr;
}

T_PDR_Ip4* get_ip4_pdr( T_Protocol* proto )
{	
	T_PDR_Ip4* pdr = NULL;
	BSL_CHECK_NULL( proto, 0 );

	if( proto->l3.protocol == ProtocolIP4 ) {
		pdr = (T_PDR_Ip4*)(proto->l3.pdr);
	}
	else if( proto->l3.protocol == ProtocolIP6OverIP4 ) {
		T_PDR* ip4pdr_msgp = (T_PDR*)proto->l4.pdr;
		pdr = (T_PDR_Ip4*)ip4pdr_msgp->pinfo;
	}
	else if( proto->l3.protocol == ProtocolIP4OverIP6 ) {
		T_PDR* ip6pdr_msgp;
		T_PDR* ip4pdr_msgp;
		unsigned int ip6pdrlen;
		T_PDR_Ip6* ip6pdr;
		ip6pdr_msgp = (T_PDR*)(proto->l3.pdr);
		ip6pdrlen = ip6pdr_msgp->length;
		ip6pdr = (T_PDR_Ip6*)ip6pdr_msgp->pinfo;
		ip4pdr_msgp = (T_PDR*)( ((void*)ip6pdr) + ip6pdrlen );
		pdr = (T_PDR_Ip4*)ip4pdr_msgp->pinfo;
	}

	return pdr;
}

EnumResultCode 
bsl_device_setStreamDetail( 
		int fd, 
		int portid, 
		int streamid,
		int groupid,								// 2017.7.15 by dgjung
		T_Stream* stream,
		T_Protocol* proto )
{
	char* map = NULL;
	int ret;
	int i;
	int hlen;
	unsigned long long regval = 0l;
	unsigned long long pacr = 0l;
	unsigned long long dmsr = 0l;
	unsigned long long dmlr = 0l;
	unsigned long long smsr = 0l;
	unsigned long long smlr = 0l;
	unsigned long long sisr = 0l;
	unsigned long long disr = 0l;
	int limit = 60;
	int fsize[NUM_FRAMESIZE_RANDOM+4] = {0,};
	unsigned int lastip4 = 0;
	unsigned int lastoffset = 0;

//	BSL_DEV(("%s: Enter. portid %d streamid %d proto %p\n", \
//				__func__, portid, streamid, proto ));
	BSL_DEV(("%s: Enter. portid %d streamid %d groupid %d proto %p\n", \
				__func__, portid, streamid, groupid, proto ));					// 2017.7.15 by dgjung

	ret = dommap( fd, &map );
	BSL_CHECK_RESULT( ret, ret );

	map +=  OFFSET_REGISTER_PORT64( portid );
	
	//0. init
	WRITE64( map, DMLR, 0ll );
	WRITE64( map, SI6SR0, 0ll );
	WRITE64( map, SI4SR, 0ll );
	WRITE64( map, SPVR, 0ll );
	WRITE64( map, DPVR, 0ll );
	WRITE64( map, IDVR, 0ll );											// 2017.4.3 by dgjung

	BSL_DEV(("%s: DMLR  %016llX\n", __func__, 0ll ));
	BSL_DEV(("%s: SI6SR0  %016llX\n", __func__, 0ll ));
	BSL_DEV(("%s: SI4SR  %016llX\n", __func__, 0ll ));
	BSL_DEV(("%s: SPVR  %016llX\n", __func__, 0ll ));
	BSL_DEV(("%s: DPVR  %016llX\n", __func__, 0ll ));
	BSL_DEV(("%s: IDVR  %016llX\n", __func__, 0ll ));					// 2017.4.3 by dgjung

	//1. frame info
	//PPIR
	BSL_CHECK_TRUE( 
			proto->fi.framesize.fsizeSpec == FrameSizeFixed || 
			proto->fi.framesize.fsizeSpec == FrameSizeRandom || 
			proto->fi.framesize.fsizeSpec == FrameSizeIncrement, 
			ResultBadValue );
	regval = ( 
		(unsigned long long)( proto->fi.framesize.fsizeSpec == FrameSizeFixed ? 0 : 
		proto->fi.framesize.fsizeSpec == FrameSizeRandom ? 3 : 1 )) << 
		I_PPIR_FRAMESIZE_TYPE;
	regval |= ( 
		proto->fi.framesize.fsizeSpec == FrameSizeIncrement ? 4 : //since hardware requirement
		proto->fi.payloadType == FrameDataTypeIncByte ? 0 :
		proto->fi.payloadType == FrameDataTypeIncWord ? 1 :
		proto->fi.payloadType == FrameDataTypeDecByte ? 2 :
		proto->fi.payloadType == FrameDataTypeDecWord ? 3 :
		proto->fi.payloadType == FrameDataTypeRepeating ? 4 :
		proto->fi.payloadType == FrameDataTypeRandom ? 5 :									// 2017.3.13 by dgjung
		proto->fi.payloadType == FrameDataTypeDownload ? 6 :								// 2017.3.13 by dgjung
		proto->fi.payloadType == FrameDataTypeFixed ? 7 : 7 ) << 							// 2017.3.13 by dgjung
		I_PPIR_PAYLOAD_PATTERN_TYPE;
	if( proto->fi.payloadType == FrameDataTypeRepeating ) {
#if 0
		unsigned char* ppirp = (unsigned char*)&regval;
		ppirp += sizeof(unsigned long long)-1; //8th bit
		memcpy( ppirp-proto->fi.pattern.validSize, 
				proto->fi.pattern.payload, 
				proto->fi.pattern.validSize );
#else
		unsigned char patval[32] = {0,};
		unsigned char* pload = (unsigned char*)proto->fi.pattern.payload;
		for( i=0; i<proto->fi.pattern.validSize; i++ ) {
			patval[proto->fi.pattern.validSize-1-i] = *pload++;
		}

		regval |= ( (*(unsigned long long*)patval) << 8 );
#endif
		regval |= ((unsigned long long)( proto->fi.pattern.validSize & 
					M_PPIR_PATTERN_SIZE )) << I_PPIR_PATTERN_SIZE;
	}
	WRITE64( map, PPIR, regval );
	BSL_DEV(("%s: PPIR  %016llX\n", __func__, regval ));

	//PGCR, stream number, frame size
	T_FrameSize* frame = (T_FrameSize*)&proto->fi;
	regval = ( ( stream->streamid & M_PGCR_STREAM_NUMBER ) << I_PGCR_STREAM_NUMBER ) | ( 0 );

	regval |= ( ( stream->groupid & M_PGCR_GROUP_NUMBER ) << I_PGCR_GROUP_NUMBER ) | ( 0 );						// 2017.7.15 by dgjung

	regval |= 
		frame->fsizeSpec == FrameSizeFixed ?
		((unsigned long long)( frame->sizeOrStep & M_PGCR_FRAMESIZE )) << I_PGCR_FRAMESIZE : 
		frame->fsizeSpec == FrameSizeIncrement ?
		((unsigned long long)( frame->fsizeMin & M_PGCR_FRAMESIZE )) << I_PGCR_FRAMESIZE :
		frame->fsizeSpec == FrameSizeRandom ?
		((unsigned long long)( frame->fsizeValueRand[0] & M_PGCR_FRAMESIZE )) << I_PGCR_FRAMESIZE : 0;

	WRITE64( map, PGCR, regval );
	BSL_DEV(("%s: PGCR  %016llX\n", __func__, regval ));

#if 1 //140721 PDWR PDCR
    if( 
		( proto->fi.payloadType == FrameDataTypeFixed ) || 
		( proto->fi.payloadType == FrameDataTypeRandom ) || 
		( proto->fi.payloadType == FrameDataTypeDownload ) || 
		( proto->fi.framesize.fsizeSpec == FrameSizeRandom ) ) {
        BSL_DEV(("%s: Pload Fixed. validsize %d\n", __func__, proto->fi.pattern.validSize ));

        unsigned char patval[32] = {0,};
        unsigned char* pload = (unsigned char*)proto->fi.pattern.payload;
        int validsize = proto->fi.pattern.validSize;
        int pushsize = 0;
        do {
            memset( patval, 0x00, sizeof(patval));
            if( validsize > 7 ) pushsize = 8;
            else pushsize = validsize;

			/*
            for( i=0; i<pushsize; i++ ) {
                patval[pushsize-1-i] = *pload++;
            }
			*/
			if( pushsize == 8 ) {
				for( i=0; i<4; i++ ) patval[4+i] = *pload++;
				for( i=0; i<4; i++ ) patval[i] = *pload++;
			}
			else if( pushsize > 4 ) {
				for( i=0; i<4; i++ ) patval[4+i] = *pload++;
				for( i=8-pushsize; i<4; i++ ) patval[i] = *pload++;
			}
			else {
				for( i=4-pushsize; i<4; i++ ) patval[4+i] = *pload++;
			}

            validsize -= pushsize;

            regval = *(unsigned long long*)patval;
//			if( pushsize != 8 ) regval <<= ( (8 - pushsize)*8 ); 
            WRITE64( map, PDWR, regval );
			if( ( proto->fi.pattern.validSize > 1500 ) && ( proto->fi.pattern.validSize - validsize > 80 ));
			else
				BSL_DEV(("%s: PDWR  %016llX\n", __func__, regval ));

            if( validsize == 0 ) regval = 3ll;
            else                 regval = 1ll;

            WRITE64( map, PDCR, regval );
			if( ( proto->fi.pattern.validSize > 1500 ) && ( proto->fi.pattern.validSize - validsize > 80 ));
			else
				BSL_DEV(("%s: PDCR  %016llX\n", __func__, regval ));

			limit = 100;
            while( ( READ64( map, PDCR ) & ( 1<<0 ) ) && --limit ) {
                usleep( 1000 );
			}

            if( limit <= 0 ) {
                BSL_ERROR(("%s: PDCR  %016llX Command not cleared. limit %d \n",
                            __func__, READ64( map, PDCR ), limit ));
                return ResultCommandNotCleared;
            }
			if( proto->fi.pattern.validSize > 200 ) {
				if( validsize % 500 < 8 ) printf("%d processing\n", validsize );
			}
        } while( validsize > 0 );
    }
#endif
	
	//PRSR
	if( proto->fi.framesize.fsizeSpec == FrameSizeRandom ) {

#if 0 //deprecated since 2015.06.14
		int min, max, mod;
		srand(time(NULL));
		mod = time(NULL)%4;

		min = proto->fi.framesize.fsizeMin < SIZE_FRAME_MIN ?
			SIZE_FRAME_MIN : proto->fi.framesize.fsizeMin; 
		max = proto->fi.framesize.fsizeMax > SIZE_FRAME_MAX ?
			SIZE_FRAME_MAX : proto->fi.framesize.fsizeMax; 

		if( max < min ) max = SIZE_FRAME_MAX;

		BSL_DEV(("%s: random frame size lists---\n\t", __func__ ));
		for( i=0; i<NUM_FRAMESIZE_RANDOM; i++ ) {
			fsize[i] = getFsizeRandom( min, max, mod );
			BSL_DEV(("%04d ",fsize[i]));
			if(i%10==9) BSL_DEV(("\n\t"));
		}
		BSL_DEV(("\n"));

		for( i=0; i<7; i++ ) {
			regval = 
				((unsigned long long)( fsize[i*5+0] & M_PRSR_FSIZE_RANDOM ) << 
					 I_PRSR_FSIZE_RANDOM_0 ) | 
				((unsigned long long)( fsize[i*5+1] & M_PRSR_FSIZE_RANDOM ) << 
					 I_PRSR_FSIZE_RANDOM_1 ) | 
				((unsigned long long)( fsize[i*5+2] & M_PRSR_FSIZE_RANDOM ) << 
					 I_PRSR_FSIZE_RANDOM_2 ) | 
				((unsigned long long)( fsize[i*5+3] & M_PRSR_FSIZE_RANDOM ) << 
					 I_PRSR_FSIZE_RANDOM_3 ) | 
				((unsigned long long)( fsize[i*5+4] & M_PRSR_FSIZE_RANDOM ) << 
					 I_PRSR_FSIZE_RANDOM_4 ) ; 
			WRITE64( map, PRSR(i), regval );
			BSL_DEV(("%s: PRSR%d %016llX\n", __func__, i, regval ));
		}
#else
		BSL_DEV(("\t"));
		for( i=0; i<NUM_FRAMESIZE_RANDOM; i++ ) {
			fsize[i] = proto->fi.framesize.fsizeValueRand[i];
			BSL_DEV(("%04X ",fsize[i]));
			if(i%10==9) BSL_DEV(("\n\t"));
		}
		BSL_DEV(("\n"));

		for( i=0; i<7; i++ ) {
			regval = 
				((unsigned long long)( fsize[i*5+0] & M_PRSR_FSIZE_RANDOM ) << 
					 I_PRSR_FSIZE_RANDOM_0 ) | 
				((unsigned long long)( fsize[i*5+1] & M_PRSR_FSIZE_RANDOM ) << 
					 I_PRSR_FSIZE_RANDOM_1 ) | 
				((unsigned long long)( fsize[i*5+2] & M_PRSR_FSIZE_RANDOM ) << 
					 I_PRSR_FSIZE_RANDOM_2 ) | 
				((unsigned long long)( fsize[i*5+3] & M_PRSR_FSIZE_RANDOM ) << 
					 I_PRSR_FSIZE_RANDOM_3 ) | 
				((unsigned long long)( fsize[i*5+4] & M_PRSR_FSIZE_RANDOM ) << 
					 I_PRSR_FSIZE_RANDOM_4 ) ; 
			WRITE64( map, PRSR(i), regval );
			BSL_DEV(("%s: PRSR%d %016llX\n", __func__, i, regval ));
		}
#endif //if 0
	}
	else if( proto->fi.framesize.fsizeSpec == FrameSizeIncrement ) {
	//PIDR
		int min, max;

		min = proto->fi.framesize.fsizeMin < SIZE_FRAME_MIN ?
			SIZE_FRAME_MIN : proto->fi.framesize.fsizeMin; 
		max = proto->fi.framesize.fsizeMax > SIZE_FRAME_MAX ?
			SIZE_FRAME_MAX : proto->fi.framesize.fsizeMax; 

		if( max < min ) max = SIZE_FRAME_MAX;

		regval = (unsigned long long)( max & M_PIDR_LAST_SIZE ) << I_PIDR_LAST_SIZE;
		regval |= ((unsigned long long)( proto->fi.framesize.sizeOrStep & M_PIDR_SIZE_DIFF ) << I_PIDR_SIZE_DIFF );

		//TODO: min value ?
		WRITE64( map, PIDR, regval );
		BSL_DEV(("%s: PIDR  %016llX\n", __func__, regval ));
	}

	//PVDR
	if( proto->fi.framesize.fsizeSpec == FrameSizeRandom ) {
		int payload2[NUM_FRAMESIZE_RANDOM+1] = {0,};
#if 0 //deprecated since 2016.06.14
		unsigned short payload[NUM_FRAMESIZE_RANDOM+1] = {0,};
		int payload2[NUM_FRAMESIZE_RANDOM+1] = {0,};
		srand(time(NULL));
		BSL_DEV(("%s: random data lists---\n\t", __func__ ));
		for( i=0; i<NUM_PAYLOAD_VALUE_RANDOM; i++ ) {
			payload[i] = getPayloadRandom();
			BSL_DEV(("%04X ",payload[i]));
			if(i%10==9) BSL_DEV(("\n\t"));
		}
		BSL_DEV(("%s: their 2's complement lists---\n\t", __func__ ));
		payload2[0] = payload[1];
		for( i=1; i<NUM_PAYLOAD_VALUE_RANDOM; i++ ) {
			payload2[i] = getTwosComplement(payload[i-1], payload[i]);
			BSL_DEV(("%04X ",payload[i]));
			if(i%10==9) BSL_DEV(("\n\t"));
		}
		for( i=0; i<11; i++ ) {
			regval = 
				( (unsigned long long)( payload2[i*3+0] & M_PVDR_VALUE ) 
				  << I_PVDR_DATA_RANDOM_0 ) | 
				( (unsigned long long)( payload2[i*3+1] & M_PVDR_VALUE ) 
				  << I_PVDR_DATA_RANDOM_1 ) | 
				( (unsigned long long)( payload2[i*3+2] & M_PVDR_VALUE ) 
				  << I_PVDR_DATA_RANDOM_2 ); 
			WRITE64( map, PVDR(i), regval );
			BSL_DEV(("%s: PVDR%d %016llX\n", __func__, i, regval ));
		}
#else
		BSL_DEV(("\t"));
		for( i=0; i<NUM_FRAMESIZE_RANDOM; i++ ) {
			payload2[i] = proto->fi.framesize.fsizeValueRandDiff[i];
			BSL_DEV(("%04X ",payload2[i]));
			if(i%10==9) BSL_DEV(("\n\t"));
		}
		BSL_DEV(("\n"));

		for( i=0; i<11; i++ ) {
			regval = 
				( (unsigned long long)( payload2[i*3+0] & M_PVDR_VALUE ) 
				  << I_PVDR_DATA_RANDOM_2 ) | 
				( (unsigned long long)( payload2[i*3+1] & M_PVDR_VALUE ) 
				  << I_PVDR_DATA_RANDOM_1 ) | 
				( (unsigned long long)( payload2[i*3+2] & M_PVDR_VALUE ) 
				  << I_PVDR_DATA_RANDOM_0 ); 
			WRITE64( map, PVDR(i), regval );
			BSL_DEV(("%s: PVDR%d %016llX\n", __func__, i, regval ));
		}
#endif //if 0
	} 

	//PDWR
	//PDCR

	//2. header assign

	//1. PACR
	/* move to LAST 
	pacr = (unsigned long long)( proto->headerLength & M_PACR_HEADSIZE ) << I_PACR_HEADSIZE;
	pacr |= ( proto->l2.protocol == ProtocolEthernet ) ? 
			( 1 << I_PACR_DMAC ) | ( 1 << I_PACR_SMAC ) : 0;
	pacr |= ( proto->l3.protocol == ProtocolIP6 ) ||
			( proto->l3.protocol == ProtocolIP4OverIP6 ) || 
			( proto->l3.protocol == ProtocolIP6OverIP4 ) ? 
			( 1 << I_PACR_IP6_SIP ) | ( 1 << I_PACR_IP6_DIP ) : 0;
	pacr |= ( proto->l3.protocol == ProtocolIP4 ) ||
			( proto->l3.protocol == ProtocolIP4OverIP6 ) || 
			( proto->l3.protocol == ProtocolIP6OverIP4 ) ? 
			( 1 << I_PACR_IP4_SIP ) | ( 1 << I_PACR_IP4_DIP ) : 0;
	pacr |= ( proto->l4.protocol == ProtocolTCP ) || 
			( proto->l4.protocol == ProtocolUDP ) ? 
			( 1 << I_PACR_SPORT ) |  ( 1 << I_PACR_DPORT ) : 0;
	pacr |= ( isoverride_ip4 ? ( 1 << I_PACR_L3_CHKSUM ) : 0;
	pacr |= ( isoverride_l4 ? ( 1 << I_PACR_L4_CHKSUM ) : 0;
	pacr |= ( proto->l3.protocol == ProtocolIP4 ) ? ( 1 << I_PACR_IS_IP4 ) : 0;
	pacr |= ( proto->l4.protocol == ProtocolTCP ) ? ( 1 << I_PACR_IS_TCP ) : 0;
	pacr |= ( proto->l4.protocol == ProtocolUDP ) ? ( 1 << I_PACR_IS_UDP ) : 0;
	pacr |= ( proto->l3.protocol == ProtocolIP4OverIP6 ) ? ( 1 << I_PACR_IS_IP4_IP6 ) : 0;
	pacr |= ( proto->l3.protocol == ProtocolIP6OverIP4 ) ? ( 1 << I_PACR_IS_IP6_IP4 ) : 0;
	pacr |= 1 << 0;

	WRITE64( map, PACR, pacr );
	BSL_DEV(("%s: PACR  %016llX\n", __func__, pacr ));
	my_msleep(100);
	*/

	//2. PHIR
	hlen = 0;
	for( i=0; hlen<proto->headerLength; i++, hlen+=sizeof( unsigned long long ) ) {
		unsigned long long regval = 0;
		memcpy( &regval, proto->header+hlen, sizeof(unsigned long long) );
		//WRITE64( map, PHIR(i), (*(unsigned long long*)(proto->header+hlen) ));
		//BSL_DEV(("%s: PHIR%d %016llX\n", __func__, i, (*(unsigned long long*)(proto->header+hlen)) ));
//		bsl_swap32(&regval, sizeof(regval));
//		bsl_swap64(&regval, sizeof(regval));
		WRITE64( map, PHIR(i), regval );
		BSL_DEV(("%s: PHIR%d %016llX\n", __func__, i, regval ));
	}

	if( proto->l2.protocol == ProtocolEthernet ) {
		T_PDR_Ethernet* pdr = (T_PDR_Ethernet*)proto->l2.pdr;
	//3. DMSR/DMLR
		dmsr = 
			( (unsigned long long ) pdr->dest.addr[5] << 0 ) |
			( (unsigned long long ) pdr->dest.addr[4] << 8 ) |
			( (unsigned long long ) pdr->dest.addr[3] << 16 ) |
			( (unsigned long long ) pdr->dest.addr[2] << 24 ) |
			( (unsigned long long ) pdr->dest.addr[1] << 32 ) |
			( (unsigned long long ) pdr->dest.addr[0] << 40 ) ;
		dmsr |= (unsigned long long)( pdr->dest.step & M_DMSR_OFFSET ) << I_DMSR_OFFSET;
		WRITE64( map, DMSR, dmsr );
		BSL_DEV(("%s: DMSR  %016llX\n", __func__, dmsr ));

		dmlr = 0ll;
		dmlr |= proto->l2tag.protocol == ProtocolISL ? (unsigned long long)(sizeof(T_Isl)) << I_DMLR_DISTANCE : 0;
		dmlr |= ( pdr->dest.mode == EtherAddrModeRandom ) ?  
				( 1ll << I_DMLR_MARK_RANDOM ) : 0;
		dmlr |= ( pdr->dest.mode != EtherAddrModeFixed ) ?  
				( 1ll << I_DMLR_MARK_CHANGE ) : 0;
		dmlr |= ( pdr->dest.mode == EtherAddrModeContDecrement ) ||
				( pdr->dest.mode == EtherAddrModeContIncrement ) ?
				( 1ll << I_DMLR_MARK_CONTINUE ) : 0;
		dmlr |= ( pdr->dest.mode == EtherAddrModeDecrement ||
				pdr->dest.mode == EtherAddrModeContDecrement ) ?
				( 1ll << I_DMLR_MARK_DECREMENT ) : 0;
		if( ( pdr->dest.mode == EtherAddrModeIncrement ) || 
				( pdr->dest.mode == EtherAddrModeDecrement ) ) {
				unsigned char lastaddr[SIZE_ETHER_ADDR] = {0,};
				get_lastmac( &pdr->dest, lastaddr );
				dmlr |= 
					( (unsigned long long ) lastaddr[5] << 0 ) |
					( (unsigned long long ) lastaddr[4] << 8 ) |
					( (unsigned long long ) lastaddr[3] << 16 ) |
					( (unsigned long long ) lastaddr[2] << 24 ) |
					( (unsigned long long ) lastaddr[1] << 32 ) |
					( (unsigned long long ) lastaddr[0] << 40 ) ;
		}
		else if( ( pdr->dest.mode == EtherAddrModeContIncrement ) ) {
			dmlr |= 
					( (unsigned long long ) 0xFF << 0 ) |
					( (unsigned long long ) 0xFF << 8 ) |
					( (unsigned long long ) 0xFF << 16 ) |
					( (unsigned long long ) 0xFF << 24 ) |
					( (unsigned long long ) 0xFF << 32 ) |
					( (unsigned long long ) 0xFF << 40 ) ;
		}

		dmlr |= ((unsigned long long)( proto->fi.crc & M_DMLR_CRC )) << I_DMLR_CRC;

		WRITE64( map, DMLR, dmlr );
		BSL_DEV(("%s: DMLR  %016llX\n", __func__, dmlr ));

	//4. SMSR/SMLR
		smsr = 
			( (unsigned long long ) pdr->src.addr[5] << 0 ) |
			( (unsigned long long ) pdr->src.addr[4] << 8 ) |
			( (unsigned long long ) pdr->src.addr[3] << 16 ) |
			( (unsigned long long ) pdr->src.addr[2] << 24 ) |
			( (unsigned long long ) pdr->src.addr[1] << 32 ) |
			( (unsigned long long ) pdr->src.addr[0] << 40 ) ;
		smsr |= (unsigned long long)( pdr->src.step & M_SMSR_OFFSET ) << I_SMSR_OFFSET;
		WRITE64( map, SMSR, smsr );
		BSL_DEV(("%s: SMSR  %016llX\n", __func__, smsr ));

		smlr = 0ll;
		smlr |= ( pdr->src.mode == EtherAddrModeRandom ) ?  
				( 1ll << I_SMLR_MARK_RANDOM ) : 0;
		smlr |= ( pdr->src.mode != EtherAddrModeFixed ) ?  
				( 1ll << I_SMLR_MARK_CHANGE ) : 0;
		smlr |= ( pdr->src.mode == EtherAddrModeContDecrement ) ||
				( pdr->src.mode == EtherAddrModeContIncrement ) ?
				( 1ll << I_SMLR_MARK_CONTINUE ) : 0;
		smlr |= ( pdr->src.mode == EtherAddrModeDecrement ||
				pdr->src.mode == EtherAddrModeContDecrement ) ?
				( 1ll << I_SMLR_MARK_DECREMENT ) : 0;
		if( ( pdr->src.mode == EtherAddrModeIncrement ) || 
				( pdr->src.mode == EtherAddrModeDecrement ) ) {
				unsigned char lastaddr[SIZE_ETHER_ADDR] = {0,};
				get_lastmac( &pdr->src, lastaddr );
				smlr |= 
					( (unsigned long long ) lastaddr[5] << 0 ) |
					( (unsigned long long ) lastaddr[4] << 8 ) |
					( (unsigned long long ) lastaddr[3] << 16 ) |
					( (unsigned long long ) lastaddr[2] << 24 ) |
					( (unsigned long long ) lastaddr[1] << 32 ) |
					( (unsigned long long ) lastaddr[0] << 40 ) ;
		}
		else if( ( pdr->src.mode == EtherAddrModeContIncrement ) ) {
			smlr |= 
					( (unsigned long long ) 0xFF << 0 ) |
					( (unsigned long long ) 0xFF << 8 ) |
					( (unsigned long long ) 0xFF << 16 ) |
					( (unsigned long long ) 0xFF << 24 ) |
					( (unsigned long long ) 0xFF << 32 ) |
					( (unsigned long long ) 0xFF << 40 ) ;
		}

		WRITE64( map, SMLR, smlr );
		BSL_DEV(("%s: SMLR  %016llX\n", __func__, smlr ));
	}

	//5. IP6
	if( ( proto->l3.protocol == ProtocolIP6 ) ||
		( proto->l3.protocol == ProtocolIP4OverIP6 ) ||
		( proto->l3.protocol == ProtocolIP6OverIP4 ) ) {

		int ishost = 0;
		int isnet = 0;
		int isdecre = 0;
		T_PDR_Ip6* pdr = get_ip6_pdr( proto );
		BSL_CHECK_NULL( pdr, ResultNullArg );
		T_Ip6AddrTuple* ip6tuple;

		//5.1 SIP6
		ip6tuple = &pdr->sip;
		ishost = ( ip6tuple->mode == IpAddrModeIncrementHost ) || 
				 ( ip6tuple->mode == IpAddrModeDecrementHost ) ? 1 : 0;
		isnet  = ( ip6tuple->mode == IpAddrModeIncrementNetwork ) || 
				 ( ip6tuple->mode == IpAddrModeDecrementNetwork ) ? 1 : 0;
		isdecre = ( ip6tuple->mode == IpAddrModeDecrementHost ) || 
				  ( ip6tuple->mode == IpAddrModeDecrementNetwork ) ? 1 : 0;

		sisr = ( get_distance_ip6sip( proto ) & M_SI6SR_DISTANCE ) << I_SI6SR_DISTANCE;
		sisr |= ( ip6tuple->mode != IpAddrModeFixed ) ? ( 1ll << I_SI6SR_MARK_CHANGE ) : 
				0;
		sisr |= ((unsigned long long)(ip6tuple->mask & M_SI6SR_MASK)) << I_SI6SR_MASK;
		sisr |= isnet ? ( 1ll << I_SI6SR_MARK_NETWORK ) : 0;
		sisr |= isdecre ? ( 1ll << I_SI6SR_MARK_SIGN ) : 0;
		sisr |= ( ip6tuple->step & M_SI6SR_OFFSET ) << I_SI6SR_OFFSET;

		WRITE64( map, SI6SR0, sisr );
		BSL_DEV(("%s: SI6SR0 %016llX\n", __func__, sisr ));

		WRITE64( map, SI6SR1, htonll(*(unsigned long long*)(ip6tuple->addr)) );
		BSL_DEV(("%s: SI6SR1 %016llX\n", 
					__func__, htonll(*(unsigned long long*)(ip6tuple->addr)) ));
		WRITE64( map, SI6SR2, 
				htonll(*(unsigned long long*)(((void*)ip6tuple->addr) + 
						sizeof( unsigned long long ))) );
		BSL_DEV(("%s: SI6SR2 %016llX\n", __func__, 
				htonll(*(unsigned long long*)(((void*)ip6tuple->addr) + 
						sizeof( unsigned long long ))) ));

		//LastAddress
		unsigned char lastip6addr[16]={0,};
		get_lastip6( ip6tuple, lastip6addr );  
		WRITE64( map, SI6LR0, htonll(*(unsigned long long*)(lastip6addr)) );
		BSL_DEV(("%s: SI6LR0 %016llX\n", 
					__func__, htonll(*(unsigned long long*)(lastip6addr)) ));
		WRITE64( map, SI6LR1, htonll(*(unsigned long long*)(lastip6addr+8)) );
		BSL_DEV(("%s: SI6LR1 %016llX\n", 
					__func__, htonll(*(unsigned long long*)(lastip6addr+8)) ));
		
		//OffsetAddr
		lastoffset = get_lastip6offset( ip6tuple, lastip6addr );
		//WRITE64( map, SI6OR0, htonll(*(unsigned long long*)(addr)) );
		WRITE64( map, SI6OR1, htonll(lastoffset) );
		BSL_DEV(("%s: SI6OR1 %016llX\n", __func__, htonll(lastoffset) ));

		
		//5.2 DIP6
		ip6tuple = &pdr->dip;
		ishost = ( ip6tuple->mode == IpAddrModeIncrementHost ) || 
				 ( ip6tuple->mode == IpAddrModeDecrementHost ) ? 1 : 0;
		isnet  = ( ip6tuple->mode == IpAddrModeIncrementNetwork ) || 
				 ( ip6tuple->mode == IpAddrModeDecrementNetwork ) ? 1 : 0;
		isdecre = ( ip6tuple->mode == IpAddrModeDecrementHost ) || 
				  ( ip6tuple->mode == IpAddrModeDecrementNetwork ) ? 1 : 0;

		sisr = 0ll;
		sisr |= ((unsigned long long)pdr->plen) << I_DI6SR_PAYLOADLEN; 
		sisr |= ( ip6tuple->mode != IpAddrModeFixed ) ? ( 1ll << I_DI6SR_MARK_CHANGE ) : 
				0;
		sisr |= ( (unsigned long long)(ip6tuple->mask & M_DI6SR_MASK) ) << I_DI6SR_MASK;
		sisr |= isnet ? ( 1ll << I_DI6SR_MARK_NETWORK ) : 0;
		sisr |= isdecre ? ( 1ll << I_DI6SR_MARK_SIGN ) : 0;
		sisr |= ( ip6tuple->step & M_DI6SR_OFFSET ) << I_DI6SR_OFFSET;

		WRITE64( map, DI6SR0, sisr );
		BSL_DEV(("%s: DI6SR0 %016llX\n", __func__, sisr ));

		WRITE64( map, DI6SR1, htonll(*(unsigned long long*)(ip6tuple->addr)) );
		BSL_DEV(("%s: DI6SR1 %016llX\n", 
					__func__, htonll(*(unsigned long long*)(ip6tuple->addr)) ));
		WRITE64( map, DI6SR2, 
				htonll(*(unsigned long long*)(((void*)ip6tuple->addr) + 
						sizeof( unsigned long long ))) );
		BSL_DEV(("%s: DI6SR2 %016llX\n", __func__, 
				htonll(*(unsigned long long*)(((void*)ip6tuple->addr) + 
						sizeof( unsigned long long ))) ));

		//LastAddress
		get_lastip6( ip6tuple, lastip6addr );  
		WRITE64( map, DI6LR0, htonll(*(unsigned long long*)(lastip6addr)) );
		BSL_DEV(("%s: DI6LR0 %016llX\n", 
					__func__, htonll(*(unsigned long long*)(lastip6addr)) ));
		WRITE64( map, DI6LR1, htonll(*(unsigned long long*)(lastip6addr+8)) );
		BSL_DEV(("%s: DI6LR1 %016llX\n", 
					__func__, htonll(*(unsigned long long*)(lastip6addr+8)) ));
		
		//OffsetAddr
		lastoffset = get_lastip6offset( ip6tuple, lastip6addr );
		//WRITE64( map, DI6OR0, htonll(*(unsigned long long*)(addr)) );
		WRITE64( map, DI6OR1, htonll(lastoffset) );
		BSL_DEV(("%s: DI6OR1 %016llX\n", __func__, htonll(lastoffset) ));
	}

	//6. IP4
	if( ( proto->l3.protocol == ProtocolIP4 ) ||
		( proto->l3.protocol == ProtocolIP4OverIP6 ) ||
		( proto->l3.protocol == ProtocolIP6OverIP4 ) ) {
		unsigned long long regi; 
		unsigned long long chksum_distance;
		unsigned long long deltaid = 0ll;					// 2017.2.27 by dgjung
		unsigned short lastid = 0;							// 2017.2.27 by dgjung
		T_PDR_Ip4* pdr = get_ip4_pdr( proto );
		BSL_CHECK_NULL( pdr, ResultNullArg );
		T_Ip4AddrTuple* tuple = &pdr->sip;
		BSL_CHECK_NULL( tuple, ResultNullArg );

		//
		//Source IP
		//
		//Change IpAddrMode 'Network' to 'Host' in case of No_mask class.
		if( tuple->mask == 0 ) {
			if( tuple->mode == IpAddrModeContIncrementNetwork ) 
				tuple->mode = IpAddrModeContIncrementHost;
			else if( tuple->mode == IpAddrModeContDecrementNetwork ) 
				tuple->mode = IpAddrModeContDecrementHost;
			else if( tuple->mode == IpAddrModeIncrementNetwork ) 
				tuple->mode = IpAddrModeIncrementHost;
			else if( tuple->mode == IpAddrModeDecrementNetwork ) 
				tuple->mode = IpAddrModeDecrementHost;
		}

		int iscont = 
			tuple->mode == IpAddrModeContIncrementHost ? 1 :
			tuple->mode == IpAddrModeContDecrementHost ? 1 :
			tuple->mode == IpAddrModeContIncrementNetwork ? 1 :
			tuple->mode == IpAddrModeContDecrementNetwork ? 1 :
			0;
		int isdecre = 
			tuple->mode == IpAddrModeDecrementHost ? 1 :
			tuple->mode == IpAddrModeDecrementNetwork ? 1 :
			tuple->mode == IpAddrModeContDecrementHost ? 1 :
			tuple->mode == IpAddrModeContDecrementNetwork ? 1 :
			0;

		//SI4SR
		chksum_distance = (unsigned long long)get_distance_ip4chksum( proto ); 
		regi = ( chksum_distance & M_SI4SR_CHKSUM_DISTANCE ) << 
			I_SI4SR_CHKSUM_DISTANCE;
		regi |= tuple->addr;
		regi |= (((unsigned long long)( chksum_distance + 2ll )) << 
				I_SI4SR_DISTANCE);

		if( tuple->mode == IpAddrModeFixed ) ; //nothing to do
		else if( 
				( tuple->mode == IpAddrModeIncrementHost ) ||
				( tuple->mode == IpAddrModeDecrementHost ) ||
				( tuple->mode == IpAddrModeContIncrementHost ) ||
				( tuple->mode == IpAddrModeContDecrementHost ) ) {
			regi |= ( 1ll << I_SI4SR_MARK_CHANGE );
			regi |= isdecre ? ( 1ll << I_SI4SR_MARK_OFFSET_SIGN ) : 0ll;
			regi |= iscont ? ( 1ll << I_SI4SR_MARK_CONTINUE ) : 0ll;
		}
		else if( 
				( tuple->mode == IpAddrModeIncrementNetwork ) ||
				( tuple->mode == IpAddrModeDecrementNetwork ) ||
				( tuple->mode == IpAddrModeContIncrementNetwork ) ||
				( tuple->mode == IpAddrModeContDecrementNetwork ) ) {
			regi |= ( 1ll << I_SI4SR_MARK_CLASS );
			regi |= ( 1ll << I_SI4SR_MARK_CHANGE );
			regi |= isdecre ? ( 1ll << I_SI4SR_MARK_OFFSET_SIGN ) : 0ll;
			regi |= ( get_ip4_classSel( tuple ) << I_SI4SR_CLASS_SEL );
			regi |= iscont ? ( 1ll << I_SI4SR_MARK_CONTINUE ) : 0ll;
		}
		else if ( tuple->mode == IpAddrModeRandom ) { 
			regi |= ( 1ll << I_SI4SR_MARK_RANDOM );
		}

		WRITE64( map, SI4SR, regi );
		BSL_DEV(("%s: SI4SR %016llX\n", __func__, regi ));


		//SI4LR
		lastip4 = get_lastip4( tuple );
		BSL_TRACE(("%s:DEBUG lastip4 %08X\n", __func__, lastip4 ));
		regi = lastip4;
		regi |= ( ( (unsigned long long)(tuple->mask) ) << I_SI4LR_IP_MASK );

		WRITE64( map, SI4LR, regi );
		BSL_DEV(("%s: SI4LR %016llX\n", __func__, regi ));


		//SI4OR
		T_CustomIntegerTuple* tuple_id = &pdr->id;
		BSL_CHECK_NULL( tuple_id, ResultNullArg );		
		regi = get_lastip4offset( tuple, lastip4 );
		BSL_TRACE(("%s:DEBUG lastoffset %08X\n", __func__, (unsigned int)regi ));
//		regi |= ((unsigned long long)pdr->tlen) << I_SI4OR_TLEN_CURRENT;
		regi |= ((unsigned long long)tuple_id->value) << I_SI4OR_ID_START; 								// 2017.4.4 by dgjung
		regi |= ((unsigned long long)pdr->tlen) << I_SI4OR_TLEN_START; 
		WRITE64( map, SI4OR, regi );
		BSL_DEV(("%s: SI4OR %016llX\n", __func__, regi ));

		//
		// Destination IP
		//
		tuple = &pdr->dip;

		//Change IpAddrMode 'Network' to 'Host' in case of No_mask class.
		if( tuple->mask == 0 ) {
			if( tuple->mode == IpAddrModeContIncrementNetwork ) 
				tuple->mode = IpAddrModeContIncrementHost;
			else if( tuple->mode == IpAddrModeContDecrementNetwork ) 
				tuple->mode = IpAddrModeContDecrementHost;
			else if( tuple->mode == IpAddrModeIncrementNetwork ) 
				tuple->mode = IpAddrModeIncrementHost;
			else if( tuple->mode == IpAddrModeDecrementNetwork ) 
				tuple->mode = IpAddrModeDecrementHost;
		}

		iscont = 
			tuple->mode == IpAddrModeContIncrementHost ? 1 :
			tuple->mode == IpAddrModeContDecrementHost ? 1 :
			tuple->mode == IpAddrModeContIncrementNetwork ? 1 :
			tuple->mode == IpAddrModeContDecrementNetwork ? 1 :
			0;
		isdecre = 
			tuple->mode == IpAddrModeDecrementHost ? 1 :
			tuple->mode == IpAddrModeDecrementNetwork ? 1 :
			tuple->mode == IpAddrModeContDecrementHost ? 1 :
			tuple->mode == IpAddrModeContDecrementNetwork ? 1 :
			0;

		//DI4SR
		chksum_distance = (unsigned long long)get_distance_ip4chksum( proto ); 
		regi = ( chksum_distance & M_SI4SR_CHKSUM_DISTANCE ) << I_SI4SR_CHKSUM_DISTANCE;
		regi |= tuple->addr;

		if( tuple->mode == IpAddrModeFixed ) ; //nothing to do
		else if( 
				( tuple->mode == IpAddrModeIncrementHost ) ||
				( tuple->mode == IpAddrModeDecrementHost ) ||
				( tuple->mode == IpAddrModeContIncrementHost ) ||
				( tuple->mode == IpAddrModeContDecrementHost ) ) {
			regi |= ( 1ll << I_SI4SR_MARK_CHANGE );
			regi |= isdecre ? ( 1ll << I_SI4SR_MARK_OFFSET_SIGN ) : 0ll;
			regi |= iscont ? ( 1ll << I_SI4SR_MARK_CONTINUE ) : 0ll;
		}
		else if( 
				( tuple->mode == IpAddrModeIncrementNetwork ) ||
				( tuple->mode == IpAddrModeDecrementNetwork ) ||
				( tuple->mode == IpAddrModeContIncrementNetwork ) ||
				( tuple->mode == IpAddrModeContDecrementNetwork ) ) {
			regi |= ( 1ll << I_SI4SR_MARK_CLASS );
			regi |= ( 1ll << I_SI4SR_MARK_CHANGE );
			regi |= isdecre ? ( 1ll << I_SI4SR_MARK_OFFSET_SIGN ) : 0ll;
			regi |= ( get_ip4_classSel( tuple ) << I_SI4SR_CLASS_SEL );
			regi |= iscont ? ( 1ll << I_SI4SR_MARK_CONTINUE ) : 0ll;
		}
		else if ( tuple->mode == IpAddrModeRandom ) { 
			regi |= ( 1ll << I_SI4SR_MARK_RANDOM );
		}

		WRITE64( map, DI4SR, regi );
		BSL_DEV(("%s: DI4SR %016llX\n", __func__, regi ));


		//DI4LR
		lastip4 = get_lastip4( tuple );
		BSL_TRACE(("%s:DEBUG lastip4 %08X\n", __func__, lastip4 ));
		regi = lastip4;
		regi |= ( ( (unsigned long long)(tuple->mask) ) << I_SI4LR_IP_MASK );

		WRITE64( map, DI4LR, regi );
		BSL_DEV(("%s: DI4LR %016llX\n", __func__, regi ));


		//DI4OR
		regi = get_lastip4offset( tuple, lastip4 );
		if( proto->l4.protocol == ProtocolUDP ) {
			T_PDR_UDP* pdrudp = (T_PDR_UDP*)proto->l4.pdr;
			BSL_CHECK_NULL( pdrudp, ResultNullArg );

			regi |= ((unsigned long long)(pdrudp->val & M_DI4OR_UDP_TLEN)) <<
				    I_DI4OR_UDP_TLEN;
		}

		BSL_TRACE(("%s:DEBUG lastoffset %08X\n", __func__, (unsigned int)regi ));
		WRITE64( map, DI4OR, regi );
		BSL_DEV(("%s: DI4OR %016llX \n", __func__, regi ));

		// IDVR									2017.2.27 by dgjung
		iscont = 
			tuple_id->mode == CustomIntegerModeContIncrement ? 1 :
			tuple_id->mode == CustomIntegerModeContDecrement ? 1 :
			0;
		int isincre = 
			tuple_id->mode == CustomIntegerModeIncrement ? 1 :
			tuple_id->mode == CustomIntegerModeContIncrement ? 1 :
			0;
		isdecre = 
			tuple_id->mode == CustomIntegerModeDecrement ? 1 :
			tuple_id->mode == CustomIntegerModeContDecrement ? 1 :
			0;

		lastid = getLastIp4ID( tuple_id );
		deltaid = lastid > tuple_id->value ? 
					lastid - tuple_id->value :
					tuple_id->value - lastid;

		regi = 0ll;
		regi |= ( (unsigned long long)(proto->l3.offset + 4) & M_IDVR_DISTANCE ) << 						// 2017.4.3 by dgjung
				I_IDVR_DISTANCE;
		regi |= ( tuple_id->mode == CustomIntegerModeRandom ) ?  
				( 1ll << I_IDVR_MARK_RANDOM ) : 0;
		regi |= ( tuple_id->mode != CustomIntegerModeFixed ) ?  
				( 1ll << I_IDVR_MARK_CHANGE ) : 0;
		regi |= iscont ?  ( 1ll << I_IDVR_MARK_CONTINUE ) : 0;
		regi |= isdecre ?  ( 1ll << I_IDVR_MARK_SIGN ) : 0;
		regi |= isincre || isdecre ?
				( (unsigned long long)(tuple_id->step) & M_IDVR_OFFSET_VALUE ) << 
				I_IDVR_OFFSET_VALUE : 0;

		regi |= ( tuple_id->mode == CustomIntegerModeIncrement ) ?
				( lastid & M_IDVR_LAST_VALUE ) << I_IDVR_LAST_VALUE :
				( tuple_id->mode == CustomIntegerModeDecrement ) ?
				( lastid & M_IDVR_LAST_VALUE ) << I_IDVR_LAST_VALUE : 
				0;
		regi |= ( tuple_id->mode == CustomIntegerModeIncrement ) ||
				( tuple_id->mode == CustomIntegerModeDecrement ) ?
				( deltaid & M_IDVR_LAST_OFFSET ) << I_IDVR_LAST_OFFSET : 0;

		WRITE64( map, IDVR, regi );
		BSL_DEV(("%s: IDVR  %016llX\n", __func__, regi ));
	}

	//7. SPORT/DPORT
	if( ( proto->l4.protocol == ProtocolTCP ) ||
		( proto->l4.protocol == ProtocolUDP ) ) {
		unsigned long long regi; 
		unsigned long long chksum_distance;
		unsigned long long deltaport = 0ll;
		unsigned short lastport = 0;
		T_CustomIntegerTuple* tuple = NULL;
		T_PDR_UDP* pdrudp = (T_PDR_UDP*)proto->l4.pdr;
		BSL_CHECK_NULL( pdrudp, ResultNullArg );

		//SPVR
		tuple = &pdrudp->sport;
		BSL_CHECK_NULL( tuple, ResultNullArg );		
		int iscont = 
			tuple->mode == CustomIntegerModeContIncrement ? 1 :
			tuple->mode == CustomIntegerModeContDecrement ? 1 :
			0;
		int isincre = 
			tuple->mode == CustomIntegerModeIncrement ? 1 :
			tuple->mode == CustomIntegerModeContIncrement ? 1 :
			0;
		int isdecre = 
			tuple->mode == CustomIntegerModeDecrement ? 1 :
			tuple->mode == CustomIntegerModeContDecrement ? 1 :
			0;

		lastport = getLastTcpUdpPort( tuple );
		deltaport = lastport > tuple->value ? 
					lastport - tuple->value :
					tuple->value - lastport;

		regi = 0ll;
		regi |= ( (unsigned long long)(proto->l4.offset) & M_SPVR_PORT_DISTANCE ) << 
				I_SPVR_PORT_DISTANCE;
		regi |= ( tuple->mode == CustomIntegerModeRandom ) ?  
				( 1ll << I_SPVR_MARK_RANDOM ) : 0;
		regi |= ( tuple->mode != CustomIntegerModeFixed ) ?  
				( 1ll << I_SPVR_MARK_CHANGE ) : 0;
		regi |= iscont ?  ( 1ll << I_SPVR_MARK_CONTINUE ) : 0;
		regi |= isdecre ?  ( 1ll << I_SPVR_MARK_SIGN ) : 0;
		regi |= isincre || isdecre ?
				( (unsigned long long)(tuple->step) & M_SPVR_OFFSET_VALUE ) << 
				I_SPVR_OFFSET_VALUE : 0;

		regi |= ( tuple->mode == CustomIntegerModeIncrement ) ?
				( lastport & M_SPVR_LAST_VALUE ) << I_SPVR_LAST_VALUE :
				( tuple->mode == CustomIntegerModeDecrement ) ?
				( lastport & M_SPVR_LAST_VALUE ) << I_SPVR_LAST_VALUE : 
				0;
		regi |= ( tuple->mode == CustomIntegerModeIncrement ) ||
				( tuple->mode == CustomIntegerModeDecrement ) ?
				( deltaport & M_SPVR_LAST_OFFSET ) << I_SPVR_LAST_OFFSET : 0;

		WRITE64( map, SPVR, regi );
		BSL_DEV(("%s: SPVR  %016llX\n", __func__, regi ));

		//DPVR
		tuple = &pdrudp->dport;
		BSL_CHECK_NULL( tuple, ResultNullArg );

		iscont = 
			tuple->mode == CustomIntegerModeContIncrement ? 1 :
			tuple->mode == CustomIntegerModeContDecrement ? 1 :
			0;
		isincre = 
			tuple->mode == CustomIntegerModeIncrement ? 1 :
			tuple->mode == CustomIntegerModeContIncrement ? 1 :
			0;
		isdecre = 
			tuple->mode == CustomIntegerModeDecrement ? 1 :
			tuple->mode == CustomIntegerModeContDecrement ? 1 :
			0;

		lastport = getLastTcpUdpPort( tuple );
		deltaport = lastport > tuple->value ? 
					lastport - tuple->value :
					tuple->value - lastport;

		regi = 0ll;
		regi |= ( proto->l4.protocol == ProtocolUDP ) ?
				((unsigned long long)( proto->l4.offset + offsetof( T_Udp, checksum ))) <<
					I_DPVR_CHKSUM_DISTANCE :
				( proto->l4.protocol == ProtocolTCP ) ?
				((unsigned long long)( proto->l4.offset + offsetof( T_Tcp, checksum ))) << 
					I_DPVR_CHKSUM_DISTANCE :
				 0;
		regi |= ( tuple->mode != CustomIntegerModeFixed ) ?  
				( 1ll << I_DPVR_MARK_CHANGE ) : 0;
		regi |= ( tuple->mode == CustomIntegerModeRandom ) ?  
				( 1ll << I_DPVR_MARK_RANDOM ) : 0;
		regi |= iscont ?  ( 1ll << I_DPVR_MARK_CONTINUE ) : 0;
		regi |= isdecre ?  ( 1ll << I_DPVR_MARK_SIGN ) : 0;
		regi |= isincre || isdecre ?
				( (unsigned long long)(tuple->step) & M_DPVR_OFFSET_VALUE ) << 
				I_DPVR_OFFSET_VALUE : 0;

		regi |= ( tuple->mode == CustomIntegerModeIncrement ) ?
				( lastport & M_DPVR_LAST_VALUE ) << I_DPVR_LAST_VALUE :
				( tuple->mode == CustomIntegerModeDecrement ) ?
				( lastport & M_DPVR_LAST_VALUE ) << I_DPVR_LAST_VALUE : 
				0;
		regi |= ( tuple->mode == CustomIntegerModeIncrement ) ||
				( tuple->mode == CustomIntegerModeDecrement ) ?
				( deltaport & M_DPVR_LAST_OFFSET ) << I_DPVR_LAST_OFFSET : 0;

		WRITE64( map, DPVR, regi );
		BSL_DEV(("%s: DPVR  %016llX\n", __func__, regi ));

		//PSAR
		tuple = &pdrudp->dport;
		regi = 0ll;
		regi |= ( (unsigned long long)(tuple->value & M_PSAR_DPORT_START_VALUE) ) << 
			I_PSAR_DPORT_START_VALUE;

		tuple = &pdrudp->sport;
		regi |= ( tuple->value & M_PSAR_SPORT_START_VALUE ) << I_PSAR_SPORT_START_VALUE;

		WRITE64( map, PSAR, regi );
		BSL_DEV(("%s: PSAR  %016llX\n", __func__, regi ));
	}

	int isoverride_ip4 = 0; 
	int isoverride_l4 = 0;

	do {
		//CSVR
		unsigned long long regi = 0ll;

		if( ( proto->l3.protocol == ProtocolIP4 ) ||
			( proto->l3.protocol == ProtocolIP4OverIP6 ) ||
			( proto->l3.protocol == ProtocolIP6OverIP4 ) ) {
			regi |= get_ip4_checksum( proto, &isoverride_ip4 );
		}

		if( ( proto->l4.protocol == ProtocolTCP ) ||
			( proto->l4.protocol == ProtocolUDP ) ||
			( proto->l4.protocol == ProtocolICMP ) ) {
			regi |= 
				(unsigned long long)(get_l4_checksum( proto, &isoverride_l4 ) & 
				M_CSVR_L4_START_VALUE ) << I_CSVR_L4_START_VALUE;
		}

		WRITE64( map, CSVR, regi );
		BSL_DEV(("%s: CSVR  %016llX\n", __func__, regi ));
	} while(0);

	//2nd Phase 
	//stream control 
	WRITE64( map, SMCR, ( 1 << stream->control.control ) );
	BSL_DEV(("%s: SMCR  %016X\n", __func__, 1 << stream->control.control ));

	//stream control set
	regval = 0l;
	regval = (unsigned long long)stream->control.returnToId << I_SCSR_RETURN_TO_ID;
	regval |= ((unsigned long long)(stream->control.loopCount & M_SCSR_LOOP_COUNT) << I_SCSR_LOOP_COUNT);
	regval |= ((unsigned long long)(stream->control.burstsPerStream & M_SCSR_BURST_PER_STREAM));
	WRITE64( map, SCSR, regval );
	BSL_DEV(("%s: SCSR  %016llX\n", __func__, regval ));

	//packets per burst
	WRITE64( map, PBCR, stream->control.pktsPerBurst );
	BSL_DEV(("%s: PBCR  %016llX\n", __func__, stream->control.pktsPerBurst ));

	//inter burst gap
	regval = get_gapvalue( stream->control.interBurstGapIntPart, 
			stream->control.interBurstGapFracPart );
	WRITE64( map, IBGR, regval );
	BSL_DEV(("%s: IBGR  %016llX\n", __func__, regval ));

	//inter stream gap
	regval = get_gapvalue( stream->control.interStreamGapIntPart, 
			stream->control.interStreamGapFracPart );
	WRITE64( map, ISGR, regval );
	BSL_DEV(("%s: ISGR  %016llX\n", __func__, regval ));

			
	//STDR
	regval = get_gapvalue( stream->control.startTxDelay, 0 );
	WRITE64( map, STDR, regval );
	BSL_DEV(("%s: STDR  %016llX\n", __func__, regval ));

    //BGCR
#if 0  //get from GUI
	regval = getIFGFromRateSpec( stream, &proto->fi, fsize );
#else
	int pktsize = 
		proto->fi.framesize.fsizeSpec == FrameSizeFixed ? 
		proto->fi.framesize.sizeOrStep : 
		proto->fi.framesize.fsizeSpec == FrameSizeRandom ? 
		proto->fi.framesize.fsizeValueRand[0] :
		proto->fi.framesize.fsizeSpec == FrameSizeIncrement ? 
		proto->fi.framesize.fsizeMin : 0;
	int mod = (pktsize+4) % 8; //== 0 ? 8 : (pktsize+4) % 8;
	int alpha = (mod < 5) ? 8 : 0;

	regval = stream->control.ifg + mod + alpha;
#endif
	WRITE64( map, BGCR, regval );
	BSL_DEV(("%s: pktsize %d mod %d ifg %llu\n", __func__, (pktsize+4), mod, stream->control.ifg ));
	BSL_DEV(("%s: BGCR  %016llX\n", __func__, regval ));

#if 0 //move to upper
	//At Last, stream number, frame size
	T_FrameSize* frame = (T_FrameSize*)&proto->fi;
	regval = ( ( stream->streamid & M_PGCR_STREAM_NUMBER ) << I_PGCR_STREAM_NUMBER ) | ( 0 );

	regval |= 
		frame->fsizeSpec == FrameSizeFixed ?
		((unsigned long long)( frame->sizeOrStep & M_PGCR_FRAMESIZE )) << I_PGCR_FRAMESIZE : 
		frame->fsizeSpec == FrameSizeIncrement ?
		((unsigned long long)( frame->fsizeMin & M_PGCR_FRAMESIZE )) << I_PGCR_FRAMESIZE :
		frame->fsizeSpec == FrameSizeRandom ?
		((unsigned long long)( frame->fsizeValueRand[0] & M_PGCR_FRAMESIZE )) << I_PGCR_FRAMESIZE : 0;

	WRITE64( map, PGCR, regval );
	BSL_DEV(("%s: PGCR  %016llX\n", __func__, regval ));
#endif

	/* move from above */
	if( proto->fi.payloadType == FrameDataTypeDownload ) pacr = 1 << 0;
	else {
		pacr = (unsigned long long)( proto->headerLength & M_PACR_HEADSIZE ) << I_PACR_HEADSIZE;
		pacr |= ( proto->l2.protocol == ProtocolEthernet ) ? 
				( 1 << I_PACR_DMAC ) | ( 1 << I_PACR_SMAC ) : 0;
		pacr |= ( proto->l3.protocol == ProtocolIP6 ) ||
				( proto->l3.protocol == ProtocolIP4OverIP6 ) || 
				( proto->l3.protocol == ProtocolIP6OverIP4 ) ? 
				( 1 << I_PACR_IP6_SIP ) | ( 1 << I_PACR_IP6_DIP ) : 0;
		pacr |= ( proto->l3.protocol == ProtocolIP4 ) ||
				( proto->l3.protocol == ProtocolIP4OverIP6 ) || 
				( proto->l3.protocol == ProtocolIP6OverIP4 ) ? 
				( 1 << I_PACR_IP4_SIP ) | ( 1 << I_PACR_IP4_DIP ) : 0;
		pacr |= ( proto->l4.protocol == ProtocolTCP ) || 
				( proto->l4.protocol == ProtocolUDP ) ? 
				( 1 << I_PACR_SPORT ) |  ( 1 << I_PACR_DPORT ) : 0;
		pacr |= ( !isoverride_ip4 ) && 
				( proto->l3.protocol == ProtocolIP4 || 
				  proto->l3.protocol == ProtocolIP4OverIP6 || 
				  proto->l3.protocol == ProtocolIP6OverIP4 ) ? 
				( 1 << I_PACR_L3_CHKSUM ) : 0;
		pacr |= ( !isoverride_l4 ) ? ( 1 << I_PACR_L4_CHKSUM ) : 0;
		pacr |= ( proto->l3.protocol == ProtocolIP4 ) ? ( 1 << I_PACR_IS_IP4 ) : 0;
		pacr |= ( proto->l4.protocol == ProtocolTCP ) ? ( 1 << I_PACR_IS_TCP ) : 0;
		pacr |= ( proto->l4.protocol == ProtocolUDP ) ? ( 1 << I_PACR_IS_UDP ) : 0;
		pacr |= ( proto->l3.protocol == ProtocolIP4OverIP6 ) ? ( 1 << I_PACR_IS_IP4_IP6 ) : 0;
		pacr |= ( proto->l3.protocol == ProtocolIP6OverIP4 ) ? ( 1 << I_PACR_IS_IP6_IP4 ) : 0;
		pacr |= 1 << 0;
	}

	WRITE64( map, PACR, pacr );
	BSL_DEV(("%s: PACR  %016llX\n", __func__, pacr ));
	do {
		usleep( 100000 );
	} while( ( READ64( map, PACR ) & ( 1<<0 ) ) && limit-- );

	if( limit <= 0 ) {
		BSL_ERROR(("%s: PACR  %016llX Command not cleared. limit %d \n", 
					__func__, READ64( map, PACR ), limit ));
		return ResultCommandNotCleared;
	}

	map -=  OFFSET_REGISTER_PORT64( portid );
	domunmap( fd, &map );

	return limit <= 0 ? ResultCommandNotCleared : ResultSuccess;
}



// MDIO Opmode flags
#define MDIO_OPMODE_ADDR	0x0
#define MDIO_OPMODE_WRITE	0x1
#define MDIO_OPMODE_POST_READ_INC_ADDR 0x2
#define MDIO_OPMODE_READ	0x3

typedef union {
	struct {
		unsigned long long mcmr;
	} value;
	struct {
		unsigned long long data			: 16;
		unsigned long long dev_addr		: 5;
		unsigned long long phy_addr		: 5;
		unsigned long long opmode		: 2;
		unsigned long long unused		: 36;
	} field;
} mdio_req_entry_t;

unsigned int phy_init_code[][2] = {
	{ 0x1cc00, 0x20c5 },
	{ 0x1cc01, 0x3c05 },
	{ 0x1cc02, 0x6536 },
	{ 0x1cc03, 0x2fe4 },
	{ 0x1cc04, 0x3cd4 },
	{ 0x1cc05, 0x6624 },
	{ 0x1cc06, 0x2015 },
	{ 0x1cc07, 0x3145 },
	{ 0x1cc08, 0x6524 },
	{ 0x1cc09, 0x27ff },
	{ 0x1cc0a, 0x300f },
	{ 0x1cc0b, 0x2c8b },
	{ 0x1cc0c, 0x300b },
	{ 0x1cc0d, 0x4009 },
	{ 0x1cc0e, 0x400e },
	{ 0x1cc0f, 0x2f52 },
	{ 0x1cc10, 0x3002 },
	{ 0x1cc11, 0x1002 },
	{ 0x1cc12, 0x2202 },
	{ 0x1cc13, 0x3012 },
	{ 0x1cc14, 0x1002 },
	{ 0x1cc15, 0x2662 },
	{ 0x1cc16, 0x3012 },
	{ 0x1cc17, 0x1002 },
	{ 0x1cc18, 0xd01e },
	{ 0x1cc19, 0x2862 },
	{ 0x1cc1a, 0x3012 },
	{ 0x1cc1b, 0x1002 },
	{ 0x1cc1c, 0x2004 },
	{ 0x1cc1d, 0x3c84 },
	{ 0x1cc1e, 0x6436 },
	{ 0x1cc1f, 0x2007 },
	{ 0x1cc20, 0x3f87 },
	{ 0x1cc21, 0x8676 },
	{ 0x1cc22, 0x40b7 },
	{ 0x1cc23, 0xa746 },
	{ 0x1cc24, 0x4047 },
	{ 0x1cc25, 0x5673 },
	{ 0x1cc26, 0x29c2 },
	{ 0x1cc27, 0x3002 },
	{ 0x1cc28, 0x13d2 },
	{ 0x1cc29, 0x8bbd },
	{ 0x1cc2a, 0x28f2 },
	{ 0x1cc2b, 0x3012 },
	{ 0x1cc2c, 0x1002 },
	{ 0x1cc2d, 0x2122 },
	{ 0x1cc2e, 0x3012 },
	{ 0x1cc2f, 0x1002 },
	{ 0x1cc30, 0x5cc3 },
	{ 0x1cc31, 0x314 },
	{ 0x1cc32, 0x2982 },
	{ 0x1cc33, 0x3002 },
	{ 0x1cc34, 0x1002 },
	{ 0x1cc35, 0xd019 },
	{ 0x1cc36, 0x20c2 },
	{ 0x1cc37, 0x3012 },
	{ 0x1cc38, 0x1002 },
	{ 0x1cc39, 0x2a04 },
	{ 0x1cc3a, 0x3c74 },
	{ 0x1cc3b, 0x6435 },
	{ 0x1cc3c, 0x2fa4 },
	{ 0x1cc3d, 0x3cd4 },
	{ 0x1cc3e, 0x6624 },
	{ 0x1cc3f, 0x5563 },
	{ 0x1cc40, 0x2d82 },
	{ 0x1cc41, 0x3002 },
	{ 0x1cc42, 0x13d2 },
	{ 0x1cc43, 0x464d },
	{ 0x1cc44, 0x28f2 },
	{ 0x1cc45, 0x3012 },
	{ 0x1cc46, 0x1002 },
	{ 0x1cc47, 0x20c2 },
	{ 0x1cc48, 0x3012 },
	{ 0x1cc49, 0x1002 },
	{ 0x1cc4a, 0x2fb4 },
	{ 0x1cc4b, 0x3cd4 },
	{ 0x1cc4c, 0x6624 },
	{ 0x1cc4d, 0x5563 },
	{ 0x1cc4e, 0x2d82 },
	{ 0x1cc4f, 0x3002 },
	{ 0x1cc50, 0x13d2 },
	{ 0x1cc51, 0x2eb2 },
	{ 0x1cc52, 0x3002 },
	{ 0x1cc53, 0x1002 },
	{ 0x1cc54, 0x2002 },
	{ 0x1cc55, 0x3012 },
	{ 0x1cc56, 0x1002 },
	{ 0x1cc57, 0x004 },
	{ 0x1cc58, 0x2982 },
	{ 0x1cc59, 0x3002 },
	{ 0x1cc5a, 0x1002 },
	{ 0x1cc5b, 0x2122 },
	{ 0x1cc5c, 0x3012 },
	{ 0x1cc5d, 0x1002 },
	{ 0x1cc5e, 0x5cc3 },
	{ 0x1cc5f, 0x317 },
	{ 0x1cc60, 0x2f52 },
	{ 0x1cc61, 0x3002 },
	{ 0x1cc62, 0x1002 },
	{ 0x1cc63, 0x2982 },
	{ 0x1cc64, 0x3002 },
	{ 0x1cc65, 0x1002 },
	{ 0x1cc66, 0x22cd },
	{ 0x1cc67, 0x301d },
	{ 0x1cc68, 0x28f2 },
	{ 0x1cc69, 0x3012 },
	{ 0x1cc6a, 0x1002 },
	{ 0x1cc6b, 0x21a2 },
	{ 0x1cc6c, 0x3012 },
	{ 0x1cc6d, 0x1002 },
	{ 0x1cc6e, 0x5aa3 },
	{ 0x1cc6f, 0x2e02 },
	{ 0x1cc70, 0x3002 },
	{ 0x1cc71, 0x1312 },
	{ 0x1cc72, 0x2d42 },
	{ 0x1cc73, 0x3002 },
	{ 0x1cc74, 0x1002 },
	{ 0x1cc75, 0x2ff7 },
	{ 0x1cc76, 0x30f7 },
	{ 0x1cc77, 0x20c4 },
	{ 0x1cc78, 0x3c04 },
	{ 0x1cc79, 0x6724 },
	{ 0x1cc7a, 0x2807 },
	{ 0x1cc7b, 0x31a7 },
	{ 0x1cc7c, 0x20c4 },
	{ 0x1cc7d, 0x3c24 },
	{ 0x1cc7e, 0x6724 },
	{ 0x1cc7f, 0x1002 },
	{ 0x1cc80, 0x2807 },
	{ 0x1cc81, 0x3187 },
	{ 0x1cc82, 0x20c4 },
	{ 0x1cc83, 0x3c24 },
	{ 0x1cc84, 0x6724 },
	{ 0x1cc85, 0x2fe4 },
	{ 0x1cc86, 0x3cd4 },
	{ 0x1cc87, 0x6437 },
	{ 0x1cc88, 0x20c4 },
	{ 0x1cc89, 0x3c04 },
	{ 0x1cc8a, 0x6724 },
	{ 0x1cc8b, 0x1002 },
	{ 0x1cc8c, 0x2514 },
	{ 0x1cc8d, 0x3c64 },
	{ 0x1cc8e, 0x6436 },
	{ 0x1cc8f, 0xdff4 },
	{ 0x1cc90, 0x6436 },
	{ 0x1cc91, 0x1002 },
	{ 0x1cc92, 0x40a4 },
	{ 0x1cc93, 0x643c },
	{ 0x1cc94, 0x4016 },
	{ 0x1cc95, 0x8c6c },
	{ 0x1cc96, 0x2b24 },
	{ 0x1cc97, 0x3c24 },
	{ 0x1cc98, 0x6435 },
	{ 0x1cc99, 0x1002 },
	{ 0x1cc9a, 0x2b24 },
	{ 0x1cc9b, 0x3c24 },
	{ 0x1cc9c, 0x643a },
	{ 0x1cc9d, 0x4025 },
	{ 0x1cc9e, 0x8a5a },
	{ 0x1cc9f, 0x1002 },
	{ 0x1cca0, 0x27c1 },
	{ 0x1cca1, 0x3011 },
	{ 0x1cca2, 0x1001 },
	{ 0x1cca3, 0xc7a0 },
	{ 0x1cca4, 0x100 },
	{ 0x1cca5, 0xc502 },
	{ 0x1cca6, 0x53ac },
	{ 0x1cca7, 0xc503 },
	{ 0x1cca8, 0xd5d5 },
	{ 0x1cca9, 0xc600 },
	{ 0x1ccaa, 0x2a6d },
	{ 0x1ccab, 0xc601 },
	{ 0x1ccac, 0x2a4c },
	{ 0x1ccad, 0xc602 },
	{ 0x1ccae, 0x111 },
	{ 0x1ccaf, 0xc60c },
	{ 0x1ccb0, 0x5900 },
	{ 0x1ccb1, 0xc710 },
	{ 0x1ccb2, 0x700 },
	{ 0x1ccb3, 0xc718 },
	{ 0x1ccb4, 0x700 },
	{ 0x1ccb5, 0xc720 },
	{ 0x1ccb6, 0x4700 },
	{ 0x1ccb7, 0xc801 },
	{ 0x1ccb8, 0x7f50 },
	{ 0x1ccb9, 0xc802 },
	{ 0x1ccba, 0x7760 },
	{ 0x1ccbb, 0xc803 },
	{ 0x1ccbc, 0x7fce },
	{ 0x1ccbd, 0xc804 },
	{ 0x1ccbe, 0x5700 },
	{ 0x1ccbf, 0xc805 },
	{ 0x1ccc0, 0x5f11 },
	{ 0x1ccc1, 0xc806 },
	{ 0x1ccc2, 0x4751 },
	{ 0x1ccc3, 0xc807 },
	{ 0x1ccc4, 0x57e1 },
	{ 0x1ccc5, 0xc808 },
	{ 0x1ccc6, 0x2700 },
	{ 0x1ccc7, 0xc809 },
	{ 0x1ccc8, 0x000 },
	{ 0x1ccc9, 0xc821 },
	{ 0x1ccca, 0x002 },
	{ 0x1cccb, 0xc822 },
	{ 0x1cccc, 0x014 },
	{ 0x1cccd, 0xc832 },
	{ 0x1ccce, 0x1186 },
	{ 0x1cccf, 0xc847 },
	{ 0x1ccd0, 0x1e02 },
	{ 0x1ccd1, 0xc013 },
	{ 0x1ccd2, 0xf341 },
	{ 0x1ccd3, 0xc01a },
	{ 0x1ccd4, 0x446 },
	{ 0x1ccd5, 0xc024 },
	{ 0x1ccd6, 0x1000 },
	{ 0x1ccd7, 0xc025 },
	{ 0x1ccd8, 0xa00 },
	{ 0x1ccd9, 0xc026 },
	{ 0x1ccda, 0xc0c },
	{ 0x1ccdb, 0xc027 },
	{ 0x1ccdc, 0xc0c },
	{ 0x1ccdd, 0xc029 },
	{ 0x1ccde, 0x0a0 },
	{ 0x1ccdf, 0xc030 },
	{ 0x1cce0, 0xa00 },
	{ 0x1cce1, 0xc03c },
	{ 0x1cce2, 0x01c },
	{ 0x1cce3, 0xc005 },
	{ 0x1cce4, 0x7a06 },
	{ 0x1cce5, 0x000 },
	{ 0x1cce6, 0x27c1 },
	{ 0x1cce7, 0x3011 },
	{ 0x1cce8, 0x1001 },
	{ 0x1cce9, 0xc620 },
	{ 0x1ccea, 0x000 },
	{ 0x1cceb, 0xc621 },
	{ 0x1ccec, 0x03f },
	{ 0x1cced, 0xc622 },
	{ 0x1ccee, 0x000 },
	{ 0x1ccef, 0xc623 },
	{ 0x1ccf0, 0x000 },
	{ 0x1ccf1, 0xc624 },
	{ 0x1ccf2, 0x000 },
	{ 0x1ccf3, 0xc625 },
	{ 0x1ccf4, 0x000 },
	{ 0x1ccf5, 0xc627 },
	{ 0x1ccf6, 0x000 },
	{ 0x1ccf7, 0xc628 },
	{ 0x1ccf8, 0x000 },
	{ 0x1ccf9, 0xc62c },
	{ 0x1ccfa, 0x000 },
	{ 0x1ccfb, 0x000 },
	{ 0x1ccfc, 0x2806 },
	{ 0x1ccfd, 0x3cb6 },
	{ 0x1ccfe, 0xc161 },
	{ 0x1ccff, 0x6134 },
	{ 0x1cd00, 0x6135 },
	{ 0x1cd01, 0x5443 },
	{ 0x1cd02, 0x303 },
	{ 0x1cd03, 0x6524 },
	{ 0x1cd04, 0x00b },
	{ 0x1cd05, 0x1002 },
	{ 0x1cd06, 0x2104 },
	{ 0x1cd07, 0x3c24 },
	{ 0x1cd08, 0x2105 },
	{ 0x1cd09, 0x3805 },
	{ 0x1cd0a, 0x6524 },
	{ 0x1cd0b, 0xdff4 },
	{ 0x1cd0c, 0x4005 },
	{ 0x1cd0d, 0x6524 },
	{ 0x1cd0e, 0x1002 },
	{ 0x1cd0f, 0x5dd3 },
	{ 0x1cd10, 0x306 },
	{ 0x1cd11, 0x2ff7 },
	{ 0x1cd12, 0x38f7 },
	{ 0x1cd13, 0x60b7 },
	{ 0x1cd14, 0xdffd },
	{ 0x1cd15, 0x00a },
	{ 0x1cd16, 0x1002 },
	{ 0x1cd17, 0x000 },


	{ 0, 0 },	// end code
};


void my_msleep(unsigned int msec)
{
	while (msec--) {
		usleep(1000);
	}
}
#ifdef _VTSS_
unsigned short read_mdio_data(char* map, unsigned char phyaddr, unsigned char devaddr, unsigned short regaddr)
{
	int timeout = 10000;
	mdio_req_entry_t entry;

	memset(&entry, 0, sizeof(mdio_req_entry_t));

//	printf("phyaddr: %x, devaddr: %x, regaddr: %x\n", phyaddr, devaddr, regaddr);

	// 1. address operation
	entry.field.opmode = MDIO_OPMODE_ADDR;
	entry.field.phy_addr = phyaddr;
	entry.field.dev_addr = devaddr;
	entry.field.data = regaddr;

	WRITE64(map, MCMR, entry.value.mcmr);
	WRITE64(map, MRWR, MRWR_WRITE_FLAG | MRWR_SW_REQUEST_FLAG);

//	printf("1.addr : mcmr : %016llx\n", entry.value.mcmr);

	while ((READ64(map, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
		usleep(1000);
		if (timeout-- == 0) {
			printf("err: Address operation failed (timeout)\n");
			return -1;		// timeout
		}
	}

	my_msleep(10);

	timeout = 10000;

	// 2. read data operation
	entry.field.opmode = MDIO_OPMODE_READ;
	entry.field.phy_addr = phyaddr;
	entry.field.dev_addr = devaddr;
	entry.field.data = 0;		// unused

	WRITE64(map, MCMR, entry.value.mcmr);
	WRITE64(map, MRWR, MRWR_READ_FLAG | MRWR_SW_REQUEST_FLAG);

//	printf("2.read : mcmr : %016llx\n", entry.value.mcmr);

	while ((READ64(map, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
		usleep(1000);
		if (timeout-- == 0) {
			printf("err: Read operation failed (timeout)\n");
			return -1;		// timeout
		}
	}

	my_msleep(10);

	// data read

//	printf("3.ret : mrdr : %016llx\n", (READ64(map, MRDR)));


	return (READ64(map, MRDR));
}

int write_mdio_data(unsigned char* map, unsigned char phyaddr, unsigned char devaddr, unsigned short regaddr, unsigned short value)
{
	int timeout = 10000;
	mdio_req_entry_t entry;

	memset(&entry, 0, sizeof(mdio_req_entry_t));

	//printf("phyaddr: %x, devaddr: %x, regaddr: %x value: %x\n", phyaddr, devaddr, regaddr, value);

	// 1. address operation
	entry.field.opmode = MDIO_OPMODE_ADDR;
	entry.field.phy_addr = phyaddr;
	entry.field.dev_addr = devaddr;
	entry.field.data = regaddr;

	WRITE64(map, MCMR, entry.value.mcmr);
	WRITE64(map, MRWR, MRWR_WRITE_FLAG | MRWR_SW_REQUEST_FLAG);

	//printf("1.addr : mcmr : %016llx\n", entry.value.mcmr);

	while ((READ64(map, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
		usleep(1000);
		if (timeout-- == 0) {
			printf("err: Address operation failed (timeout)\n");
			return -1;		// timeout
		}
	}

	my_msleep(10);

	timeout = 10000;

	// 2. write data operation
	entry.field.opmode = MDIO_OPMODE_WRITE;
	entry.field.phy_addr = phyaddr;
	entry.field.dev_addr = devaddr;
	entry.field.data = value;

	WRITE64(map, MCMR, entry.value.mcmr);
	WRITE64(map, MRWR, MRWR_WRITE_FLAG | MRWR_SW_REQUEST_FLAG);

	//printf("2.write : mcmr : %016llx\n", entry.value.mcmr);

	while ((READ64(map, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
		usleep(1000);
		if (timeout-- == 0) {
			printf("err: Write operation failed (timeout)\n");
			return -1;		// timeout
		}
	}

	my_msleep(10);

	return 0;
}
#else
unsigned short read_mdio_data(int fd, unsigned char phyaddr, unsigned char devaddr, unsigned short regaddr)
{
	int ret = 0;
	ioc_mdio_access_t mdio;
	mdio.phyaddr = phyaddr;
	mdio.devaddr = devaddr;
	mdio.regaddr = regaddr;

	ret = ioctl( fd, CMD_READ_MDIO, &mdio );
	BSL_CHECK_IOCTL( ret, ResultIoctlFailure );

	return mdio.value;
}

int write_mdio_data(int fd, unsigned char phyaddr, unsigned char devaddr, unsigned short regaddr, unsigned short value)
{
	int ret = 0;
	ioc_mdio_access_t mdio;
	mdio.phyaddr = phyaddr;
	mdio.devaddr = devaddr;
	mdio.regaddr = regaddr;
	mdio.value = value;

	ret = ioctl( fd, CMD_WRITE_MDIO, &mdio );
	BSL_CHECK_IOCTL( ret, ResultIoctlFailure );

	return ResultSuccess;
}
#endif
unsigned int read32_mdio_data(unsigned char* map, unsigned char phyaddr, unsigned char devaddr, unsigned short regaddr)
{
    int timeout = 10000;
    unsigned long long temp_read, temp_post_read_inc;
    unsigned int return_value;
    mdio_req_entry_t entry;

    memset(&entry, 0, sizeof(mdio_req_entry_t));

    // 1. address operation
    entry.field.opmode = MDIO_OPMODE_ADDR;
    entry.field.phy_addr = phyaddr;
    entry.field.dev_addr = devaddr;
    entry.field.data = regaddr;

    WRITE64(map, MCMR, entry.value.mcmr);
    WRITE64(map, MRWR, MRWR_SW_REQUEST_FLAG);

    while ((READ64(map, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
        usleep(1000);
        if (timeout-- == 0) {
            printf("err: Address operation failed (timeout)\n");
            return -1;      // timeout
        }
    }

    my_msleep(5);

    timeout = 10000;

    // 2. post read increament address operation
    entry.field.opmode = MDIO_OPMODE_POST_READ_INC_ADDR;
    entry.field.phy_addr = phyaddr;
    entry.field.dev_addr = devaddr;
    entry.field.data = 0;       // unused

    WRITE64(map, MCMR, entry.value.mcmr);
    WRITE64(map, MRWR, MRWR_SW_REQUEST_FLAG);

    while ((READ64(map, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
        usleep(1000);
        if (timeout-- == 0) {
            printf("err: Read operation failed (timeout)\n");
            return -1;      // timeout
        }
    }

    my_msleep(5);

    temp_post_read_inc = (READ64(map, MRDR));

    timeout = 10000;

    // 3. read data operation
    entry.field.opmode = MDIO_OPMODE_READ;
    entry.field.phy_addr = phyaddr;
    entry.field.dev_addr = devaddr;
    entry.field.data = 0;       // unused

    WRITE64(map, MCMR, entry.value.mcmr);
    WRITE64(map, MRWR, MRWR_SW_REQUEST_FLAG);

    //printf("2.read : mcmr : %016llx\n", entry.value.mcmr);

    while ((READ64(map, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
        usleep(1000);
        if (timeout-- == 0) {
            printf("err: Read operation failed (timeout)\n");
            return -1;      // timeout
        }
    }

    my_msleep(5);

    temp_read = (READ64(map, MRDR));

    return_value = (temp_read << 16) | (0xFFFF & temp_post_read_inc);

    // data read

    return (return_value);
}

static unsigned short calc_checksum( void* hp, int hlen )
{
	unsigned int sum = 0;
	unsigned short* shp = (unsigned short*)hp;

	while( hlen > 1 ) {
		sum += (unsigned int)*shp++;
		if( sum & 0x80000000 )
			sum = ( sum & 0xFFFF ) + ( sum >> 16 );
		hlen -= 2;
	}

	if( hlen )
		sum += (unsigned int)*(unsigned char*)shp;

	while(sum>>16)
		sum = ( sum & 0xFFFF ) + ( sum >> 16 );

	return (unsigned short)~sum;
}

static unsigned short get_l4_checksum( T_Protocol* proto, int* isoverride )
{	
	int i=0;
	struct pseudoip4
	{
		unsigned int   sip;
		unsigned int   dip;
		unsigned char  zero;
		unsigned char  protocol;
		unsigned short length;
	}__attribute__((packed));

	struct pseudoip6
	{
		unsigned char sip[16];
		unsigned char dip[16];
		unsigned int  length;
		unsigned char zero[3];
		unsigned char nexthdr;
	}__attribute__((packed));

	T_Ip4* ip4hp;
	T_Ip6* ip6hp;

	T_PDR_TCP* pdrtcp = NULL;
	T_PDR_UDP* pdrudp = NULL;
	T_PDR_ICMP* pdricmp = NULL;
	T_ChecksumTuple* cstuple = NULL;

	unsigned short chksum = 0;
	int isvalid = TRUE;

	//0. get checksumtype
	if( proto->l4.protocol == ProtocolTCP ) {
		T_PDR_TCP* pdrtcp = (T_PDR_TCP*)proto->l4.pdr;
		if( pdrtcp ) {
			if( pdrtcp->checksum.type == ChecksumOverride ) {
				*isoverride = TRUE;
				return pdrtcp->checksum.value;
			}
			else if( pdrtcp->checksum.type == ChecksumInvalid ) 
				isvalid = FALSE;

			cstuple = &pdrtcp->checksum;
		}
	}
	else if( proto->l4.protocol == ProtocolUDP ) {
		T_PDR_UDP* pdrudp = (T_PDR_UDP*)proto->l4.pdr;
		if( pdrudp ) {
			if( pdrudp->checksum.type == ChecksumOverride ) {
				*isoverride = TRUE;
				return pdrudp->checksum.value;
			}
			else if( pdrudp->checksum.type == ChecksumInvalid ) 
				isvalid = FALSE;

			cstuple = &pdrudp->checksum;
		}
	}
	else if( proto->l4.protocol == ProtocolICMP ) {
		T_PDR_ICMP* pdricmp = (T_PDR_ICMP*)proto->l4.pdr;
		if( pdricmp ) {
			if( pdricmp->checksum.type == ChecksumOverride ) {
				*isoverride = TRUE;
				return pdricmp->checksum.value;
			}
			else if( pdricmp->checksum.type == ChecksumInvalid ) 
				isvalid = FALSE;

			cstuple = &pdricmp->checksum;
		}
	}
	
	//request by GUI.
	*isoverride = FALSE;
	return cstuple ? cstuple->value : 0;

	//0. make pseudohdr
	int pseudo_offset = 
		( proto->l3.protocol == ProtocolIP4 ) || ( proto->l3.protocol == ProtocolIP6 ) ?
		proto->l3.offset :
		( proto->l3.protocol == ProtocolIP6OverIP4 ) ? 
		proto->l3.offset + get_ip4hlen( proto ) :
		( proto->l3.protocol == ProtocolIP4OverIP6 ) ? 
		proto->l3.offset + get_ip6hlen( proto ) : 0;

	ip4hp = 
		( proto->l3.protocol == ProtocolIP4 ) || 
		( proto->l3.protocol == ProtocolIP4OverIP6 ) ?
		(T_Ip4*)(proto->header + pseudo_offset ) : NULL;

	ip6hp = 
		( proto->l3.protocol == ProtocolIP6 ) || 
		( proto->l3.protocol == ProtocolIP6OverIP4 ) ?
		(T_Ip6*)(proto->header + pseudo_offset ) : NULL;

	//1. get framesize
	T_FrameSize* frame = (T_FrameSize*)&proto->fi;
	int framelen = 
		frame->fsizeSpec == FrameSizeFixed ?
		frame->sizeOrStep :
		frame->fsizeSpec == FrameSizeIncrement ?
		frame->fsizeMin :
		frame->fsizeSpec == FrameSizeRandom ?
		frame->fsizeValueRand[0] : 0;

	if( framelen )
		framelen -= proto->l4.offset;

	//2. fill payload pattern
	int pseudolen = 0;
	unsigned char data[SIZE_MAX_PAYLOAD] = {0,};
	int validsize = proto->fi.pattern.validSize;
	unsigned char* pload = (unsigned char*)proto->fi.pattern.payload;
	unsigned char* payload = 
		proto->l4.protocol == ProtocolTCP ?  data + pseudolen + sizeof(T_Tcp) :
		proto->l4.protocol == ProtocolUDP ?  data + pseudolen + sizeof(T_Udp) :
		proto->l4.protocol == ProtocolICMP ?  data + pseudolen + sizeof(T_Icmp) :
		data + pseudolen;

	for( i=0; i<framelen; i+=validsize ) {
		memcpy( payload+i, pload, validsize );
	}

	//3. fill psudoheader
	struct pseudoip4 pip4 = {0,};
	struct pseudoip6 pip6 = {0,};

	if( ip4hp ) {
		pip4.sip = ip4hp->sip;
		pip4.dip = ip4hp->dip;
		pip4.protocol = ip4hp->proto;
		pip4.length = framelen;
		pseudolen = sizeof(struct pseudoip4);
		memcpy( data, &pip4, pseudolen );
	}
	else if( ip6hp ) {
		memcpy( pip6.sip, ip6hp->sip, 16 );
		memcpy( pip6.dip, ip6hp->dip, 16 );
		pip6.length = framelen;
		pip6.nexthdr = ip6hp->nextheader;
		pseudolen = sizeof(struct pseudoip6);
		memcpy( data, &pip6, pseudolen );
	}

	//4. copy l4
	if( proto->l4.protocol == ProtocolTCP ) {
		T_Tcp* ptcp = (T_Tcp*)( proto->header + proto->l4.offset );
		memcpy( data+pseudolen, ptcp, sizeof(T_Tcp) );
		ptcp = (T_Tcp*)(data+pseudolen);
		ptcp->checksum = 0x0000;
	}
	else if( proto->l4.protocol == ProtocolUDP ) {
		T_Udp* pudp = (T_Udp*)( proto->header + proto->l4.offset );
		memcpy( data+pseudolen, pudp, sizeof(T_Udp) );
		pudp = (T_Udp*)(data+pseudolen);
		pudp->checksum = 0x0000;
	}
	else if( proto->l4.protocol == ProtocolICMP ) {
		T_Icmp* picmp = (T_Icmp*)( proto->header + proto->l4.offset );
		memcpy( data+pseudolen, picmp, sizeof(T_Icmp) );
		picmp = (T_Icmp*)(data+pseudolen);
		picmp->checksum = 0x0000;
	}

	chksum = calc_checksum( (void*)payload, pseudolen + framelen );
	return isvalid ? chksum : chksum + 0x0101;
}
// 2017.2.27 by dgjung
static unsigned short getLastIp4ID( T_CustomIntegerTuple* tuple )
{
    BSL_CHECK_NULL( tuple, 0 );
    BSL_CHECK_NULL( tuple->repeat, 0 );

	unsigned long long bound = 0ll;
	unsigned long long start = (unsigned long long)tuple->value;
	unsigned long long max = 1ull << 16; //because of 2bytes

	int iscont = 
		tuple->mode == CustomIntegerModeContIncrement ? 1 :
		tuple->mode == CustomIntegerModeContDecrement ? 1 :
		0;
	int isincre = 
		tuple->mode == CustomIntegerModeIncrement ? 1 :
		tuple->mode == CustomIntegerModeContIncrement ? 1 :
		0;
	int isdecre = 
		tuple->mode == CustomIntegerModeDecrement ? 1 :
		tuple->mode == CustomIntegerModeContDecrement ? 1 :
		0;

	if( iscont ) return 0;

	int shift = 0;
	if( tuple->step != 0 )
		while( ! ( tuple->step & ( 1 << shift ) ) ) { shift++; }

	unsigned long long max_repeat = max / ( 1ull << shift );
	unsigned long long repeat;

	BSL_TRACE(("%s:DEBUG max_repeat %d input_repeat %d shift %d\n", 
				__func__, max_repeat, tuple->repeat, shift ));

	repeat = (unsigned long long)tuple->repeat > max_repeat ? max_repeat : (unsigned long long)tuple->repeat;

	bound = isincre ? 
			start + ( (max-1ull) & ( tuple->step * ( repeat - 1ull ) ) ) :
			start > ( (max-1ull) & ( tuple->step * ( repeat - 1ull ) ) ) ?
			start - ( (max-1ull) & ( tuple->step * ( repeat - 1ull ) ) ) :
			start + max - ( (max-1ull) & ( tuple->step * ( repeat - 1ull ) ) );

	bound &= ( max - 1ull );

	BSL_TRACE(("%s: bound = %11d\n", __func__, bound ));

	return bound; 
}

static unsigned short getLastTcpUdpPort( T_CustomIntegerTuple* tuple )
{
    BSL_CHECK_NULL( tuple, 0 );
    BSL_CHECK_NULL( tuple->repeat, 0 );

	unsigned long long bound = 0ll;
	unsigned long long start = (unsigned long long)tuple->value;
	unsigned long long max = 1ull << 16; //because of 2bytes

	int iscont = 
		tuple->mode == CustomIntegerModeContIncrement ? 1 :
		tuple->mode == CustomIntegerModeContDecrement ? 1 :
		0;
	int isincre = 
		tuple->mode == CustomIntegerModeIncrement ? 1 :
		tuple->mode == CustomIntegerModeContIncrement ? 1 :
		0;
	int isdecre = 
		tuple->mode == CustomIntegerModeDecrement ? 1 :
		tuple->mode == CustomIntegerModeContDecrement ? 1 :
		0;

	if( iscont ) return 0;

	int shift = 0;
	if( tuple->step != 0 )
		while( ! ( tuple->step & ( 1 << shift ) ) ) { shift++; }

	unsigned long long max_repeat = max / ( 1ull << shift );
	unsigned long long repeat;

	BSL_TRACE(("%s:DEBUG max_repeat %d input_repeat %d shift %d\n", 
				__func__, max_repeat, tuple->repeat, shift ));

	repeat = (unsigned long long)tuple->repeat > max_repeat ? max_repeat : (unsigned long long)tuple->repeat;

	bound = isincre ? 
			start + ( (max-1ull) & ( tuple->step * ( repeat - 1ull ) ) ) :
			start > ( (max-1ull) & ( tuple->step * ( repeat - 1ull ) ) ) ?
			start - ( (max-1ull) & ( tuple->step * ( repeat - 1ull ) ) ) :
			start + max - ( (max-1ull) & ( tuple->step * ( repeat - 1ull ) ) );

	bound &= ( max - 1ull );

	BSL_TRACE(("%s: bound = %11d\n", __func__, bound ));

	return bound; 
}

static void stop_dma( char* map, int cardid, int portid )
{
	unsigned long long ccfr;
#ifdef DMA_PLDA
	unsigned long long cxsr;

	cxsr = READ64( map, portid ? C3SR : C0SR );
	cxsr |= ( 1ll << I_CnSR_ABORT );
	WRITE64( map, portid ? C3SR : C0SR, cxsr );
#else
    ccfr = 0x00;
    printf("WRITE - CCFR[%d] %llx\n", portid, ccfr );
    WRITE64( map, OFFSET_REGISTER_PORT(portid)+CCFR, ccfr );
#endif
}

typedef struct t_dma_arg {
	int fd;
	char* map;
	int cardid;
	int portid;
	unsigned int size;
	ioc_io_buffer_t* pciBuffer;
	void* pciUserp; 
	EnumCaptureMode mode;
} T_DMA_ARG;

static int start_dma( int fd, char* map, int cardid, int portid, unsigned int size, EnumCaptureMode mode ) 
{
    int ret;

    /************************************
    *         For DMA Resources
    ************************************/
	T_DMA_ARG* dma_arg;
    ioc_io_buffer_t* pciBuffer;
    void* pciUserp = NULL;    
	pthread_t thrid;

	pciBuffer = (ioc_io_buffer_t*)malloc( sizeof(ioc_io_buffer_t) );
    BSL_CHECK_NULL( pciBuffer, -1 );
	
    pciBuffer->Size = size;
    ret = ioctl( fd, CMD_PHY_MEM_ALLOC, pciBuffer );
    BSL_CHECK_IOCTL_GOTO( ret, close_fd );

    int file;
    file = open("/dev/mem", O_RDWR|O_SYNC);
    if( ( pciUserp = mmap( 0, pciBuffer->Size, PROT_READ | PROT_WRITE, MAP_SHARED, file, pciBuffer->PhysicalAddr )) == MAP_FAILED ) {
        perror("mmap");
        goto close_fd;
    }
    //memset( pciUserp, 0, pciBuffer->Size );

    printf("UAddr(%llx) PAddr(%llx) VAddr(%llx) Size(%d)\n",
        pciUserp,
        pciBuffer->PhysicalAddr,
        pciBuffer->CpuPhysical,
        pciBuffer->Size );

	dma_arg = (T_DMA_ARG*)malloc( sizeof(T_DMA_ARG) );
	BSL_CHECK_NULL( dma_arg, -1 );

	dma_arg->fd = fd;
	dma_arg->map = map;
	dma_arg->cardid = cardid;
	dma_arg->portid = portid;
	dma_arg->size = size;
	dma_arg->pciBuffer = pciBuffer;
	dma_arg->pciUserp = pciUserp;
	dma_arg->mode = mode;

    ret = pthread_create( thrid, NULL, dma_thread, (void*)dma_arg );
    if( ret == 0 ) {
		sleep(1);
		return ret;  //success
	}

    printf("%s: Error.. pthread_create ret %d\n", __func__, ret );

close_fd:
    domunmap( fd, &map );    

    close(fd);
	free(pciBuffer);
	free(dma_arg);

    return -1; 
}

static void* dma_thread( void* arg )
{
    int ret;
	T_DMA_ARG* dma_arg = (T_DMA_ARG*)arg;
	BSL_CHECK_NULL( dma_arg, NULL );

    ret = pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL );
    BSL_CHECK_RESULT( ret, NULL );

    ret = pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL );
    BSL_CHECK_RESULT( ret, NULL );

    bsl_process_dma( dma_arg );

    pthread_exit( NULL );
    return NULL;
}

void bsl_process_dma( T_DMA_ARG* dma_arg )
{
    /**************************************************************
     * For File
     *************************************************************/

    int   f_filename = 0;
    const char* dirname = "/var/www/html/BSL";
    const char* def_filename = "bslcaps";
    const char* file_ext = "bcap";
    char filename[256] = {0,};
    char extfilename[256] = {0,};
    char user_filename[256] = {0,};
    FILE* pcapfp;
    int fileno = -1;

    if( access( dirname, R_OK ) != 0 ) {
        mkdir( dirname, 0755 );
        printf("Directory %s has been created....\n", dirname );
    }
   
    if( f_filename ) {
        strcpy( filename, user_filename );
    }
    else {        
		strcpy( filename, def_filename );
    }
   
    sprintf(extfilename, "%s/%s.%d.%s", dirname, filename, time(NULL), file_ext );
    printf("--> Captured Filename = %s\n", extfilename );
   
    do {
		int flags = O_RDWR | O_CREAT | O_TRUNC | O_LARGEFILE; // | O_DIRECT;
   
        fileno = open( extfilename, flags, 0667 );
        if( ( fileno == -1 ) ) {
            printf("%s: open error - %s\n",__FUNCTION__, extfilename );
            goto close_func;
        }
    } while(0);

	int i = 0;
	int index_count = 0;
    unsigned long long cxar = 0ll;
    unsigned long long ini_cxar = 0ll;
    unsigned long long end_cxar = 0ll;
    unsigned long long cxsr = 0ll;
    unsigned long long ccfr = 0ll;
    int CXAR = dma_arg->portid ? C3AR : C0AR;
    int CXSR = dma_arg->portid ? C3SR : C0SR;
	unsigned int writesize = dma_arg->pciBuffer->Size;
	unsigned int written;
	char cport = dma_arg->portid ? '3' : '0';
   
    cxar = dma_arg->pciBuffer->PhysicalAddr;
    cxsr = dma_arg->pciBuffer->Size | ( 0ll << 33 ) |
        ( 0ll << 36 ) | ( 1ll << 35 ); //36 : SGL, 35 : Abort, 34 : Run, 33 : RAM mode
    ccfr = 0x00;
    printf("WRITE - CCFR[%d] %llx\n", dma_arg->portid, ccfr );
    WRITE64( dma_arg->map, OFFSET_REGISTER_PORT(dma_arg->portid)+CCFR, ccfr );
   
    printf("WRITE - C%cAR %llx\n", cport, cxar );
    WRITE64( dma_arg->map, CXAR, cxar );
#ifdef DMA_PLDA
    WRITE64( dma_arg->map, CXSR, cxsr );
    printf("WRITE - C%cSR %llx\n", cport, cxsr );
   
    cxsr = dma_arg->pciBuffer->Size | ( 0ll << 33 ) |
        ( 0ll << 36 ) | ( 1ll << 34 ); //36 : SGL, 34 : Run, 33 : RAM mode
    printf("WRITE - C%cSR %llx\n", cport, cxsr );
    WRITE64( dma_arg->map, CXSR, cxsr );
#endif

    ccfr = 1 << 0;
	if( dma_arg->mode == 0 ) ccfr = 0xF;
	else {
		if( dma_arg->mode & ( 1 << 0 ) ) ccfr |= ( 1 << 1 );
		if( dma_arg->mode & ( 1 << 1 ) ) ccfr |= ( 1 << 2 );
		if( dma_arg->mode & ( 1 << 2 ) ) ccfr |= ( 1 << 3 );
	}
    printf("WRITE - CCFR[%d] %llx\n", dma_arg->portid, ccfr );
    WRITE64( dma_arg->map, OFFSET_REGISTER_PORT(dma_arg->portid)+CCFR, ccfr );

#ifdef DMA_PLDA

	do {
		cxar = READ64( dma_arg->map, CXAR );
		cxsr = READ64( dma_arg->map, CXSR );
		printf("C%cSR %016llx C%cAR %016llx (count %d)\n", 
				cport, cxsr, cport, cxar, i++ );
		if( cxsr & ( 1ll << 34 ) ) sleep(1);
		else break;
	} while(1);

	cxsr = READ64( dma_arg->map, CXSR );
	writesize -= (unsigned int)cxsr;
	if( writesize > dma_arg->pciBuffer->Size ) writesize = dma_arg->pciBuffer->Size;

	printf("%s: portid %d writesize 0x%08X(%ud) C%cSR %016llx\n", __func__, dma_arg->portid, writesize, writesize, cport, cxsr );

#else

	ini_cxar = cxar;
	end_cxar = ini_cxar + dma_arg->pciBuffer->Size;
	do {
		cxar = READ64( dma_arg->map, CXAR );
		ccfr = READ64( dma_arg->map, OFFSET_REGISTER_PORT(dma_arg->portid)+CCFR );
		printf("C%cAR %016llx ccfr %016llx (count %d)\n", cport, cxar, ccfr, i++ );
		if( cxar >= end_cxar ) break;
		else if( ccfr == 0 ) break;
		else sleep(1);
	} while(1);

	cxar = READ64( dma_arg->map, CXAR );
	writesize = cxar - ini_cxar;

	printf("%s: cardid %d portid %d writesize 0x%08X(%ud) C%cAR %016llx\n", __func__, dma_arg->portid, dma_arg->portid, writesize, writesize, cport, cxar );
#endif

	WRITE64( dma_arg->map, OFFSET_REGISTER_PORT(dma_arg->portid)+CCFR, 0ll );

	written = write( fileno, dma_arg->pciUserp, writesize );
	if( written != writesize ) {
		printf("%s:%d written %d is different from writesize %d\n", 
				__FUNCTION__, __LINE__, written, writesize );            
	}

	printf("%s: written %d\n", __func__, written );

	munmap( &dma_arg->pciUserp, dma_arg->size );
    ioctl( dma_arg->fd, CMD_PHY_MEM_FREE, dma_arg->pciBuffer ); 
        
    domunmap( dma_arg->fd, &dma_arg->map );    
	if( fileno != -1 ) close( fileno );
        
    close(dma_arg->fd);	

	bsl_toPcap( extfilename, dma_arg->cardid, dma_arg->portid );
close_func:
#ifdef REMOVE_BCAP
	remove( extfilename ); 
#endif
	free( dma_arg->pciBuffer );
	free( dma_arg );
}

EnumResultCode
bsl_device_setRegister( int fd, EnumCommandReg command, unsigned int addr, unsigned long long* value )
{
	char* map = NULL;
	int ret;

	BSL_DEV(("%s: Enter. command %d addr %X *value %llX\n", __func__, command, addr, *value ));

	ret = dommap( fd, &map );
	BSL_CHECK_RESULT( ret, ret );

	addr >>= 3; //normalization

	if( command == CommandRegRead ) {
		*value = READ64( map, addr );
	}
	else if( command == CommandRegWrite ) {
		WRITE64( map, addr, *value );
	}

	domunmap( fd, &map );
	return ResultSuccess;
}
