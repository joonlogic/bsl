/********************************************************************
 *  FILE   : bsl_msgif.h
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
 *								         
 ********************************************************************/
#ifndef BSL_MSGIF_H
#define BSL_MSGIF_H

#include <pthread.h>
#include "common.h"
#include "bsl_def.h"
#include "bsl_type.h"

#pragma pack(1)

typedef struct {
	pthread_t             thrid;
	pid_t                 pid;
	int                   cpuid;
	pthread_mutex_t       mutex;
} T_Thread;


#define SIZE_MAX_MSGIF_ID              100
#define VALUE_MAX_MSGIF_ID             200
#define VALUE_MSGID_OFFSET             100
#define VALUE_MSGIF_DELIM              0x01010000

#define MSGIF_OFFSET_DELIM             0
#define MSGIF_OFFSET_ID                1
#define MSGIF_OFFSET_TYPE              2
#define MSGIF_OFFSET_LENGTH            3
#define MSGIF_OFFSET_NRECORD           4
#define MSGIF_OFFSET_BODY              5

#define MSGIF_TYPE_REQUEST             1
#define MSGIF_TYPE_REPLY               2
#define MSGIF_TYPE_NOTIFY              3

#define MSGIF_GET_LENGTH( msgptr )     \
	ntohl(*(((unsigned int*)(msgptr))+MSGIF_OFFSET_LENGTH))
#define MSGIF_GET_BODY_PTR( msgptr )     \
	(((unsigned int*)(msgptr))+MSGIF_OFFSET_BODY)
#define MSGIF_GET_DELIM( msgptr )     \
	ntohl(*(((unsigned int*)(msgptr))+MSGIF_OFFSET_DELIM))
#define MSGIF_GET_ID( msgptr )     \
	ntohl(*(((unsigned int*)(msgptr))+MSGIF_OFFSET_ID))
#define MSGIF_GET_TYPE( msgptr )     \
	ntohl(*(((unsigned int*)(msgptr))+MSGIF_OFFSET_TYPE))
#define MSGIF_GET_NRECORD( msgptr )     \
	ntohl(*(((unsigned int*)(msgptr))+MSGIF_OFFSET_NRECORD))

#define MSGIF_SET_DELIM( msgptr )     \
	*(((unsigned int*)(msgptr))+MSGIF_OFFSET_DELIM)=htonl(VALUE_MSGIF_DELIM)
#define MSGIF_SET_ID( msgptr, id )     \
	*(((unsigned int*)(msgptr))+MSGIF_OFFSET_ID)=htonl(id)
#define MSGIF_SET_TYPE( msgptr, type )     \
	*(((unsigned int*)(msgptr))+MSGIF_OFFSET_TYPE)=htonl(type)
#define MSGIF_SET_LENGTH( msgptr, length )     \
	*(((unsigned int*)(msgptr))+MSGIF_OFFSET_LENGTH)=htonl(length)
#define MSGIF_SET_NRECORD( msgptr, nrecord )     \
	*(((unsigned int*)(msgptr))+MSGIF_OFFSET_NRECORD)=htonl(nrecord)

#define BSLSWAP32(val) \
	((unsigned int)((((unsigned int)(val) & (unsigned int)0x000000ffU) << 24) | \
		(((unsigned int)(val) & (unsigned int)0x0000ff00U) <<  8) | \
		(((unsigned int)(val) & (unsigned int)0x00ff0000U) >>  8) | \
		(((unsigned int)(val) & (unsigned int)0xff000000U) >> 24)))

//BSLSWAP40
//INPUT  : AABBCCDD_EEFFFFFF
//OUTPUT : EEDDCCBB_AAFFFFFF
#define BSLSWAP40(val) \
	((unsigned long long)( \
		(((unsigned long long)(val) & (unsigned long long)0x00000000FF000000ULL) << 32) | \
		(((unsigned long long)(val) & (unsigned long long)0x000000FF00000000ULL) << 16) | \
		(((unsigned long long)(val) & (unsigned long long)0x0000FF0000000000ULL) << 0 ) | \
		(((unsigned long long)(val) & (unsigned long long)0x00FF000000000000ULL) >> 16) | \
		(((unsigned long long)(val) & (unsigned long long)0xFF00000000000000ULL) >> 32) | \
		(((unsigned long long)(val) & (unsigned long long)0x0000000000FFFFFFULL) )))


#define BSLSWAP64(val) \
	((unsigned long long)((((unsigned long long)(val) & (unsigned long long)0x000000ffULL) << 56) | \
		(((unsigned long long)(val) & (unsigned long long)0x0000ff00ULL) << 40) | \
		(((unsigned long long)(val) & (unsigned long long)0x00ff0000ULL) << 24) | \
		(((unsigned long long)(val) & (unsigned long long)0xff000000ULL) << 8 ) | \
		(((unsigned long long)(val) & (unsigned long long)0xff00000000ULL) >> 8) | \
		(((unsigned long long)(val) & (unsigned long long)0xff0000000000ULL) >> 24) | \
		(((unsigned long long)(val) & (unsigned long long)0xff000000000000ULL) >> 40) | \
		(((unsigned long long)(val) & (unsigned long long)0xff00000000000000ULL) >> 56)))

