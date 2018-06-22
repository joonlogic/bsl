/********************************************************************
 *  FILE   : bsl_api.h
 *  Author : joon
 *
 *  Library : 
 *  Export Function :
 *
 ********************************************************************
 *                    Change Log
 *
 *    Date			Author			Note
 *  -----------   -----------  ----------------------------------   
 *  2013.11.13       joon        This File is written first.
 *  2016.04.09       joon        01.00.00 release first
 *  2016.04.17       joon        01.00.01 add message 101
 *  2016.05.10       joon        01.00.02 touch message 108 wmcr
 *  2018.05.10       joon        01.01.00 add C extension for python
 *								         
 ********************************************************************/

#ifndef BSL_API_H
#define BSL_API_H

#include "bsl_type.h"
#include "bsl_proto.h"

/* Move to Makefile
#define VERSION_BSL_API_STR                 "01.01.00"
#define VERSION_BSL_API_INT                 0x00010100
*/

#ifdef _TARGET_
#define OPEN_DEVICE(cardid)                   \
	cardid == 0 ?                             \
	open("/dev/bsl_ctl", O_RDWR, (mode_t)0600) : \
	cardid == 1 ?                             \
	open("/dev/bsl_ctl-1", O_RDWR, (mode_t)0600) : \
	cardid == 2 ?                             \
	open("/dev/bsl_ctl-2", O_RDWR, (mode_t)0600) : \
	cardid == 3 ?                             \
	open("/dev/bsl_ctl-3", O_RDWR, (mode_t)0600) : -1
#else
#define OPEN_DEVICE(cardid)             100
#endif

#define BSL_CHECK_DEVICE(fd)            \
	if( fd < 0 ) { \
		fprintf( stderr, "%s> device file open failure. fd %d\n", \
				__func__, fd ); \
		return -1; \
	}

extern EnumResultCode
bsl_getNumberOfCards( int* nCards ),
bsl_getVersionInfo( int cardid, T_SystemVersion* ver ),
bsl_getLinkStatus( int cardid, T_Card* card ),
bsl_getLinkStats( int cardid, int portid, T_LinkStats* stat ),
bsl_setPortMode( int cardid, int portid, EnumPortOpMode mode ),
bsl_setPortActive( int cardid, int portid, EnumPortActive enable ),
bsl_setLatency( int cardid, int portid, int latency_enable, int sequence_enable, int signature_enable ),
bsl_setCaptureStartStop( int cardid, int portid, EnumCaptureMode mode, unsigned int size, int start ),
bsl_setControlCommand( int portsel, int cardid, int portid, int streamid, EnumCommand command, unsigned long long clocks, unsigned int netmode, unsigned long long mac, unsigned int ip ),
bsl_enableStream( int cardid, int portid, int streamid, int enable ),
bsl_setRegister( int cardid, EnumCommandReg command, unsigned int addr, unsigned long long* value ),
bsl_setStreamDetail( int cardid, int portid, int streamid, int groupid,T_Stream* stream, T_Protocol* proto );

#endif //BSL_API_H
