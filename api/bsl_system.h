/********************************************************************
 *  FILE   : bsl_system.h
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
#ifndef BSL_SYSTEM_H
#define BSL_SYSTEM_H

#include "common.h"
#include "bsl_def.h"
#include "bsl_type.h"

typedef struct {
	unsigned int          payloadOffset;
	unsigned int          validSize;
	unsigned char         payload[SIZE_MAX_PAYLOAD];
} T_FrameDataPattern;

typedef struct {
	unsigned int          sizeOrStep;     
	unsigned int          sizeMin;  //has meaning only for non fixed mode
	unsigned int          sizeMax;  //has meaning only for non fixed mode
} T_FrameDataSize;

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
} T_StreamControl;

typedef struct {
	EnumSpecFrameSize     fsizeSpec;
	unsigned int          sizeOrStep;
	unsigned int          fsizeMin;
	unsigned int          fsizeMax;
	unsigned int          fsizeValueRand[32];
	unsigned int          fsizeValueRandDiff[32];
} T_FrameSize; 

typedef struct {
	unsigned int          portid;
	unsigned int          streamid;
	unsigned int          groupid;						// 2017.7.15 by dgjung
	char                  name[SIZE_STRING_STREAM_NAME];
	int                   enable;
	T_StreamControl       control;
} T_Stream;

typedef struct {
	T_FrameSize           framesize;
	EnumFrameCrcType      crc;
	EnumFrameDataType     payloadType;
	T_FrameDataPattern    pattern;
} T_Frame;

typedef struct {
	EnumLinkOperState     operState;
	EnumLinkAdminState    adminState;
	EnumLinkSpeed         operSpeed;
	EnumLinkSpeed         adminSpeed;
	EnumLinkDuplex        duplex;
	EnumLinkMedia         media;
} T_Link;

typedef struct {
	int                   portid;
	time_t                timestamp;
	unsigned int          framesSentRate;
	unsigned int          validFramesRxRate;
	unsigned int          bytesSentRate;
	unsigned int          bytesRxRate;
	unsigned int          crcErrorFrameRxRate;
	unsigned int          fragmentErrorFrameRxRate;
	unsigned int          crcErrorByteRxRate;
	unsigned int          fragmentErrorByteRxRate;
	unsigned long long    framesSent;
	unsigned long long    byteSent;
	unsigned long long    validFrameReceived;
	unsigned long long    validByteReceived;
	unsigned long long    fragments;
	unsigned long long    fragmentsByteReceived;
	unsigned long long    crcErrors;
	unsigned long long    vlanTaggedFrames;
	unsigned long long    crcErrorByteReceived;
	unsigned long long    vlanByteReceived;
	unsigned int		  latencyMaximum;
	unsigned int		  latencyMinimum;
	unsigned int     	  AverageLatIntPart;
	unsigned int     	  AverageLatFracPart;
	unsigned long long	  seqErrorPacketCount;
	unsigned long long	  undersizeFrameCount;
	unsigned long long	  oversizeFrameCount;
	unsigned long long	  signatureFrameCount;								// 2016.12.31 by dgjung
} T_LinkStats;

typedef struct {
	unsigned int          countStream;
	T_Stream              stream[SIZE_MAX_STREAM];
	T_Link                link;
	EnumPortStatus        status;
	EnumPortOpMode        opmode;
	EnumLinkType          linkType;
} T_Port;

typedef struct {
	T_Port                port[SIZE_MAX_PORT];
	EnumCardStatus        status;
} T_Card;

typedef struct {   
	T_Card                card[SIZE_MAX_CARD];
	EnumChassisStatus     status;
} T_Chassis;

typedef struct {
	char                  board[SIZE_MAX_VERSION_STRING];
	char                  fpga[SIZE_MAX_VERSION_STRING];
	char                  driver[SIZE_MAX_VERSION_STRING];
	char                  api[SIZE_MAX_VERSION_STRING];
	char                  gui[SIZE_MAX_VERSION_STRING];
} T_SystemVersion;

#endif //BSL_SYSTEM_H