typedef struct {
	unsigned int          delim;
	unsigned int          id;
	unsigned int          type;
	unsigned int          length;
	unsigned int          nrecord;
} T_MSGIF_HDR;

typedef struct {
	T_MSGIF_HDR           hdr;
	unsigned int          result;
} T_MSGIF_SETCMD_RESP;

//NYI Chassis Information
typedef struct {
} T_MSGIF_NYI_REQ_UNIT;

typedef struct {
	unsigned int          cardid;
	unsigned int          nports;
	unsigned int          linkSpeed; // 0:10G 1:1G
} T_MSGIF_NYI_RESP_UNIT;

typedef T_MSGIF_HDR       T_MSGIF_NYI_REQ;
typedef T_MSGIF_HDR       T_MSGIF_NYI_RESP;

//100 Version Information
typedef struct {
} T_MSGIF_100_REQ_UNIT;

typedef struct {
	unsigned int          api;
	unsigned long long    fpga;
} T_MSGIF_100_RESP_UNIT;

typedef T_MSGIF_HDR       T_MSGIF_100_REQ;
typedef T_MSGIF_HDR       T_MSGIF_100_RESP;


//101 Link Status
typedef struct {
	unsigned int          cardid;
} T_MSGIF_101_REQ_UNIT;

typedef struct {
	unsigned int          DBGR;
	unsigned int          link0; // 1:Up 2:Down
	unsigned int          link1; // 1:Up 2:Down
} T_MSGIF_101_RESP_UNIT;

typedef T_MSGIF_HDR       T_MSGIF_101_REQ;
typedef T_MSGIF_HDR       T_MSGIF_101_RESP;

//102 Port Mode ( Normal/Interleave )
typedef struct {
	unsigned int          cardid;
	unsigned int          portid;
	unsigned int          portmode;
} T_MSGIF_102_REQ_UNIT;

typedef T_MSGIF_HDR       T_MSGIF_102_REQ;
typedef T_MSGIF_SETCMD_RESP T_MSGIF_102_RESP;

//103. Statistics
typedef struct {
	unsigned int          cardid;
	unsigned int          portid;
} T_MSGIF_103_REQ_UNIT;

typedef struct {
	unsigned int          cardid;
	unsigned int          portid;
	unsigned int          timestamp;
	unsigned int          frameSentRate;
	unsigned int          validFrameRxRate;
	unsigned int          byteSentRate;
	unsigned int          byteRxRate;
	unsigned int          crcErrorFrameRxRate;
	unsigned int          fragmentErrorFrameRxRate;
	unsigned int          crcErrorByteRxRate;
	unsigned int          fragmentErrorByteRxRate;
	unsigned long long    frameSent;
	unsigned long long    byteSent;
	unsigned long long    validFrameReceived;
	unsigned long long    validByteReceived;
	unsigned long long    fragments;
	unsigned long long    fragmentsByteReceived;
	unsigned long long    crcErrors;
	unsigned long long    vlanTaggedFrame;
	unsigned long long    crcErrorByteReceived;
	unsigned long long    vlanByteReceived;
	unsigned int 		  latencyMaximum;
	unsigned int		  latencyMinimum;
	unsigned int    	  AverageLatIntPart;
	unsigned int    	  AverageLatFracPart;
	unsigned long long	  seqErrorPacketCount;
	unsigned long long	  undersizeFrameCount;
	unsigned long long	  oversizeFrameCount;
	unsigned long long	  signatureFrameCount;					// 2016.12.26 by dgjung
} __attribute__ ((packed)) T_MSGIF_103_RESP_UNIT;

typedef T_MSGIF_HDR          T_MSGIF_103_REQ;
typedef T_MSGIF_HDR          T_MSGIF_103_RESP;

//104. Set Port Enable/Disable
typedef struct {
	unsigned int          cardid;
	unsigned int          portid;
	unsigned int          enable;
} T_MSGIF_104_REQ_UNIT;

typedef T_MSGIF_HDR          T_MSGIF_104_REQ;
typedef T_MSGIF_HDR          T_MSGIF_104_RESP;
typedef struct {
	unsigned int          result;
} T_MSGIF_104_RESP_UNIT;

//105. Packet Capture
typedef struct {
	unsigned int          cardid;
	unsigned int          portid;
	EnumCaptureMode       mode;
	unsigned int          size;
	unsigned int          start;
} T_MSGIF_105_REQ_UNIT;

