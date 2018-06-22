#include <stdio.h>
#include <Python.h>
#include "bsl_system.h"
#include "bsl_dbg.h"
#include "bsl_api.h"
#include "bsl_register.h"

/**********************************************
 * data type indication of tuples -------------
	(B) : unsigned char
	(H) : unsigned short
	(i) : int
	(I) : unsigned int
	(K) : unsigned long long
	(f) : float
	(s) : string
	(O) : Embedded Object like tuple, list, etc.
	-------------------------------------------
***********************************************/

#ifdef _IMPLEMENT_STREAM_
typedef void (*convert_T_PDR_f_ptr_type) ( PyTupleObject* pdr, T_Protocol* proto );
static convert_T_PDR_f_ptr_type
    convert_T_PDR_f_ptr[SIZE_MAX_PROTOCOL_ID] =
{
    [0] = NULL,
    [1] = (convert_T_PDR_f_ptr_type)&convertPdrNull,
    [21] = (convert_T_PDR_f_ptr_type)&convertPdrEthernet,
    [22] = (convert_T_PDR_f_ptr_type)&convertPdrVLAN,
    [23] = (convert_T_PDR_f_ptr_type)&convertPdrISL,
    [24] = (convert_T_PDR_f_ptr_type)&convertPdrMPLS,
    [31] = (convert_T_PDR_f_ptr_type)&convertPdrIp4,
    [32] = (convert_T_PDR_f_ptr_type)&convertPdrIp6,
    [33] = (convert_T_PDR_f_ptr_type)&convertPdrIp4OverIp6,
    [34] = (convert_T_PDR_f_ptr_type)&convertPdrIp6OverIp4,
    [35] = (convert_T_PDR_f_ptr_type)&convertPdrArp,
    [41] = (convert_T_PDR_f_ptr_type)&convertPdrTcp,
    [42] = (convert_T_PDR_f_ptr_type)&convertPdrUdp,
    [43] = (convert_T_PDR_f_ptr_type)&convertPdrIcmp,
    [44] = (convert_T_PDR_f_ptr_type)&convertPdrIgmpv2,
//  [51] = (convert_T_PDR_f_ptr_type)&convertPdrUDF,
};

static void convertPdrNull( PyTupleObject* pdr, T_Protocol* proto )
{
	BSL_CHECK_NULL( pdr, );
	proto->l2.protocol = ProtocolNull;
	proto->l2.length = 0;
	proto->l2.pdr = NULL;
}

