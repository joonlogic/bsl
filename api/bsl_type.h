/********************************************************************
 *  FILE   : bsl_type.h
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
#ifndef BSL_TYPE_H
#define BSL_TYPE_H

typedef enum {
	ChassisStatusPowereDown,
	ChassisStatusInstalled,
	ChassisStatusActive,
	ChassisStatusOutOfOrder,
	ChassisStatusUnknown
} EnumChassisStatus;

typedef enum {
	CardStatusOpen,
	CardStatusInstalled,
	CardStatusActive,
	CardStatusOutOfOrder,
	CardStatusUnknown
} EnumCardStatus;

typedef enum {
	PortStatusActive,
	PortStatusNotInstall,
	PortStatusOutOfOrder,
	PoetStatusUnknown
} EnumPortStatus;

typedef enum {
	LinkType1GE,
	LinkType10GE,
	LinkType100GE
} EnumLinkType;

typedef enum {
	OpModeNormal,
	OpModeInterleave,
	OpModeUnknown
} EnumPortOpMode;

typedef enum {
	LinkSpeed10Mbps,
	LinkSpeed100Mbps,
	LinkSpeed1Gbps,
	LinkSpeed10Gbps,
	LinkSpeed40Gbps,
	LinkSpeed100Gbps,
	LinkSpeedUnknown
} EnumLinkSpeed;

typedef enum {
	AdminUp,
	AdminDown,
	AdminUnknown
} EnumLinkAdminState;

typedef enum {
	LinkDuplexFull,
	LinkDuplexHalf,
	LinkDuplexUnknown
} EnumLinkDuplex;

typedef enum {
	LinkMediaOptic,
	LinkMediaCopper,
	LinkMediaUnknown
} EnumLinkMedia;

typedef enum {
	PortActiveDisable,
	PortActiveEnable,
	PortActiveUnknown
} EnumPortActive;

typedef enum {
	LinkUp,
	LinkDown,
	LinkUnknown
} EnumLinkOperState;

typedef enum {
	FrameSizeFixed,
	FrameSizeRandom,
	FrameSizeIncrement,
	FrameSizeAuto,
	FrameSizeUnknown
} EnumSpecFrameSize;

typedef enum {
	FrameCrcNoError,
	FrameCrcBadCrc,
	FrameCrcNoCrc,
	FrameCrcUnknown
} EnumFrameCrcType;


typedef enum {
	FrameDataTypeIncByte,
	FrameDataTypeIncWord,
	FrameDataTypeDecByte,
	FrameDataTypeDecWord,
	FrameDataTypeRandom, //Not Implemented						-> 2017.3.13 Implement by dgjung
	FrameDataTypeRepeating,
	FrameDataTypeFixed,
	FrameDataTypeNoPayload,
	FrameDataTypeDownload,
	FrameDataTypeUnknown
} EnumFrameDataType;

typedef enum {
	RateControlPercent,
	RateControlPacketPerSec,
	RateControlBitPerSec,
	RateControlInterPacketGap,
	RateControlUnknown
} EnumRateControl;

typedef enum {
	StreamContinuousPacket,
	StreamContinuousBurst,
	StreamStopAfterThisStream,
	StreamAdvanceToNextStream,
	StreamReturnToId,
	StreamReturnToIdForCount,
	StreamControlUnknown
} EnumStreamControl;

#define SIZE_MAX_PROTOCOL_ID    60 //Used for msgif
typedef enum {
	ProtocolUnused = 0,
	ProtocolNull = 1,				// 2016.12.20 by dgjung
	ProtocolEthernet = 21,
	ProtocolVLAN = 22,
	ProtocolISL = 23,
	ProtocolMPLS = 24,
	ProtocolIP4 = 31,
	ProtocolIP6 = 32,
	ProtocolIP4OverIP6 = 33,
	ProtocolIP6OverIP4 = 34,
	ProtocolARP = 35,
	ProtocolTCP = 41,
	ProtocolUDP = 42,
	ProtocolICMP = 43,
	ProtocolIGMPv2 = 44,
	ProtocolUDF = 45, //User Defined Format
	ProtocolUnknown
} EnumProtocol;

typedef enum {
	ResultGeneralError = -1,
	ResultSuccess,
	ResultAlreadyExist,
	ResultOpenDeviceError,
	ResultNoSuchInstance,
	ResultOutOfRange,
	ResultNullArg,
	ResultSocketError,
	ResultBindError,
	ResultIoctlFailure,
	ResultCommandNotCleared,
	ResultPthreadFailure,
	ResultMmapFailure,
	ResultMunmapFailure,
	ResultMsgIfDelimError,
	ResultMsgIfMsgidError,
	ResultMsgIfSendError,
	ResultMsgIfMsgHandleNYI, //Not Yet Implemented
	ResultMsgIfMsgHandleError, 
	ResultMallocError, 
	ResultBadValue, 
	ResultDmaError, 
	ResultUnknown
} EnumResultCode;

typedef enum {
	EtherAddrModeFixed,
	EtherAddrModeIncrement,
	EtherAddrModeDecrement,
	EtherAddrModeContIncrement,
	EtherAddrModeContDecrement,
	EtherAddrModeRandom,
	EtherAddrModeUnknown
} EnumEtherAddrMode;

typedef enum {
	CustomIntegerModeFixed,
	CustomIntegerModeIncrement,
	CustomIntegerModeDecrement,
	CustomIntegerModeContIncrement,
	CustomIntegerModeContDecrement,
	CustomIntegerModeRandom,
	CustomIntegerModeUnknown
} EnumCustomIntegerMode;

typedef enum {
	VlanIdCountModeFixed,
	VlanIdCountModeIncrement,
	VlanIdCountModeDecrement,
	VlanIdCountModeContIncrement,
	VlanIdCountModeContDecrement,
	VlanIdCountModeRandom,
	VlanIdCountModeUnknown
} EnumVlanIdCountMode;

typedef enum {
	IpAddrModeFixed,
	IpAddrModeIncrementHost,
	IpAddrModeDecrementHost,
	IpAddrModeContIncrementHost,
	IpAddrModeContDecrementHost,
	IpAddrModeIncrementNetwork,
	IpAddrModeDecrementNetwork,
	IpAddrModeContIncrementNetwork,
	IpAddrModeContDecrementNetwork,
	IpAddrModeRandom,
	IpAddrModeUnknown
} EnumIpAddrMode;

typedef enum {
	ChecksumValid,
	ChecksumInvalid,
	ChecksumOverride
} EnumChecksum;

typedef enum {
	CommandStart,
	CommandStop,
	CommandPause,
	CommandSingleStep,
	CommandResetStats,
	CommandDeleteStream,
	CommandNetworkMode,
	CommandUnknown
} EnumCommand;

typedef enum {
	CommandRegRead,
	CommandRegWrite,
	CommandRegUnknown
} EnumCommandReg;

typedef enum {
	ExecShutdown,
	ExecWskDisplay,
	ExecInitial,
	ExecNetworkChg, 
	ExecUnknown
} EnumExecType;

typedef enum {
	UDFCounter_8,
	UDFCounter_16,
	UDFCounter_8by8,
	UDFCounter_24,
	UDFCounter_16by8,
	UDFCounter_8by16,
	UDFCounter_8by8by8,
	UDFCounter_32,
	UDFCounter_24by8,
	UDFCounter_16by16,
	UDFCounter_16by8by8,
	UDFCounter_8by24,
	UDFCounter_8by16by8,
	UDFCounter_8by8by16,
	UDFCounter_8by8by8by8,
	UDFCounterUnknown
} EnumUDFCounterType;

typedef enum {
	UDFCountUp,
	UDFCountDown,
	UDFCountUnknown
} EnumUDFCountingMode;

typedef enum {
	UDFSetFromInitVal,
	UDFContinueFromLastVal,
	UDFCascadeContinueFromLastVal
} EnumUDFInitValueType;

typedef enum {
	CaptureModeAll,
	CaptureModeNormal,
	CaptureModeCrcError,
	CaptureModeFragmentError=4,
} EnumCaptureMode;

typedef enum {
	MplsTypeUnicast,
	MplsTypeMulticast
} EnumMplsType;

typedef enum {
	VlanModeSingle,
	VlanModeStack
} EnumVlanMode;

#endif //BSL_TYPE_H