typedef T_MSGIF_HDR          T_MSGIF_105_REQ;
typedef T_MSGIF_HDR          T_MSGIF_105_RESP;
//typedef T_MSGIF_SETCMD_RESP  T_MSGIF_105_RESP;
typedef struct {
	unsigned int          result;
} T_MSGIF_105_RESP_UNIT;

//106. Latency Enable/Disable
typedef struct {
	unsigned int          cardid;
	unsigned int          portid;
	unsigned int          latency_enable;
	unsigned int          sequence_enable;
	unsigned int          signature_enable;						// 2016. 12.26 by dgjung
} T_MSGIF_106_REQ_UNIT;

typedef T_MSGIF_HDR         T_MSGIF_106_REQ;
typedef T_MSGIF_HDR         T_MSGIF_106_RESP;
//typedef T_MSGIF_SETCMD_RESP T_MSGIF_106_RESP;
typedef struct {
	unsigned int          result;
} T_MSGIF_106_RESP_UNIT;

//108. Command
typedef struct {
	unsigned int          portsel;
	unsigned int          cardid;
	unsigned int          portid;
	unsigned int          streamid;
	EnumCommand           command;
	unsigned long long    timesec;
	unsigned int          netmode;
	unsigned long long    mac;
	unsigned int          ip;
} T_MSGIF_108_REQ_UNIT;

typedef T_MSGIF_HDR          T_MSGIF_108_REQ;
typedef T_MSGIF_HDR         T_MSGIF_108_RESP;
typedef struct {
	unsigned int          result;
} T_MSGIF_108_RESP_UNIT;

//109. Stream General Information
typedef struct {
	unsigned int          cardid;
	unsigned int          portid;
	unsigned int          streamid;
	unsigned int          enable;
} T_MSGIF_109_REQ_UNIT;

typedef T_MSGIF_HDR          T_MSGIF_109_REQ;
typedef T_MSGIF_SETCMD_RESP  T_MSGIF_109_RESP;

//110. Stream Detail Information
typedef struct {
	EnumStreamControl     control;
	unsigned int          returnToId;
	unsigned int          loopCount;
	unsigned long long    pktsPerBurst;
	unsigned int          burstsPerStream;
	EnumRateControl       rateControl;
	unsigned int          rateControlIntPart;
	unsigned int          rateControlFracPart;
	unsigned int          startTxDelay;
	unsigned int          interBurstGapIntPart;
	unsigned int          interBurstGapFracPart;
	unsigned int          interStreamGapIntPart;
	unsigned int          interStreamGapFracPart;
	unsigned int          ifg;
} T_TUPLE_STREAM_CTL;

typedef struct {
	EnumSpecFrameSize     fsizeSpec;
	unsigned int          sizeOrStep;
	unsigned int          fsizeMin;
	unsigned int          fsizeMax;
//	unsigned char         fsizeValueRand[0];
} T_TUPLE_FRAME_SIZE;
	
typedef struct {
	EnumFrameCrcType      crc;
	EnumFrameDataType     dataPatternType;
	unsigned int          poffset;
	unsigned int          pvalidSize;
	unsigned char         pattern[0];
} T_TUPLE_PLOAD_INFO;

typedef struct {
	unsigned int          cardid;
	unsigned int          portid;
	unsigned int          streamid;
	unsigned int          groupid;						// 2017.7.15 by dgjung
	unsigned int          enable;
	T_TUPLE_STREAM_CTL    control;
	T_TUPLE_FRAME_SIZE    framesize;
	T_TUPLE_PLOAD_INFO    pload[0];
	unsigned char         pdr[0];
} T_MSGIF_110_REQ_UNIT;

typedef T_MSGIF_HDR          T_MSGIF_110_REQ;
typedef T_MSGIF_SETCMD_RESP  T_MSGIF_110_RESP;

typedef struct {
	EnumProtocol          protocolid;
	unsigned int          length;
	unsigned int          pinfo[0];
} T_PDR;

//111. Direct Register I/O
typedef struct {
	unsigned int          cardid;
	EnumCommandReg        command;
	unsigned int          addr;
	unsigned long long    value;
} T_MSGIF_111_REQ_UNIT;

typedef T_MSGIF_HDR         T_MSGIF_111_REQ;
typedef T_MSGIF_HDR         T_MSGIF_111_RESP;
typedef struct {
	unsigned int          result;
	unsigned long long    value;
} T_MSGIF_111_RESP_UNIT;

//112. Execute shell script
typedef struct {
	unsigned int          length;
	unsigned int          strshell[0];
} T_MSGIF_112_REQ_UNIT;

typedef T_MSGIF_HDR         T_MSGIF_112_REQ;
typedef T_MSGIF_HDR         T_MSGIF_112_RESP;
typedef struct {
	unsigned int          result;
} T_MSGIF_112_RESP_UNIT;


#pragma pack()

#endif //BSL_MSGIF_H