static void convertPdrEthernet( PyTupleObject* pdr, T_Protocol* proto )
{
	int ret = 0;
	PyTupleObject* tuple_dmac = 0; 
	PyTupleObject* tuple_smac = 0; 

	T_PDR_Ethernet* pdr = (T_PDR_Ethernet*)proto->l2.pdr;
	BSL_CHECK_NULL( pdr, );

	ret = PyArg_ParseTuple(pdr, "IIOOI", 
			&proto->l2.protocol,
			&proto->l2.length,
			&tuple_dmac,
			&tuple_smac,
			&pdr->type);
	BSL_CHECK_TRUE(ret, -1);

	ret = PyArg_ParseTuple(tuple_dmac, "I"...........




		unsigned int* randp = 
			(unsigned int*)proto->fi.framesize.fsizeValueRand;
		int randpsize = PyTuple_Size( fsizeRandom );
		BSL_CHECK_TRUE( randpsize == NUM_FRAMESIZE_RANDOM, -1 );
		for( int i=0; i<randpsize; i++ ) 
			randp[i] = PyLong_AsLong( PyTuple_GetItem( fsizeRandom, i ) );


	proto->l2.protocol = ProtocolEthernet;
	proto->l2.length = ;
	proto->l2.pdr = NULL;
}

#endif

/* ---------------------------------------
 * name : getNumberOfCards()
 *
 * input argument tuple ------------------
	  None

 * return tuple  --------------------------
  (I) number of installed cards
 -----------------------------------------*/

static PyObject* getNumberOfCards(PyObject *self, PyObject *args) 
{
    int nCards;
	int ret = 0;

	printf("%s: %d - \n", __func__, __LINE__ ); 

    ret = bsl_getNumberOfCards( &nCards );
	BSL_CHECK_RESULT( ret, NULL );

    printf("Installed Cards : %d!\n", nCards);
	return Py_BuildValue("i", nCards);
}

/* ---------------------------------------
 * name : getVersion()
 *
 * input argument tuple ------------------
  (i) card id

 * return tuple  --------------------------
  (I) //TODO: later
 -----------------------------------------*/

static PyObject* getVersion(PyObject *self, PyObject *args) 
{
	int ret = 0;
	int cardid = 0;
	T_SystemVersion ver = {0,};

	ret = PyArg_ParseTuple(args, "i", &cardid);
	BSL_CHECK_TRUE(ret, (Py_BuildValue("(i)", ret)));

	printf("%s: %d - \n", __func__, __LINE__ ); 
	printf("Card : %d\n", cardid ); 

    ret = bsl_getVersionInfo( cardid, &ver );
	BSL_CHECK_RESULT( ret, NULL );

    printf("FPGA  : %s\n", ver.fpga);
    printf("API   : %s\n", ver.api);

	//deliver to Python
	return Py_BuildValue("ss", ver.fpga, ver.api);
}

/* ---------------------------------------
 * name : getLinkStatus()
 *
 * input argument tuple ------------------
  (i) card id

 * return tuple  --------------------------
  (i) link status of port 0
  (i) link status of port 1
 -----------------------------------------*/
static PyObject* getLinkStatus(PyObject *self, PyObject *args) 
{
    T_Card card={0,};
	int ret = 0;
	int cardid = 0;
	int nport = 0;

	ret = PyArg_ParseTuple(args, "i", &cardid);
	BSL_CHECK_TRUE(ret, (Py_BuildValue("(i)", ret)));

	printf("%s: %d - \n", __func__, __LINE__ ); 
	printf("Card : %d\n", cardid ); 

    ret = bsl_getLinkStatus( cardid, &card );
	BSL_CHECK_RESULT( ret, NULL );

	//deliver to Python
	//TODO: nport
	nport = 2; //2 for 10G, 1 for 40G

	if( nport == 2 ) {
		return Py_BuildValue("(ii)", card.port[0].link.operState, card.port[1].link.operState);
	}
	else {
		return Py_BuildValue("(i)", card.port[0].link.operState);
	}
	return NULL;
}

/* ---------------------------------------
 * name : setPortMode()
 *
 * input argument tuple ------------------
  (i) card id
  (i) port id
  (i) port mode

 * return tuple  --------------------------
  (I) error code. If success return 0 
 -----------------------------------------*/
static PyObject* setPortMode(PyObject *self, PyObject *args) 
{
    EnumPortOpMode mode;
	int ret = 0;
	int cardid = 0;
	int portid = 0;

	ret = PyArg_ParseTuple(args, "iii", &cardid, &portid, &mode);
	BSL_CHECK_TRUE(ret, (Py_BuildValue("(i)", ret)));

	printf("%s: %d - \n", __func__, __LINE__ ); 
	printf("Card : %d\n", cardid ); 
	printf("Port : %d\n", portid ); 
	printf("Mode : %d\n", mode ); 

    ret = bsl_setPortMode( cardid, portid, mode );
	BSL_CHECK_RESULT( ret, NULL );

	//deliver to Python
	return Py_BuildValue("(i)", ret);
}

/* ---------------------------------------
 * name : getLinkStats()
 *
 * input argument tuple ------------------
  (i) card id
  (i) port id

 * return tuple  --------------------------
  (I) timestamp,
  (I) framesSentRate,
  (I) validFramesRxRate,
  (I) bytesSentRate,
  (I) bytesRxRate,
  (I) crcErrorFrameRxRate,
  (I) fragmentErrorFrameRxRate,
  (I) crcErrorByteRxRate,
  (I) fragmentErrorByteRxRate,
  (K) framesSent,
  (K) byteSent,
  (K) validFrameReceived,
  (K) validByteReceived,
  (K) fragments,
  (K) fragmentsByteReceived,
  (K) crcErrors,
  (K) vlanTaggedFrames,
  (K) crcErrorByteReceived,
  (K) vlanByteReceived,
  (I) latencyMaximum,
  (I) latencyMinimum,
  (I) AverageLatIntPart,  //TODO: to float
  (I) AverageLatFracPart, //TODO: to float
  (K) seqErrorPacketCount,
  (K) undersizeFrameCount,
  (K) oversizeFrameCount,
  (K) signatureFrameCount );
 -----------------------------------------*/
static PyObject* getLinkStats(PyObject *self, PyObject *args) 
{
    T_LinkStats stat;
	int ret = 0;
	int cardid = 0;
	int portid = 0;

	ret = PyArg_ParseTuple(args, "ii", &cardid, &portid);
	BSL_CHECK_TRUE(ret, (Py_BuildValue("(i)", ret)));

	printf("%s: %d - \n", __func__, __LINE__ ); 
	printf("Card : %d\n", cardid ); 
	printf("Port : %d\n", portid ); 

    ret = bsl_getLinkStats( cardid, portid, &stat );
	BSL_CHECK_RESULT( ret, NULL );

	//deliver to Python
	PyObject* tuple = 
		Py_BuildValue("(iiIIIIIIIIIKKKKKKKKKKIIIIKKKK)",
			cardid,
			portid,
			stat.timestamp,
			stat.framesSentRate,
			stat.validFramesRxRate,
			stat.bytesSentRate,
			stat.bytesRxRate,
			stat.crcErrorFrameRxRate,
			stat.fragmentErrorFrameRxRate,
			stat.crcErrorByteRxRate,
			stat.fragmentErrorByteRxRate,
			stat.framesSent,
			stat.byteSent,
			stat.validFrameReceived,
			stat.validByteReceived,
			stat.fragments,
			stat.fragmentsByteReceived,
			stat.crcErrors,
			stat.vlanTaggedFrames,
			stat.crcErrorByteReceived,
			stat.vlanByteReceived,
			stat.latencyMaximum,
			stat.latencyMinimum,
			stat.AverageLatIntPart,  //TODO: to float
			stat.AverageLatFracPart, //TODO: to float
			stat.seqErrorPacketCount,
			stat.undersizeFrameCount,
			stat.oversizeFrameCount,
			stat.signatureFrameCount );

	printf("%s: returned tuple size %ld\n", __func__, PyTuple_Size(tuple));
	return tuple; 
}

/* ---------------------------------------
 * name : setPortActive()
 *
 * input argument tuple ------------------
  (i) card id
  (i) port id
  (i) enable

 * return tuple  --------------------------
  (I) error code. If success return 0 
 -----------------------------------------*/

static PyObject* setPortActive(PyObject *self, PyObject *args) 
{
    EnumPortActive enable;
	int ret = 0;
	int cardid = 0;
	int portid = 0;

	ret = PyArg_ParseTuple(args, "iii", &cardid, &portid, &enable);
	BSL_CHECK_TRUE(ret, (Py_BuildValue("(i)", ret)));

	printf("%s: %d - \n", __func__, __LINE__ ); 
	printf("Card : %d\n", cardid ); 
	printf("Port : %d\n", portid ); 
	printf("Enable : %d\n", enable ); 

    ret = bsl_setPortActive( cardid, portid, enable );
	BSL_CHECK_RESULT( ret, NULL );

	//deliver to Python
	PyObject* tuple = Py_BuildValue("(i)", ret);
	printf("%s: PyTuple_Check %d\n", __func__, PyTuple_Check(tuple));

	return tuple;
}

/* ---------------------------------------
 * name : setCaptureStartStop()
 *
 * input argument tuple ------------------
  (i) card id
  (i) port id
  (i) mode
  (I) size
  (i) start

 * return tuple  --------------------------
  (I) error code. If success return 0 
 -----------------------------------------*/

static PyObject* setCaptureStartStop(PyObject *self, PyObject *args) 
{
	int ret = 0;
	int cardid = 0;
	int portid = 0;
	EnumCaptureMode mode;
	unsigned int size = 0;
	int start = 0;

	ret = PyArg_ParseTuple(args, "iiiIi", &cardid, &portid, &mode, &size, &start);
	BSL_CHECK_TRUE(ret, (Py_BuildValue("(i)", ret)));

	printf("%s: %d - \n", __func__, __LINE__ ); 
	printf("Card : %d\n", cardid ); 
	printf("Port : %d\n", portid ); 
	printf("Mode : %d\n", mode ); 
	printf("Size : %u\n", size ); 
	printf("start : %d\n", start ); 

    ret = bsl_setCaptureStartStop( cardid, portid, mode, size, start );
	BSL_CHECK_RESULT( ret, NULL );

	//deliver to Python
	return Py_BuildValue("(i)", ret);
}

/* ---------------------------------------
 * name : setLatency()
 *
 * input argument tuple ------------------
  (i) card id
  (i) port id
  (i) latency_enable
  (i) sequence_enable
  (i) signature_enable

 * return tuple  --------------------------
  (I) error code. If success return 0 
 -----------------------------------------*/

static PyObject* setLatency(PyObject *self, PyObject *args) 
{
	int ret = 0;
	int cardid = 0;
	int portid = 0;
	int latency_enable;
	int sequence_enable;
	int signature_enable;

	ret = PyArg_ParseTuple(args, "iiiii", &cardid, &portid, &latency_enable, &sequence_enable, &signature_enable);
	BSL_CHECK_TRUE(ret, (Py_BuildValue("(i)", ret)));

	printf("%s: %d - \n", __func__, __LINE__ ); 
	printf("Card : %d\n", cardid ); 
	printf("Port : %d\n", portid ); 
	printf("Latency_enable : %d\n", latency_enable ); 
	printf("Sequence_enable : %d\n", sequence_enable ); 
	printf("Signature_enable : %d\n", signature_enable ); 

    ret = bsl_setLatency( cardid, portid, latency_enable, sequence_enable, latency_enable );
	BSL_CHECK_RESULT( ret, NULL );

	//deliver to Python
	return Py_BuildValue("(i)", ret);
}

/* ---------------------------------------
 * name : setControlCommand()
 *
 * input argument tuple ------------------
  (i) portsel
  (i) card id
  (i) port id
  (i) stream id
  (i) command
  (K) clocks
  (I) netmode
  (K) mac
  (I) ip

 * return tuple  --------------------------
  (I) error code. If success return 0 
 -----------------------------------------*/

static PyObject* setControlCommand(PyObject *self, PyObject *args) 
{
	int ret = 0;
	int portsel = 0;
	int cardid = 0;
	int portid = 0;
	int streamid = 0;
	EnumCommand command;
	unsigned long long clocks;
	unsigned int netmode;
	unsigned long long mac;
	unsigned int ip;

	ret = PyArg_ParseTuple(args, "iiiiiKIKI", &portsel, &cardid, &portid, &streamid, &command, &clocks, &netmode, &mac, &ip );
	BSL_CHECK_TRUE(ret, (Py_BuildValue("(i)", ret)));

	printf("%s: %d - \n", __func__, __LINE__ ); 
	printf("Portsel : %d\n", portsel ); 
	printf("Card : %d\n", cardid ); 
	printf("Port : %d\n", portid ); 
	printf("Stream : %d\n", streamid ); 
	printf("command : %d\n", command ); 
	printf("clocks : %llu\n", clocks ); 
	printf("netmode : %u\n", netmode ); 
	printf("mac : %016llX\n", mac ); 
	printf("ip : %08X\n", ip ); 

    ret = bsl_setControlCommand( portsel, cardid, portid, streamid, command, clocks, netmode, mac, ip );
	BSL_CHECK_RESULT( ret, NULL );

	//deliver to Python
	return Py_BuildValue("(i)", ret);
}

/* ---------------------------------------
 * name : enableStream()
 *
 * input argument tuple ------------------
  (i) card id
  (i) port id
  (i) stream id
  (i) enable

 * return tuple  --------------------------
  (I) error code. If success return 0 
 -----------------------------------------*/

static PyObject* enableStream(PyObject *self, PyObject *args) 
{
	int ret = 0;
	int cardid = 0;
	int portid = 0;
	int streamid = 0;
	int enable;

	ret = PyArg_ParseTuple(args, "iiii", &cardid, &portid, &streamid, &enable );
	BSL_CHECK_TRUE(ret, (Py_BuildValue("(i)", ret)));

	printf("%s: %d - \n", __func__, __LINE__ ); 
	printf("Card : %d\n", cardid ); 
	printf("Port : %d\n", portid ); 
	printf("Stream : %d\n", streamid ); 
	printf("Enable : %d\n", enable ); 

    ret = bsl_enableStream( cardid, portid, streamid, enable );
	BSL_CHECK_RESULT( ret, NULL );

	//deliver to Python
	return Py_BuildValue("(i)", ret);
}

/* ---------------------------------------
 * name : setStreamDetail()
 *
 * input argument tuple *********************
  +------------------------------------------+
  | card | port | stream | group | SIT | PIT |
  +------------------------------------------+
  (i) card id
  (i) port id
  (i) stream id
  (i) group id
  (O) Stream Information Tuple (SIT)
  (O) PDR Information Tuple (PIT)

 * detail description ***********************
 * 
  0. Total Tuple ****************************
	+--------------------------------+
	| Stream Info (T) | PDR Info (T) |
	+--------------------------------+

  1. Stream Information Tuple ***************
	+-------------------------------------+
	| Stream Control (T) | Frame Size (T) |
	+-------------------------------------+
  1.1 Stream Control Tuple
	(I) stream control specification
	(I) return to Id
	(I) loop count
	(K) packets per burst
	(I) bursts per stream
	(I) start tx delay
	(I) inter burst gap integer part
	(I) inter burst gap fractional part
	(I) inter stream gap integer part
	(I) inter stream gap fractional part
	(I) inter frame gap

  1.2 Frame Size Tuple
	(I) frame size spec
	(I) frame size (when spec is 'fixed') or step (when 'increment')
	(I) frame size min
	(I) frame size max
	(O) frame size random : tuple of 32 (unsigned int) fixed
	(O) frame size diff   : tuple of 32 (unsigned int) fixed
	(I) CRC
	(I) data pattern type
	(I) payload offset
	(s) payload pattern 

  2. PDR Information Tuples ( Protocol Description Records )
    * for example,
	+-------------------------------------------+
	| Ethernet(T) |  MPLS(T) |  IP4(T) | TCP(T) |
	+-------------------------------------------+
  2.1 PDR Tuples -------------------------
	2.1.1 Ethernet 
	  (I) ID : 21
	  (O) destination mac - Ethernet Addr Tuple
	  (O) source mac - Ethernet Addr Tuple
	  (I) ether type

	2.1.2 802.1Q VLAN
	  (I) ID : 22
	  (I) Mode
	  (O) vlan data : Vlan Tuple

	2.1.3. ISL
	  (I) ID : 23
	  (s) destination ISL addr : string length 10 fixed
	  (I) type
	  (I) user
	  (s) source ethernet addr : string length 12 fixed
	  (I) length
	  (I) vlanid
	  (I) bpduInd
	  (I) index
	  (I) reserved
	  (s) destination mac : string length 12 fixed
	  (s) source mac : string length 12 fixed

	2.1.4 IP4
	  (I) ID : 31
	  (I) version
	  (I) header length
	  (I) tos
	  (I) total length
	  (O) id - Custom Interger Tuple
	  (I) flags
	  (I) flagoffset
	  (I) ttl
	  (I) protocol
	  (O) checksum - Checksum Tuple
	  (O) source ip4 - IP4 Addr Tuple
	  (O) destination ip4 - IP4 Addr Tuple
	  (I) length of option field : optional
	  (s) value of option field : optional

	2.1.5 IP6
	  (I) ID : 32
	  (I) version
	  (I) traffic class
	  (I) flow label
	  (I) payload length
	  (I) next header
	  (I) hop limit
	  (O) source ip6 - IP6 Address Tuple
	  (O) destinaion ip6 - IP6 Address Tuple
	  (I) next header length 1 : optional
	  (s) next header value 1 : optional
	  (I) next header length 2 : optional
	  (s) next header value 2 : optional
		---- repeated ----
	2.1.6 ARP
	  (I) ID : 35
	  (I) type
	  (I) hardware type
	  (I) protocol type
	  (I) hardware address length
	  (I) physical address length
	  (I) operation
	  (O) sender mac - Ethernet Addr Tuple
	  (O) sender ip4 - IP4 Addr Tuple
	  (O) target mac - Ethernet Addr Tuple
	  (O) target ip4 - IP4 Addr Tuple

	2.1.7 TCP
	  (I) ID : 41
	  (O) source port - Custom Integer Tuple
	  (O) destination port - Custom Integer Tuple
	  (I) sequence number
	  (I) ack number
	  (I) offset
	  (I) flag reserved
	  (I) flag urgent
	  (I) flag ack
	  (I) flag push
	  (I) flag reset
	  (I) flag syn
	  (I) flag fin
	  (I) windows
	  (I) checksum - Checksum Tuple
	  (I) urgent
	  (I) option length : optional
	  (s) option value : optional

	2.1.8 UDF ( User Defined Format )
	  (I) ID : 51
	  (I) udf id 
	  (I) enable
	  (I) counter type
	  (I) random
	  (I) offset
	  (I) repeat count
	  (I) counting mode
	  (I) step
	  (I) initial value
	  (I) initial value type
	  (I) bitmask

	2.1.9 MPLS 
	  (I) ID : 24
	  (I) type
	  (O) MPLS data - SubMPLS Tuple

	2.1.10 IP4/IP6 
	  (I) ID : 33
	  (I) length
	  (O) IP6
	  (O) IP4

	2.1.11 IP6/IP4 
	  (I) ID : 34
	  (I) length
	  (O) IP4
	  (O) IP6

	2.1.12 UDP
	  (I) ID : 42
	  (O) source port - Custom Integer Tuple
	  (O) dest port - Custom Integer Tuple
	  (I) length override
	  (I) length value
	  (O) checksum - Checksum Tuple

	2.1.13 ICMP
	  (I) ID : 43
	  (I) type
	  (I) code
	  (O) checksum - Checksum Tuple
	  (I) data1
	  (I) data2

	2.1.14 IGMPv2
	  (I) ID : 44
	  (I) version
	  (I) type
	  (I) max resp time
	  (O) checksum - Checksum Tuple
	  (O) group - IP4 Address Tuple

  2.2 Sub-PDR Tuples -----------------------
	2.2.1 Ethernet Addr Tuple
	  (I) mode
	  (s) mac address : string length 12 fixed
	  (I) repeat count
	  (I) step

	2.2.2 Vlan Tuple
	  (I) priority
	  (I) cfi
	  (I) vlanid
	  (I) tag control info
	  (I) tag protocol id
	  (I) vlan id count mode
	  (I) count
	  (I) bit mask

	2.2.3 Checksum Tuple
	  (I) checksum type
	  (I) checksum value

	2.2.4 IP4 Address Tuple
	  (I) address
	  (I) mask
	  (I) repeat
	  (I) address mode

	2.2.5 IP6 Address Tuple
	  (s) address : string length 32 fixed
	  (I) repeat
	  (I) step
	  (I) mask
	  (I) address mode

	2.2.6 Custom Integer Tuple
	  (I) mode
	  (I) value
	  (I) step
	  (I) repeat

	2.2.7 SubMPLS Tuple
	  (I) label
	  (I) experimental use
	  (I) bottom of stack
	  (I) time to live

 * return tuple  --------------------------
  (I) error code. If success return 0 
 -----------------------------------------*/

#ifdef _IMPLEMENT_STREAM_
#define SIZE_MAX_PDR_LENGTH   256
static PyObject* setStreamDetail(PyObject *self, PyObject *args) 
{
	int ret = 0;
	int cardid = 0;
	int portid = 0;
	int streamid = 0;
	int groupid = 0;
	int enable = 0;
	T_Protocol proto = {0,};
	T_Stream stream = {0,};
	PyTupleObject* tuples = 0; 
	PyTupleObject* tuplep = 0; 

	//reserve memory as a local variable
	unsigned char pdrinfo[4][SIZE_MAX_PDR_LENGTH] = {{0,},};
	proto.l2.pdr = (void*)pdrinfo[0];
	proto.l2tag.pdr = (void*)pdrinfo[1];
	proto.l3.pdr = (void*)pdrinfo[2];
	proto.l4.pdr = (void*)pdrinfo[3];

	ret = PyArg_ParseTuple(args, "iiiiiOO", &cardid, &portid, &streamid, &groupid, &enable, &tuples, &tuplep );
	BSL_CHECK_TRUE(ret, (Py_BuildValue("(i)", ret)));

	printf("%s: %d - \n", __func__, __LINE__ ); 
	printf("Card : %d\n", cardid ); 
	printf("Port : %d\n", portid ); 
	printf("Stream : %d\n", streamid ); 
	printf("Groupid : %d\n", groupid ); 
	printf("Enable : %d\n", enable ); 

	printf("PyTuple_Check(tuples) = %d\n", PyTuple_Check(tuples));
	printf("PyTuple_Check(tuplep) = %d\n", PyTuple_Check(tuplep));

	stream.portid = portid;
	stream.streamid = streamid;
	stream.groupid = groupid;
	stream.enable = enable;
	proto.streamid = streamid;

	ret = copyStreamDetailInfo( tuples, tuplep, &stream, &proto );
	BSL_CHECK_RESULT(ret, (Py_BuildValue("(i)", ret)));
	
	if( enable != 0 ) {
		ret=bsl_setStreamDetail ( cardid, portid, streamid,
				groupid, &stream, &proto );
	}

	ret = 
		ret == 0 ? bsl_enableStream( cardid, portid, streamid, enable ) :
		ret;


	//deliver to Python
	return Py_BuildValue("(i)", ret);
}

// Copy tuples into stream and proto 
const int SizeOfStreamInfoTuples = 2;
const int SizeOfStreamControlTuples = 11;
const int SizeOfFrameSizeTuples = 10;

static int
copyStreamDetailInfo( PyTupleObject* tuples, PyTupleObject* tuplep, T_Stream* stream, T_Proto* proto ) 
{
	int slen = PyTuple_Size(tuples);
	int plen = PyTuple_Size(tuplep);
	int ret = 0;

	BSL_CHECK_TRUE((slen == SizeOfStreamInfoTuples), -1);

	//1. Stream Control info
	PyTupleObject* iter = PyTuple_GetItem( tuples, 0 );
	BSL_CHECK_NULL(iter, -1);

	ret = copyStreamControl( iter, stream );
	BSL_CHECK_RESULT( ret, ret );

	//2. Frame Size info
	iter = PyTuple_GetItem( tuples, 1 );
	BSL_CHECK_NULL(iter, -1);

	ret = copyFrameSize( iter, proto );
	BSL_CHECK_RESULT( ret, ret );

	//3. Protocol
	ret = copyProtocolInfo( tuplep, proto );
	BSL_CHECK_RESULT( ret, ret );

	return 0;
}

static int
copyStreamControl( PyTupleObject* tuple, T_Stream* stream )
{
	BSL_CHECK_NULL(tuple, -1);
	BSL_CHECK_NULL(steam, -1);

	int ret = 0;
	int slen = PyTuple_Size(tuple);
	BSL_CHECK_TRUE((slen == SizeOfStreamControlTuples), -1);

	ret = PyArg_ParseTuple(tuple, "IIIKIIIIIII", 
			&stream->control.control,
			&stream->control.returnToId,
			&stream->control.loopCount,
			&stream->control.pktsPerBurst,
			&stream->control.burstsPerStream,
			&stream->control.startTxDelay,
			&stream->control.interBurstGapIntPart,
			&stream->control.interBurstGapFracPart,
			&stream->control.interStreamGapIntPart,
			&stream->control.interStreamGapFracPart,
			&stream->control.ifg );
	BSL_CHECK_TRUE(ret, -1);

	return 0; 
}

static int
copyFrameSize( PyTupleObject* tuple, T_Protocol* proto )
{
	BSL_CHECK_NULL(tuple, -1);
	BSL_CHECK_NULL(steam, -1);

	int ret = 0;
	int slen = PyTuple_Size(t_control);
	BSL_CHECK_TRUE((slen == SizeOfFrameSizeTuples), -1);

	PyTupleObject* fsizeRandom;
	PyTupleObject* fsizeRandomDiff;
	char* fsizePattern;

	ret = PyArg_ParseTuple(tuple, "IIIIOOIIIs", 
			&proto->fi.framesize.fsizeSpec,
			&proto->fi.framesize.sizeOrStep,
			&proto->fi.framesize.fsizeMin,
			&proto->fi.framesize.fsizeMax,
			&fsizeRandom,
			&fsizeRandomDiff,
			&proto->fi.payloadType,
			&proto->fi.crc,
			&proto->fi.pattern.payloadOffset,
			&fsizePattern );
	BSL_CHECK_TRUE(ret, -1);

	if( proto->fi.framesize.fsizeSpec == FrameSizeRandom ) {
		unsigned int* randp = 
			(unsigned int*)proto->fi.framesize.fsizeValueRand;
		int randpsize = PyTuple_Size( fsizeRandom );
		BSL_CHECK_TRUE( randpsize == NUM_FRAMESIZE_RANDOM, -1 );
		for( int i=0; i<randpsize; i++ ) 
			randp[i] = PyLong_AsLong( PyTuple_GetItem( fsizeRandom, i ) );

		randp = (unsigned int*)proto->fi.framesize.fsizeValueRandDiff;
		randpsize = PyTuple_Size( fsizeRandomDiff );
		BSL_CHECK_TRUE( randpsize == NUM_FRAMESIZE_RANDOM, -1 );
		for( int i=0; i<randpsize; i++ ) 
			randp[i] = PyLong_AsLong( PyTuple_GetItem( fsizeRandom, i ) );
	}

	int valsize = strlen( fsizePattern )/2;
	proto->fi.pattern.validSize = valsize;
	unsigned char pattern[SIZE_MAX_PAYLOAD] = {0,};
	unsigned char* payload = proto->fi.pattern.payload;

	str2hex( fsizePattern, proto->fi.pattern.validSze, pattern );
	if( proto->fi.payloadType == FrameDataTypeRepeating ) {
		int i,j;
		for( i=0; i<valsize-(valsize%4); i+=4 ) {
			for( j=0; j<4; j++ )
				payload[i+3-j] = pattern[i+j];
		}
		for( i, j=0; j<valsize%4; i++,j++ ) {
			payload[i] = pattern[valsize-1-j];
		}
	}

	return 0; 
}

static int
copyProtocolInfo( PyTupleObject* tuple, T_Protocol* proto )
{
	BSL_CHECK_NULL( tuple, -1 );
	BSL_CHECK_NULL( proto, -1 );

	PyTupleObject* iter;
	EnumProtocolId pid;
	unsigned int plen;
	int tsize = PyTuple_Size( tuple );

	for( int i=0; i<tsize; i++ ) {
		iter = PyTuple_GetItem( tuple, i );
		if( iter == NULL ) break;

		ret = PyArg_ParseTuple(iter, "II", &pid, &plen);
		BSL_CHECK_TRUE(ret, -1);

		printf("%s:[%d] PROTOCOL ID %d. LENGTH %d.\n", 
				__func__, i, pid, plen );

		if( pid == ProtocolNull ) {
			printf("Hello BSL Test\n");
			break;
		}
		else if( pid >= SIZE_MAX_PROTOCOL_ID ) {
			printf("%s: Out Of Range. protocolid %d, pdrlen %d\n",
					__func__, pid, plen );
			break;
		}

		if( convert_T_PDR_f_ptr[pid] ) 
			convert_T_PDR_f_ptr[pid]( iter, proto );
		else {
			printf("%s: No Such Protocol Id %d or NYI, plen %d\n",
					__func__, pid, plen );
			break;
		}
	} 

	return 0;
}

static int 
str2hex( char* str, int len, unsigned char* hex )
{
	BSL_CHECK_NULL( str, -1 );
	BSL_CHECK_NULL( hex, -1 );

	char iter[4] = {0,};
	for( int i=0; i<len; i+=2 ) {
		strncpy( iter, str+i, 2 );
		*hex++ = strtol( iter, NULL, 16 );
	}

	return 0;
}
#endif //_IMPLEMENT_STREAM_

// Method definition object for this extension, these argumens mean:
// ml_name: The name of the method
// ml_meth: Function pointer to the method implementation
// ml_flags: Flags indicating special features of this method, such as
//          accepting arguments, accepting keyword arguments, being a
//          class method, or being a static method of a class.
// ml_doc:  Contents of this method's docstring
static PyMethodDef bslext_methods[] = { 
    {   
        "getNumberOfCards", getNumberOfCards, METH_NOARGS,
        "Get number of Cards installed"
    },
	{   
        "getVersion", getVersion, METH_VARARGS,
        "Get Version of board, fpga, driver, api"
    },  
	{   
        "getLinkStatus", getLinkStatus, METH_VARARGS,
        "Get Operational status for each ports"
    },  
	{   
        "setPortMode", setPortMode, METH_VARARGS,
        "Configure Port mode (Normal/Interactive)"
    },  
	{   
        "getLinkStats", getLinkStats, METH_VARARGS,
        "Get Link Statistics information"
    },  
	{   
        "setPortActive", setPortActive, METH_VARARGS,
        "Enable/Disable port"
    },  
	{   
        "setCaptureStartStop", setCaptureStartStop, METH_VARARGS,
        "Start/Stop packet sniffering"
    },  
	{   
        "setLatency", setLatency, METH_VARARGS,
        "Set Latency"
    },  
	{   
        "setControlCommand", setControlCommand, METH_VARARGS,
        "Set Control Command"
    },  
	{   
        "enableStream", enableStream, METH_VARARGS,
        "enable/disable Stream"
    },  
#ifdef _IMPLEMENT_STREAM_
	{   
        "setStreamDetail", setStreamDetail, METH_VARARGS,
        "Configure Detail Stream"
    },  
#endif
    {NULL, NULL, 0, NULL}
};

// Module definition
// The arguments of this structure tell Python what to call your extension,
// what it's methods are and where to look for it's method definitions
static struct PyModuleDef bslext_definition = { 
    PyModuleDef_HEAD_INIT,
    "bslext",
    "A Python module that controls BSL from C code.",
    -1, 
    bslext_methods
};

// Module initialization
// Python calls this function when importing your extension. It is important
// that this function is named PyInit_[[your_module_name]] exactly, and matches
// the name keyword argument in setup.py's setup() call.
PyMODINIT_FUNC PyInit_bslext(void) {
    Py_Initialize();
    return PyModule_Create(&bslext_definition);
}
