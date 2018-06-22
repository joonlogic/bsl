/********************************************************************
 *  FILE   : bsl_msg_handle.c
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "bsl_type.h"
#include "bsl_dbg.h"
#include "bsl_system.h"
#include "bsl_msgif.h"
#include "bsl_proto.h"
#include "bsl_register.h"
#include "bsl_api.h"

static void printPdrEthernet( T_PDR* pdr );
static void printPdrVLAN( T_PDR* pdr );
static void printPdrISL( T_PDR* pdr );
static void printPdrMPLS( T_PDR* pdr );
static void printPdrIp4( T_PDR* pdr );
static void printPdrIp6( T_PDR* pdr );
static void printPdrIp4OverIp6( T_PDR* pdr );
static void printPdrIp6OverIp4( T_PDR* pdr );
static void printPdrArp( T_PDR* pdr );
static void printPdrTcp( T_PDR* pdr );
static void printPdrUdp( T_PDR* pdr );
static void printPdrIcmp( T_PDR* pdr );
static void printPdrIgmpv2( T_PDR* pdr );
static void printPdrUDF( T_PDR* pdr );

static void assignPdrNull( T_PDR* pdr, T_Protocol* proto );								// 2016.12.20 by dgjung
static void assignPdrEthernet( T_PDR* pdr, T_Protocol* proto );
static void assignPdrVLAN( T_PDR* pdr, T_Protocol* proto );
static void assignPdrISL( T_PDR* pdr, T_Protocol* proto );
static void assignPdrMPLS( T_PDR* pdr, T_Protocol* proto );
static void assignPdrIp4( T_PDR* pdr, T_Protocol* proto );
static void assignPdrIp6( T_PDR* pdr, T_Protocol* proto );
static void assignPdrIp4OverIp6( T_PDR* pdr, T_Protocol* proto );
static void assignPdrIp6OverIp4( T_PDR* pdr, T_Protocol* proto );
static void assignPdrArp( T_PDR* pdr, T_Protocol* proto );
static void assignPdrTcp( T_PDR* pdr, T_Protocol* proto );
static void assignPdrUdp( T_PDR* pdr, T_Protocol* proto );
static void assignPdrIcmp( T_PDR* pdr, T_Protocol* proto );
static void assignPdrIgmpv2( T_PDR* pdr, T_Protocol* proto );
//static void assignPdrUDF( T_PDR* pdr, T_Protocol* proto );

typedef void (*print_T_PDR_f_ptr_type) ( T_PDR* pdr );
print_T_PDR_f_ptr_type
    print_T_PDR_f_ptr[SIZE_MAX_PROTOCOL_ID] =
{
	[0] = NULL,
	[21] = (print_T_PDR_f_ptr_type)&printPdrEthernet, 
	[22] = (print_T_PDR_f_ptr_type)&printPdrVLAN, 
	[23] = (print_T_PDR_f_ptr_type)&printPdrISL, 
	[24] = (print_T_PDR_f_ptr_type)&printPdrMPLS, 
	[31] = (print_T_PDR_f_ptr_type)&printPdrIp4, 
	[32] = (print_T_PDR_f_ptr_type)&printPdrIp6, 
	[33] = (print_T_PDR_f_ptr_type)&printPdrIp4OverIp6, 
	[34] = (print_T_PDR_f_ptr_type)&printPdrIp6OverIp4, 
	[35] = (print_T_PDR_f_ptr_type)&printPdrArp, 
	[41] = (print_T_PDR_f_ptr_type)&printPdrTcp, 
	[42] = (print_T_PDR_f_ptr_type)&printPdrUdp, 
	[43] = (print_T_PDR_f_ptr_type)&printPdrIcmp, 
	[44] = (print_T_PDR_f_ptr_type)&printPdrIgmpv2, 
	[51] = (print_T_PDR_f_ptr_type)&printPdrUDF,
};

typedef void (*assign_T_PDR_f_ptr_type) ( T_PDR* pdr, T_Protocol* proto );
static assign_T_PDR_f_ptr_type
    assign_T_PDR_f_ptr[SIZE_MAX_PROTOCOL_ID] =
{
	[0] = NULL,
	[1] = (assign_T_PDR_f_ptr_type)&assignPdrNull, 									// 2016.12.20 by dgjung
	[21] = (assign_T_PDR_f_ptr_type)&assignPdrEthernet, 
	[22] = (assign_T_PDR_f_ptr_type)&assignPdrVLAN, 
	[23] = (assign_T_PDR_f_ptr_type)&assignPdrISL, 
	[24] = (assign_T_PDR_f_ptr_type)&assignPdrMPLS, 
	[31] = (assign_T_PDR_f_ptr_type)&assignPdrIp4, 
	[32] = (assign_T_PDR_f_ptr_type)&assignPdrIp6, 
	[33] = (assign_T_PDR_f_ptr_type)&assignPdrIp4OverIp6, 
	[34] = (assign_T_PDR_f_ptr_type)&assignPdrIp6OverIp4, 
	[35] = (assign_T_PDR_f_ptr_type)&assignPdrArp, 
	[41] = (assign_T_PDR_f_ptr_type)&assignPdrTcp, 
	[42] = (assign_T_PDR_f_ptr_type)&assignPdrUdp, 
	[43] = (assign_T_PDR_f_ptr_type)&assignPdrIcmp, 
	[44] = (assign_T_PDR_f_ptr_type)&assignPdrIgmpv2, 
//	[51] = (assign_T_PDR_f_ptr_type)&assignPdrUDF,
};


static EnumResultCode make_msgid_103( 
		T_MSGIF_103_RESP_UNIT* resp, 
		unsigned int cardid, 
		unsigned int portid, 
		T_LinkStats* stats );

void print_msgid_setcmd_resp( T_MSGIF_SETCMD_RESP* replyp );
void print_msgid_NYI( T_MSGIF_NYI_RESP* replyp );
void print_msgid_100( T_MSGIF_100_RESP* replyp );
void print_msgid_101( T_MSGIF_101_RESP* replyp );
void print_msgid_103( T_MSGIF_103_RESP* replyp );
void print_msgid_109_req( T_MSGIF_109_REQ_UNIT* requnit );
void print_msgid_110_req( T_MSGIF_110_REQ_UNIT* requnit, int reqlen );
void copy_msgid_110_stream_req( T_MSGIF_110_REQ_UNIT* requnit, T_Stream* stream );
void copy_msgid_110_req( T_MSGIF_110_REQ_UNIT* requnit, int reqlen, T_Protocol* proto );

void print_msg_hdr( T_MSGIF_HDR* hdr )
{
	BSL_MSG(("delim   : 0x%08X ( %d )\n", ntohl(hdr->delim), ntohl(hdr->delim) ));
	BSL_MSG(("msgid   : 0x%08X ( %d )\n", ntohl(hdr->id), ntohl(hdr->id) ));
	BSL_MSG(("type    : 0x%08X ( %d )\n", ntohl(hdr->type), ntohl(hdr->type) ));
	BSL_MSG(("length  : 0x%08X ( %d )\n", ntohl(hdr->length), ntohl(hdr->length) ));
	BSL_MSG(("nrecord : 0x%08X ( %d )\n", ntohl(hdr->nrecord), ntohl(hdr->nrecord) ));
}

void bsl_swap32( void* ptr, int length );

// Extern functions
extern EnumResultCode bsl_getLinkStatus( int cardid, T_Card* card );
extern EnumResultCode bsl_getLinkStats( int cardid, int portid, T_LinkStats* stat );
extern EnumResultCode bsl_setPortActive( int cardid, int portid, EnumPortActive enable );
extern EnumResultCode bsl_setControlCommand( int portsel, int cardid, int portid, int streamid, EnumCommand command, unsigned long long timesec, unsigned int, unsigned long long , unsigned int ); // 2017.7.15 by dgjung
extern EnumResultCode bsl_enableStream( int cardid, int portid, int streamid, int enable );
extern EnumResultCode bsl_setStreamDetail( int cardid, int portid, int streamid, int groupid, T_Stream* stream, T_Protocol* proto );			// 2017.7.15 by dgjung

/*****************************************************************************/
/* Message Id NYI : Get Chassis Information                                  */
/*****************************************************************************/
EnumResultCode 
bsl_handle_msgid_NYI(
		void* msgdata, 
		unsigned int msglen, 
		void** msgreply, 
		unsigned int* msgreplylen )
{	
	T_MSGIF_NYI_REQ* reqp = NULL;
	T_MSGIF_NYI_RESP* resp = NULL;
	T_MSGIF_NYI_REQ_UNIT* requnit = NULL;
	T_MSGIF_NYI_RESP_UNIT* respunit = NULL;
	unsigned int retval = 0;
	unsigned int resplen = sizeof( T_MSGIF_HDR );
	int nrecord = 0;
	unsigned int cardid;
	int i = 0;
	T_Chassis chassis={0,};

	BSL_MSG(("%s enter\n", __func__));

	//1. Get Request information
	reqp = (T_MSGIF_HDR*)msgdata;
	nrecord = MSGIF_GET_NRECORD( reqp );
	requnit = (T_MSGIF_NYI_REQ_UNIT*)MSGIF_GET_BODY_PTR(reqp);

	//2. Get Ready for Response information
	resp = malloc( sizeof( T_MSGIF_NYI_RESP ) );
	BSL_CHECK_NULL( resp, ResultMsgIfMsgHandleError );

	retval = bsl_getNumberOfCards( &nrecord );
	BSL_CHECK_RESULT( retval, ResultGeneralError );

	retval = bsl_getChassis( &chassis );
	BSL_CHECK_RESULT( retval, ResultGeneralError );

	for( i=0; i<nrecord; i++ ) {
		resplen += sizeof( T_MSGIF_NYI_RESP_UNIT );
		resp = realloc( resp, resplen );
		respunit = (T_MSGIF_NYI_RESP_UNIT*)MSGIF_GET_BODY_PTR(resp);
		respunit += i;
		respunit->cardid = htonl(i);
		respunit->nports = 2; //TODO: fixed
		respunit->linkSpeed = chassis.card[i].port[0].linkType == LinkType10GE ? 0 : 1;
	}

	memcpy( resp, reqp, sizeof( T_MSGIF_HDR ) );
	MSGIF_SET_TYPE( resp, MSGIF_TYPE_REPLY );
	MSGIF_SET_LENGTH( resp, resplen-sizeof(T_MSGIF_HDR) );
	MSGIF_SET_NRECORD( resp, nrecord );

	*msgreply = resp;
	*msgreplylen = resplen;
	
	free( msgdata );

	BSL_MSG(("%s: reply messages ---------\n", __func__ ));
	print_msgid_NYI( resp );

	return ResultSuccess;
}

void print_msgid_NYI( T_MSGIF_NYI_RESP* resp )
{
	int i;
	int nrecord;
	T_MSGIF_NYI_RESP_UNIT* respunit;
	print_msg_hdr( (T_MSGIF_HDR*)resp );

	respunit = (T_MSGIF_NYI_RESP_UNIT*)MSGIF_GET_BODY_PTR(resp);
	nrecord = MSGIF_GET_NRECORD( resp );
	for( i=0; i<nrecord; i++ ) {
		BSL_MSG(("card        : %x\n", ntohl(respunit->cardid) ));
		BSL_MSG(("nports      : %x\n", ntohl(respunit->nports) ));
		BSL_MSG(("linkSpeed   : %d\n", ntohl(respunit->linkSpeed) ));
		respunit++;
	}
}

//*****************************************************************************/
/* Message Id 100 : Get Chassis Information                                  */
/*****************************************************************************/
EnumResultCode 
bsl_handle_msgid_100(
		void* msgdata, 
		unsigned int msglen, 
		void** msgreply, 
		unsigned int* msgreplylen )
{	
	T_MSGIF_100_REQ* reqp = NULL;
	T_MSGIF_100_RESP* resp = NULL;
	T_MSGIF_100_REQ_UNIT* requnit = NULL;
	T_MSGIF_100_RESP_UNIT* respunit = NULL;
	unsigned int retval = 0;
	unsigned int resplen = sizeof( T_MSGIF_HDR );
	int nrecord = 0;
	unsigned int cardid=0;
	unsigned long long fpga;
	int i = 0;
	T_Chassis chassis={0,};

	BSL_MSG(("%s enter\n", __func__));

	//1. Get Request information
	reqp = (T_MSGIF_HDR*)msgdata;
	nrecord = MSGIF_GET_NRECORD( reqp );
	requnit = (T_MSGIF_100_REQ_UNIT*)MSGIF_GET_BODY_PTR(reqp);

	//2. Get Ready for Response information
	resp = malloc( sizeof( T_MSGIF_100_RESP ) );
	BSL_CHECK_NULL( resp, ResultMsgIfMsgHandleError );

	resplen += 	sizeof( T_MSGIF_100_RESP_UNIT );
	retval = bsl_setRegister( 
			cardid,
			CommandRegRead,
			(VERR<<3),
			&fpga
			); 
	respunit->fpga = htonll( fpga );
	respunit->api = htonl( VERSION_BSL_API_INT );

	memcpy( resp, reqp, sizeof( T_MSGIF_HDR ) );
	MSGIF_SET_TYPE( resp, MSGIF_TYPE_REPLY );
	MSGIF_SET_LENGTH( resp, resplen-sizeof(T_MSGIF_HDR) );
	MSGIF_SET_NRECORD( resp, nrecord );

	*msgreply = resp;
	*msgreplylen = resplen;
	
	free( msgdata );

	BSL_MSG(("%s: reply messages ---------\n", __func__ ));
	print_msgid_100( resp );

	return ResultSuccess;
}

void print_msgid_100( T_MSGIF_100_RESP* resp )
{
	int i;
	int nrecord;
	T_MSGIF_100_RESP_UNIT* respunit;
	print_msg_hdr( (T_MSGIF_HDR*)resp );

	respunit = (T_MSGIF_100_RESP_UNIT*)MSGIF_GET_BODY_PTR(resp);
	nrecord = MSGIF_GET_NRECORD( resp );
	for( i=0; i<nrecord; i++ ) {
		BSL_MSG(("api       : %x\n", ntohl(respunit->api) ));
		BSL_MSG(("fpga      : %x\n", ntohll(respunit->fpga) ));
		respunit++;
	}
}

/*****************************************************************************/
/* Message Id 101 : Get Link Status                                          */
/*****************************************************************************/
EnumResultCode 
bsl_handle_msgid_101(
		void* msgdata, 
		unsigned int msglen, 
		void** msgreply, 
		unsigned int* msgreplylen )
{	
	T_MSGIF_101_REQ* reqp = NULL;
	T_MSGIF_101_RESP* resp = NULL;
	T_MSGIF_101_REQ_UNIT* requnit = NULL;
	T_MSGIF_101_RESP_UNIT* respunit = NULL;
	unsigned int retval = 0;
	unsigned int resplen = sizeof( T_MSGIF_HDR );
	int nrecord = 0;
	unsigned int cardid;
	int i = 0;
	unsigned long long value = 0ll;

	BSL_MSG(("%s enter\n", __func__));

	//1. Get Request information
	reqp = (T_MSGIF_HDR*)msgdata;
	nrecord = MSGIF_GET_NRECORD( reqp );
	requnit = (T_MSGIF_101_REQ_UNIT*)MSGIF_GET_BODY_PTR(reqp);

	//2. Get Ready for Response information
	resp = malloc( sizeof( T_MSGIF_101_RESP ) );
	BSL_CHECK_NULL( resp, ResultMsgIfMsgHandleError );

	for( i=0; i<nrecord; i++ ) {
		T_Card card;
		cardid = ntohl( requnit->cardid );

		retval = bsl_getLinkStatus( cardid, &card );
		BSL_CHECK_RESULT( retval, ResultGeneralError );
	
		resplen += sizeof( T_MSGIF_101_RESP_UNIT );
		resp = realloc( resp, resplen );
		respunit = (T_MSGIF_101_RESP_UNIT*)MSGIF_GET_BODY_PTR(resp);
		respunit += i;
		retval = bsl_setRegister( 
				cardid,
				CommandRegRead,
				(DBGR<<3),
				&value
				); 
		respunit->DBGR = htonl( (unsigned int)value );

		respunit->link0 = htonl( card.port[0].link.operState + 1 ); //+1 means offset
		respunit->link1 = htonl( card.port[1].link.operState + 1 );

		requnit += sizeof(T_MSGIF_101_REQ_UNIT)/sizeof(int);
	}

	memcpy( resp, reqp, sizeof( T_MSGIF_HDR ) );
	MSGIF_SET_TYPE( resp, MSGIF_TYPE_REPLY );
	MSGIF_SET_LENGTH( resp, resplen-sizeof(T_MSGIF_HDR) );
	MSGIF_SET_NRECORD( resp, nrecord );

	*msgreply = resp;
	*msgreplylen = resplen;
	
	free( msgdata );

	BSL_MSG(("%s: reply messages ---------\n", __func__ ));
	print_msgid_101( resp );

	return ResultSuccess;
}

void print_msgid_101( T_MSGIF_101_RESP* resp )
{
	int i;
	int nrecord;
	T_MSGIF_101_RESP_UNIT* respunit;
	print_msg_hdr( (T_MSGIF_HDR*)resp );

	respunit = (T_MSGIF_101_RESP_UNIT*)MSGIF_GET_BODY_PTR(resp);
	nrecord = MSGIF_GET_NRECORD( resp );
	for( i=0; i<nrecord; i++ ) {
		BSL_MSG(("DBGR      : %x\n", ntohl(respunit->DBGR) ));
		BSL_MSG(("link0  : %d\n", ntohl(respunit->link0) ));
		BSL_MSG(("link1  : %d\n", ntohl(respunit->link1) ));
		respunit++;
	}
}

/*****************************************************************************/
/* Message Id 102 : Set Port Mode                                            */
/*****************************************************************************/
EnumResultCode
bsl_setPortMode( int cardid, int portid, EnumPortOpMode mode );
	
EnumResultCode 
bsl_handle_msgid_102(
		void* msgdata, 
		unsigned int msglen, 
		void** msgreply, 
		unsigned int* msgreplylen )
{
	T_MSGIF_102_REQ* reqp = NULL;
	T_MSGIF_102_REQ_UNIT* requnit = NULL;
	T_MSGIF_102_RESP* replyp = NULL;
	T_MSGIF_HDR* hdr = NULL;
	const static int nport = 1;
	unsigned int retval = 0;

	BSL_MSG(("%s enter\n", __func__));

	reqp = (T_MSGIF_102_REQ*)msgdata;
	requnit = (T_MSGIF_102_REQ_UNIT*)MSGIF_GET_BODY_PTR(reqp);

	replyp = malloc( sizeof( T_MSGIF_102_RESP ) );
	BSL_CHECK_NULL( replyp, ResultMsgIfMsgHandleError );

	*msgreply = replyp;
	*msgreplylen = sizeof( T_MSGIF_102_RESP );

	hdr = (T_MSGIF_HDR*)replyp;
	memcpy( hdr, msgdata, sizeof( T_MSGIF_HDR ) );
	MSGIF_SET_TYPE( hdr, MSGIF_TYPE_REPLY );
	MSGIF_SET_LENGTH( hdr, *msgreplylen-sizeof(T_MSGIF_HDR) );
	MSGIF_SET_NRECORD( hdr, nport );

	//Set port mode to device
	retval = bsl_setPortMode( 
			ntohl(requnit->cardid), 
			ntohl(requnit->portid), 
			ntohl(requnit->portmode) );

	replyp->result = htonl(retval);

	free( msgdata );

	BSL_MSG(("%s: reply messages ---------\n", __func__ ));
	print_msgid_setcmd_resp( replyp );

	return ResultSuccess;
}

void print_msgid_setcmd_resp( T_MSGIF_SETCMD_RESP* replyp )
{
	unsigned int* resultp = NULL;
	int nrecord = 0;

	resultp = &replyp->result;
	nrecord = ntohl(replyp->hdr.nrecord);

	print_msg_hdr( (T_MSGIF_HDR*)replyp );

	do {
		BSL_MSG(("[%d] result  : %d\n", nrecord, ntohl(*resultp++) ));
	} while( --nrecord > 0 );
}

/*****************************************************************************/
/* Message Id 103 : Get Statistics                                           */
/*****************************************************************************/
EnumResultCode 
bsl_handle_msgid_103(
		void* msgdata, 
		unsigned int msglen, 
		void** msgreply, 
		unsigned int* msgreplylen )
{
	T_MSGIF_103_REQ* reqp = NULL;
	T_MSGIF_103_RESP* resp = NULL;
	T_MSGIF_HDR* hdr = NULL;
	int nrecord = 0;
	int i = 0;
	EnumResultCode retval = 0;

	BSL_MSG(("%s enter\n", __func__));

	reqp = (T_MSGIF_103_REQ*)msgdata;
	nrecord = MSGIF_GET_NRECORD( reqp );

	resp = malloc( sizeof( T_MSGIF_103_RESP ) + 
			( nrecord*sizeof( T_MSGIF_103_RESP_UNIT ) ) );
	BSL_CHECK_NULL( resp, ResultMsgIfMsgHandleError );

	*msgreply = resp;
	*msgreplylen = sizeof( T_MSGIF_103_RESP ) + 
				( nrecord*sizeof( T_MSGIF_103_RESP_UNIT ) );

	hdr = (T_MSGIF_HDR*)resp;
	memcpy( hdr, msgdata, sizeof( T_MSGIF_HDR ) );
	MSGIF_SET_TYPE( hdr, MSGIF_TYPE_REPLY );
	MSGIF_SET_LENGTH( hdr, *msgreplylen-sizeof(T_MSGIF_HDR) );
	MSGIF_SET_NRECORD( hdr, nrecord );

	T_MSGIF_103_REQ_UNIT* reqpunit = (T_MSGIF_103_REQ_UNIT*)(reqp+1);
	T_MSGIF_103_RESP_UNIT* respunit = (T_MSGIF_103_RESP_UNIT*)(resp+1);

	for( i=0; i<nrecord; i++ ) {
		T_LinkStats stats = {0,};
		
		//Get statistics information from device
		retval = bsl_getLinkStats( 
				ntohl(reqpunit->cardid), 
				ntohl(reqpunit->portid), 
				&stats );

		make_msgid_103( respunit, 
				ntohl(reqpunit->cardid), 
				ntohl(reqpunit->portid), 
				&stats );

		reqpunit ++;
		respunit ++;
	}

	free( msgdata );

	BSL_MSG(("%s: reply messages ---------\n", __func__ ));
	print_msgid_103( resp );

	return retval;
}

EnumResultCode
make_msgid_103( 
		T_MSGIF_103_RESP_UNIT* resp, 
		unsigned int cardid, 
		unsigned int portid, 
		T_LinkStats* stats ) 
{
	BSL_CHECK_NULL( resp, ResultNullArg );
	BSL_CHECK_NULL( stats, ResultNullArg );

	resp->cardid = htonl( cardid );
	resp->portid = htonl( portid );
	resp->frameSentRate = htonl( stats->framesSentRate );
	resp->validFrameRxRate = htonl( stats->validFramesRxRate );
	resp->byteSentRate = htonl( stats->bytesSentRate );
	resp->byteRxRate = htonl( stats->bytesRxRate );
	resp->crcErrorFrameRxRate = htonl( stats->crcErrorFrameRxRate );
	resp->fragmentErrorFrameRxRate = htonl( stats->fragmentErrorFrameRxRate );
	resp->crcErrorByteRxRate = htonl( stats->crcErrorByteRxRate );
	resp->fragmentErrorByteRxRate = htonl( stats->fragmentErrorByteRxRate );
	resp->frameSent = htonll( stats->framesSent );
	resp->byteSent = htonll( stats->byteSent );
	resp->validFrameReceived = htonll( stats->validFrameReceived );
	resp->validByteReceived = htonll( stats->validByteReceived );
	resp->fragments = htonll( stats->fragments );
	resp->fragmentsByteReceived = htonll( stats->fragmentsByteReceived );
	resp->crcErrors = htonll( stats->crcErrors );
	resp->vlanTaggedFrame = htonll( stats->vlanTaggedFrames );
	resp->crcErrorByteReceived = htonll( stats->crcErrorByteReceived );
	resp->vlanByteReceived = htonll( stats->vlanByteReceived );
	resp->latencyMaximum = htonll( stats->latencyMaximum );
	resp->latencyMinimum = htonll( stats->latencyMinimum );
	resp->AverageLatIntPart = htonl( stats->AverageLatIntPart );
	resp->AverageLatFracPart = htonl( stats->AverageLatFracPart );
	resp->seqErrorPacketCount = htonll( stats->seqErrorPacketCount );
	resp->undersizeFrameCount = htonll( stats->undersizeFrameCount );
	resp->oversizeFrameCount = htonll( stats->oversizeFrameCount );
	resp->signatureFrameCount = htonll( stats->signatureFrameCount );					// 2016.12.26 by dgjung

	return ResultSuccess;
}

void print_msgid_103( T_MSGIF_103_RESP* replyp )
{
	int i=0;
	T_MSGIF_103_RESP_UNIT* respunit = (T_MSGIF_103_RESP_UNIT*)MSGIF_GET_BODY_PTR(replyp);
	print_msg_hdr( (T_MSGIF_HDR*)replyp );

	for( i=0; i<MSGIF_GET_NRECORD(replyp); i++ ) {
		BSL_MSG(("[%d].cardid             : %d\n", \
					i, ntohl(respunit->cardid) ));
		BSL_MSG(("[%d].portid             : %d\n", \
					i, ntohl(respunit->portid) ));
		BSL_MSG(("[%d].timestamp          : %d\n", \
					i, ntohl(respunit->timestamp) ));
		BSL_MSG(("[%d].frameSentRate      : %d\n", \
					i, ntohl(respunit->frameSentRate) ));
		BSL_MSG(("[%d].validFrameRxRate  : %d\n", \
					i, ntohl(respunit->validFrameRxRate) ));
		BSL_MSG(("[%d].byteSentRate      : %d\n", \
					i, ntohl(respunit->byteSentRate) ));
		BSL_MSG(("[%d].byteRxRate        : %d\n", \
					i, ntohl(respunit->byteRxRate) ));
		BSL_MSG(("[%d].frameSent         : %llu\n", \
					i, ntohll(respunit->frameSent) ));
		BSL_MSG(("[%d].byteSent          : %llu\n", \
					i, ntohll(respunit->byteSent) ));
		BSL_MSG(("[%d].validFrameReceived : %llu\n", \
					i, ntohll(respunit->validFrameReceived) ));
		BSL_MSG(("[%d].validByteReceived : %llu\n", \
					i, ntohll(respunit->validByteReceived) ));
		BSL_MSG(("[%d].fragments          : %llu\n", \
					i, ntohll(respunit->fragments) ));
		BSL_MSG(("[%d].fragmentsByteReceived : %llu\n", \
					i, ntohll(respunit->fragmentsByteReceived) ));
		BSL_MSG(("[%d].crcErrors          : %llu\n", \
					i, ntohll(respunit->crcErrors) ));
		BSL_MSG(("[%d].vlanTaggedFrame   : %llu\n", \
					i, ntohll(respunit->vlanTaggedFrame) ));
		BSL_MSG(("[%d].crcErrorByteReceived   : %llu\n", \
					i, ntohll(respunit->crcErrorByteReceived) ));
		BSL_MSG(("[%d].latencyMaximum   : %d\n", \
					i, ntohl(respunit->latencyMaximum) ));
		BSL_MSG(("[%d].latencyMinimum   : %d\n",  \
					i, ntohl(respunit->latencyMinimum) ));
		BSL_MSG(("[%d].totalLatencyValue   : %u\n",  \
					i, ntohl(respunit->AverageLatIntPart) ));
		BSL_MSG(("[%d].totalPacketCount   : %u\n",  \
					i, ntohl(respunit->AverageLatFracPart) ));
		BSL_MSG(("[%d].seqErrorPacketCount   : %llu\n",  \
					i, ntohll(respunit->seqErrorPacketCount) ));
		BSL_MSG(("[%d].undersizeFrameCount   : %llu\n",  \
					i, ntohll(respunit->undersizeFrameCount) ));
		BSL_MSG(("[%d].oversizeFrameCount   : %llu\n",  \
					i, ntohll(respunit->oversizeFrameCount) ));
		BSL_MSG(("[%d].signatureFrameCount   : %llu\n",  \
					i, ntohll(respunit->signatureFrameCount) ));					// 2016.12.26 by dgjung
		respunit++;
	}
}

/*****************************************************************************/
/* Message Id 104 : Set Port Active                                          */
/*****************************************************************************/
EnumResultCode 
bsl_handle_msgid_104(
		void* msgdata, 
		unsigned int msglen, 
		void** msgreply, 
		unsigned int* msgreplylen )
{
	T_MSGIF_104_REQ* reqp = NULL;
	T_MSGIF_104_REQ_UNIT* reqpunit = NULL;
	T_MSGIF_104_RESP* replyp = NULL;
	T_MSGIF_104_RESP_UNIT* respunit = NULL;
	T_MSGIF_HDR* hdr = NULL;
	const static int nport = 1;
	EnumResultCode retval = 0;
	int i = 0;
	int nrecord = 0;

	BSL_MSG(("%s enter\n", __func__));

	reqp = (T_MSGIF_104_REQ*)msgdata;
	nrecord = MSGIF_GET_NRECORD( reqp );
	reqpunit = (T_MSGIF_104_REQ_UNIT*)( msgdata + sizeof( T_MSGIF_HDR ) );

	replyp = malloc( sizeof( T_MSGIF_104_RESP ) + 
			( nrecord*sizeof( T_MSGIF_104_RESP_UNIT ) ) );
	BSL_CHECK_NULL( replyp, ResultMsgIfMsgHandleError );

	*msgreply = replyp;
	*msgreplylen = sizeof( T_MSGIF_104_RESP ) + 
		( nrecord*sizeof( T_MSGIF_104_RESP_UNIT ) );

	hdr = (T_MSGIF_HDR*)replyp;
	memcpy( hdr, msgdata, sizeof( T_MSGIF_HDR ) );
	MSGIF_SET_TYPE( hdr, MSGIF_TYPE_REPLY );
	MSGIF_SET_LENGTH( hdr, *msgreplylen-sizeof(T_MSGIF_HDR) );
	MSGIF_SET_NRECORD( hdr, nrecord );

	reqpunit = (T_MSGIF_104_REQ_UNIT*)( msgdata + sizeof( T_MSGIF_HDR ) );
	respunit = (T_MSGIF_104_RESP_UNIT*)(replyp+1);

	for( i=0; i<nrecord; i++ ) {

		//Set port mode to device
		retval = bsl_setPortActive( 
				ntohl(reqpunit->cardid), 
				ntohl(reqpunit->portid), 
				ntohl(reqpunit->enable) );

		//150820 ysh request
		retval = retval==ResultSuccess ? ntohl(reqpunit->enable)==0 ? 0 : 21 : retval;

		respunit->result = htonl(retval);
		reqpunit++;
		respunit++;
	}

	free( msgdata );

	BSL_MSG(("%s: reply messages ---------\n", __func__ ));
	print_msg_hdr( replyp );
	respunit = (T_MSGIF_104_RESP_UNIT*)(replyp+1);
	do {
		BSL_MSG(("[%d] result  : %d\n", nrecord, ntohl(respunit->result) ));
		respunit++;
	} while( --nrecord > 0 );

	return ResultSuccess;
}

/*****************************************************************************/
/* Message Id 105 : Capture Start & Stop                                     */
/*****************************************************************************/
EnumResultCode 
bsl_handle_msgid_105(
		void* msgdata, 
		unsigned int msglen, 
		void** msgreply, 
		unsigned int* msgreplylen )
{
	T_MSGIF_105_REQ* reqp = NULL;
	T_MSGIF_105_REQ_UNIT* reqpunit = NULL;
	T_MSGIF_105_RESP* replyp = NULL;
	T_MSGIF_105_RESP_UNIT* respunit = NULL;
	T_MSGIF_HDR* hdr = NULL;
	int i;
	int nrecord = 0;
	EnumResultCode retval = 0;

	BSL_MSG(("%s enter\n", __func__));

	reqp = (T_MSGIF_105_REQ*)msgdata;
	nrecord = MSGIF_GET_NRECORD( reqp );

	replyp = malloc( sizeof( T_MSGIF_105_RESP ) + 
			( nrecord*sizeof( T_MSGIF_105_RESP_UNIT ) ) );
	BSL_CHECK_NULL( replyp, ResultMsgIfMsgHandleError );

	*msgreply = replyp;
	*msgreplylen = sizeof( T_MSGIF_105_RESP ) + ( nrecord*sizeof( T_MSGIF_105_RESP_UNIT ) );

	hdr = (T_MSGIF_HDR*)replyp;
	memcpy( hdr, msgdata, sizeof( T_MSGIF_HDR ) );
	MSGIF_SET_TYPE( hdr, MSGIF_TYPE_REPLY );
	MSGIF_SET_LENGTH( hdr, *msgreplylen-sizeof(T_MSGIF_HDR) );
	MSGIF_SET_NRECORD( hdr, nrecord );

	reqpunit = (T_MSGIF_105_REQ_UNIT*)( msgdata + sizeof( T_MSGIF_HDR ) );
	respunit = (T_MSGIF_105_RESP_UNIT*)(replyp+1);

	for( i=0; i<nrecord; i++ ) {
		retval = bsl_setCaptureStartStop( 
				ntohl(reqpunit->cardid), 
				ntohl(reqpunit->portid), 
				ntohl(reqpunit->mode),
				ntohl(reqpunit->size),
				ntohl(reqpunit->start) );

		//150820 ysh request
		retval = retval==ResultSuccess ? ntohl(reqpunit->start)==0 ? 0 : 1 : retval;

		respunit->result = htonl(retval);
		reqpunit++;
		respunit++;
	}

	free( msgdata );

	BSL_MSG(("%s: reply messages ---------\n", __func__ ));
	print_msg_hdr( replyp );
	respunit = (T_MSGIF_105_RESP_UNIT*)(replyp+1);
	do {
		BSL_MSG(("[%d] result  : %d\n", nrecord, ntohl(respunit->result) ));
		respunit++;
	} while( --nrecord > 0 );

	return ResultSuccess;
}

/*****************************************************************************/
/* Message Id 106 : Latency Enable & Disable                                 */
/*****************************************************************************/
EnumResultCode 
bsl_handle_msgid_106(
		void* msgdata, 
		unsigned int msglen, 
		void** msgreply, 
		unsigned int* msgreplylen )
{
	T_MSGIF_106_REQ_UNIT* reqpunit = NULL;
	T_MSGIF_106_RESP_UNIT* respunit = NULL;
	T_MSGIF_106_RESP* replyp = NULL;
	T_MSGIF_HDR* hdr = NULL;
	const static int nport = 1;
	EnumResultCode retval = 0;
	int nrecord = 0;

	BSL_MSG(("%s enter\n", __func__));

	nrecord = MSGIF_GET_NRECORD( msgdata );

	replyp = malloc( sizeof( T_MSGIF_HDR ) + nrecord*sizeof( unsigned int ) );
	BSL_CHECK_NULL( replyp, ResultMsgIfMsgHandleError );

	reqpunit = (T_MSGIF_106_REQ_UNIT*)( msgdata + sizeof( T_MSGIF_HDR ) );
	respunit = (T_MSGIF_106_RESP_UNIT*)(replyp+1);

	*msgreply = replyp;
	*msgreplylen = nrecord*sizeof( unsigned int ) + sizeof(T_MSGIF_HDR);

	hdr = (T_MSGIF_HDR*)replyp;
	memcpy( hdr, msgdata, sizeof( T_MSGIF_HDR ) );
	MSGIF_SET_TYPE( hdr, MSGIF_TYPE_REPLY );
	MSGIF_SET_LENGTH( hdr, *msgreplylen-sizeof(T_MSGIF_HDR) );
	MSGIF_SET_NRECORD( hdr, nrecord );

	do {
		//Set latency to device
		retval = bsl_setLatency( 
				ntohl(reqpunit->cardid), 
				ntohl(reqpunit->portid), 
				ntohl(reqpunit->latency_enable),
				ntohl(reqpunit->sequence_enable),
				ntohl(reqpunit->signature_enable)						// 2016.12.26 by dgjung
				);

		//150820 ysh request
		retval = retval==ResultSuccess ? 
//			ntohl(reqpunit->latency_enable)==0 && ntohl(reqpunit->sequence_enable)==0 ? 0 : 
//			ntohl(reqpunit->latency_enable)==1 && ntohl(reqpunit->sequence_enable)==0 ? 1 : 
//			ntohl(reqpunit->latency_enable)==0 && ntohl(reqpunit->sequence_enable)==1 ? 2 : 
//			0 : retval;
			ntohl(reqpunit->latency_enable)==0 && ntohl(reqpunit->sequence_enable)==0 && ntohl(reqpunit->signature_enable)==0 ? 0 : 
			ntohl(reqpunit->latency_enable)==1 && ntohl(reqpunit->sequence_enable)==0 && ntohl(reqpunit->signature_enable)==0 ? 1 : 
			ntohl(reqpunit->latency_enable)==0 && ntohl(reqpunit->sequence_enable)==1 && ntohl(reqpunit->signature_enable)==0 ? 2 : 
			ntohl(reqpunit->latency_enable)==0 && ntohl(reqpunit->sequence_enable)==0 && ntohl(reqpunit->signature_enable)==1 ? 3 : 
			ntohl(reqpunit->latency_enable)==1 && ntohl(reqpunit->sequence_enable)==1 && ntohl(reqpunit->signature_enable)==1 ? 4 : 			// 2017.3.14 by dgjung
			0 : retval;

		respunit->result = htonl(retval);

		reqpunit++;
		respunit++;
	} while( --nrecord > 0 );

	free( msgdata );

	BSL_MSG(("%s: reply messages ---------\n", __func__ ));
	print_msg_hdr( replyp );
	respunit = (T_MSGIF_106_RESP_UNIT*)(replyp+1);
	do {
		BSL_MSG(("[%d] result  : %d\n", nrecord, ntohl(respunit->result) ));
		respunit++;
	} while( --nrecord > 0 );

	return ResultSuccess;
}


/*****************************************************************************/
/* Message Id 108 : Control Command                                          */
/*****************************************************************************/
EnumResultCode 
bsl_handle_msgid_108(
		void* msgdata, 
		unsigned int msglen, 
		void** msgreply, 
		unsigned int* msgreplylen )
{
	T_MSGIF_108_REQ_UNIT* reqpunit = NULL;
	T_MSGIF_108_RESP_UNIT* respunit = NULL;
	T_MSGIF_108_RESP* replyp = NULL;
	T_MSGIF_HDR* hdr = NULL;
	unsigned int retval = 0;
	int nrecord = 0;

	BSL_MSG(("%s enter\n", __func__));

	nrecord = MSGIF_GET_NRECORD( msgdata );

	replyp = malloc( sizeof( T_MSGIF_HDR ) + nrecord*sizeof( unsigned int ) );
	BSL_CHECK_NULL( replyp, ResultMsgIfMsgHandleError );

	reqpunit = (T_MSGIF_108_REQ_UNIT*)( msgdata + sizeof( T_MSGIF_HDR ) );
	respunit = (T_MSGIF_108_RESP_UNIT*)(replyp+1);

	*msgreply = replyp;
	*msgreplylen = nrecord*sizeof( unsigned int ) + sizeof(T_MSGIF_108_RESP);

	hdr = (T_MSGIF_HDR*)replyp;
	memcpy( hdr, msgdata, sizeof( T_MSGIF_HDR ) );
	MSGIF_SET_TYPE( hdr, MSGIF_TYPE_REPLY );
	MSGIF_SET_LENGTH( hdr, *msgreplylen-sizeof(T_MSGIF_HDR) );
	MSGIF_SET_NRECORD( hdr, nrecord );

	//Set port mode to device
	do {
		retval = bsl_setControlCommand( 
				ntohl(reqpunit->portsel), 
				ntohl(reqpunit->cardid), 
				ntohl(reqpunit->portid), 
				ntohl(reqpunit->streamid), 
				ntohl(reqpunit->command),
				ntohll(reqpunit->timesec),
				ntohl(reqpunit->netmode),
				ntohll(reqpunit->mac),
				ntohl(reqpunit->ip));

		respunit->result = 
			( ntohl(reqpunit->command) == CommandNetworkMode ) && ( retval == 0 ) ? 
			htonl(20) : htonl(retval); //20 is requested by YSH

		reqpunit++;
		respunit++;
	} while( --nrecord > 0 );

	free( msgdata );

	BSL_MSG(("%s: reply messages ---------\n", __func__ ));
	print_msgid_setcmd_resp( replyp );

	return ResultSuccess;
}


/*****************************************************************************/
/* Message Id 109 : Stream General Information                               */
/*****************************************************************************/
EnumResultCode 
bsl_handle_msgid_109(
		void* msgdata, 
		unsigned int msglen, 
		void** msgreply, 
		unsigned int* msgreplylen )
{
	T_MSGIF_109_REQ* reqp = NULL;
	T_MSGIF_109_RESP* replyp = NULL;
	T_MSGIF_HDR* hdr = NULL;
	unsigned int retval = 0;
	int nrecord = 0;

	BSL_MSG(("%s enter\n", __func__));

	reqp = (T_MSGIF_109_REQ*)msgdata;
	nrecord = MSGIF_GET_NRECORD( reqp );

	replyp = malloc( sizeof( T_MSGIF_109_RESP ) );
	BSL_CHECK_NULL( replyp, ResultMsgIfMsgHandleError );

	*msgreply = replyp;
	*msgreplylen = sizeof( T_MSGIF_109_RESP );

	hdr = (T_MSGIF_HDR*)replyp;
	memcpy( hdr, msgdata, sizeof( T_MSGIF_HDR ) );
	MSGIF_SET_TYPE( hdr, MSGIF_TYPE_REPLY );
	MSGIF_SET_LENGTH( hdr, *msgreplylen-sizeof(T_MSGIF_HDR) );
	MSGIF_SET_NRECORD( hdr, nrecord );

	do {
		T_MSGIF_109_REQ_UNIT* requnit = (T_MSGIF_109_REQ_UNIT*)MSGIF_GET_BODY_PTR(reqp);
		print_msgid_109_req( requnit );
		
		//Set Stream General Information						// 2017.7.15 by dgjung
/*	    retval = bsl_enableStream( 
				ntohl(requnit->cardid), 
				ntohl(requnit->portid), 
				ntohl(requnit->streamid), 
				ntohl(requnit->groupid), 
				ntohl(requnit->enable)
				);

		replyp->result = htonl(retval);
*/	} while(0);

	free( msgdata );

	BSL_MSG(("%s: reply messages ---------\n", __func__ ));
	print_msgid_setcmd_resp( replyp );

	return ResultSuccess;
}

void print_msgid_109_req( T_MSGIF_109_REQ_UNIT* requnit )
{
	BSL_MSG(("cardid            : %d\n", ntohl(requnit->cardid) ));
	BSL_MSG(("portid            : %d\n", ntohl(requnit->portid) ));
	BSL_MSG(("streamid          : %d\n", ntohl(requnit->streamid) ));
	BSL_MSG(("enable            : %d\n", ntohl(requnit->enable  ) ));
}

/*****************************************************************************/
/* Message Id 110 : Stream Detail Information                                */
/*****************************************************************************/
void copy_msgid_110_stream_req( T_MSGIF_110_REQ_UNIT* requnit, T_Stream* stream )
{
	BSL_CHECK_NULL( requnit, );
	BSL_CHECK_NULL( stream, );

	stream->portid = (requnit->portid);
	stream->streamid = (requnit->streamid);
	stream->groupid = (requnit->groupid);								// 2017.7.15 by dgjung	
	stream->enable = (requnit->enable);
	stream->control.control = (requnit->control.control);
	stream->control.returnToId = (requnit->control.returnToId);
	stream->control.loopCount = (requnit->control.loopCount);
	stream->control.pktsPerBurst = (requnit->control.pktsPerBurst);
	stream->control.burstsPerStream = (requnit->control.burstsPerStream);
	stream->control.rateControl = (requnit->control.rateControl);
	stream->control.rateControlIntPart = (requnit->control.rateControlIntPart);
	stream->control.rateControlFracPart = (requnit->control.rateControlFracPart);
	stream->control.startTxDelay = (requnit->control.startTxDelay);
	stream->control.interBurstGapIntPart = (requnit->control.interBurstGapIntPart);
	stream->control.interBurstGapFracPart = (requnit->control.interBurstGapFracPart);
	stream->control.interStreamGapIntPart = (requnit->control.interStreamGapIntPart);
	stream->control.interStreamGapFracPart = (requnit->control.interStreamGapFracPart);
	stream->control.ifg = (requnit->control.ifg);
}

EnumResultCode 
bsl_handle_msgid_110(
		void* msgdata, 
		unsigned int msglen, 
		void** msgreply, 
		unsigned int* msgreplylen )
{
	T_MSGIF_110_REQ* reqp = NULL;
	T_MSGIF_110_RESP* replyp = NULL;
	T_MSGIF_HDR* hdr = NULL;
	unsigned int retval = 0;
	int reqlen = 0;
	int nrecord = 1;

	BSL_MSG(("%s enter\n", __func__));

	reqp = (T_MSGIF_110_REQ*)msgdata;
	reqlen = MSGIF_GET_LENGTH( reqp );

	replyp = malloc( sizeof( T_MSGIF_110_RESP ) );
	BSL_CHECK_NULL( replyp, ResultMsgIfMsgHandleError );

	*msgreply = replyp;
	*msgreplylen = sizeof( T_MSGIF_110_RESP );

	hdr = (T_MSGIF_HDR*)replyp;
	memcpy( hdr, msgdata, sizeof( T_MSGIF_HDR ) );
	MSGIF_SET_TYPE( hdr, MSGIF_TYPE_REPLY );
	MSGIF_SET_LENGTH( hdr, *msgreplylen-sizeof(T_MSGIF_HDR) );
	MSGIF_SET_NRECORD( hdr, nrecord );

	do {
		T_MSGIF_110_REQ_UNIT* requnit = (T_MSGIF_110_REQ_UNIT*)MSGIF_GET_BODY_PTR(reqp);
		T_Protocol proto = {0,};
		T_Stream stream = {0,};
		bsl_swap32( (void*)requnit, reqlen );
printf("J:%s> reqlen %d\n", __func__, reqlen );
		print_msgid_110_req( requnit, reqlen );
		copy_msgid_110_stream_req( requnit, &stream );
		copy_msgid_110_req( requnit, reqlen, &proto );
		
		if( requnit->enable != 0 ) {
			//Set Stream Detail Information
			retval = bsl_setStreamDetail( 
					requnit->cardid, 
					requnit->portid, 
					requnit->streamid, 
					requnit->groupid, 						// 2017.7.15 by dgjung
					&stream,
					&proto );
		}

		if( retval == 0 ) {
			//Set Stream Enable/Disable
			retval = bsl_enableStream( 
						requnit->cardid, 
						requnit->portid, 
						requnit->streamid, 
						requnit->enable
					);
		}

		replyp->result = htonl(retval);
	} while(0);

	free( msgdata );

	BSL_MSG(("%s: reply messages ---------\n", __func__ ));
	print_msgid_setcmd_resp( replyp );

	return ResultSuccess;
}

void copy_msgid_110_req( T_MSGIF_110_REQ_UNIT* requnit, int tpdrlen, T_Protocol* proto )
{
	T_PDR* pdr;

	BSL_CHECK_NULL( requnit, );
	BSL_CHECK_NULL( proto, );

	proto->streamid = requnit->streamid;

	int fsizerandlen = 
		requnit->framesize.fsizeSpec == FrameSizeRandom ? ( 32+32 )*sizeof(int) : 0;

	T_TUPLE_PLOAD_INFO* pload = (T_TUPLE_PLOAD_INFO*)((unsigned char*)(requnit->pload) + fsizerandlen);

	int alignedlen = pload->pvalidSize % 4 == 0 ? pload->pvalidSize :
				pload->pvalidSize + 4 - ( pload->pvalidSize % 4 );

	//in case of all '0' in 110 msg
	int mainlen = tpdrlen - ( sizeof(T_MSGIF_110_REQ_UNIT) + sizeof(T_TUPLE_PLOAD_INFO) + alignedlen + fsizerandlen );

	if( ( pload->dataPatternType != FrameDataTypeDownload ) && ( mainlen > 0 ) ) {
		tpdrlen -= ( sizeof(T_MSGIF_110_REQ_UNIT) + sizeof(T_TUPLE_PLOAD_INFO) + alignedlen + fsizerandlen - sizeof(T_TUPLE_PLOAD_INFO*) );

		pdr = (T_PDR*)( pload->pattern + alignedlen );

		do {
			int protocolid = pdr->protocolid;
			int pdrlen = pdr->length;
			tpdrlen -= ( pdrlen + sizeof(T_PDR) );

			printf ("======= protool id : %d\n", protocolid);

			if ( protocolid == ProtocolNull ) {						// 2016.12.20 by dgjung
				printf ("======== Hello BSL Test\n" );
				break;
			}

			if( protocolid >= SIZE_MAX_PROTOCOL_ID ) {
				fprintf( stderr, "%s: Out Of Range. protocolid %d, pdrlen %d\n", 
						__func__, protocolid, pdrlen );
				break;
			}

			if( assign_T_PDR_f_ptr[protocolid] )
				assign_T_PDR_f_ptr[protocolid]( pdr, proto );
			else {
				fprintf( stderr, "%s: No Such protocolid %d or NYI, pdrlen %d\n", 
						__func__, protocolid, pdrlen );
				break;
			}

			pdr = (void*)pdr + pdrlen + sizeof(T_PDR);
			
		} while( tpdrlen > sizeof(T_PDR) );
	}

	proto->fi.framesize.fsizeSpec = (requnit->framesize.fsizeSpec);
	proto->fi.framesize.sizeOrStep = (requnit->framesize.sizeOrStep);
	proto->fi.framesize.fsizeMin = (requnit->framesize.fsizeMin);
	proto->fi.framesize.fsizeMax = (requnit->framesize.fsizeMax);

	if( fsizerandlen > 0 ) {
		int i=0;
		unsigned int* randp = (unsigned int*)requnit->pload;
        BSL_MSG(("%s: randp %p \n\t", __func__, randp));
        for( i=0; i<32; i++ ) {
            BSL_MSG(("%04X ", randp[i] ));
            if(i%10==9) BSL_MSG(("\n\t"));
        }
        BSL_MSG(("\n"));
		memcpy( proto->fi.framesize.fsizeValueRand, requnit->pload, 32*sizeof(int) );
		memcpy( proto->fi.framesize.fsizeValueRandDiff, (unsigned char*)(requnit->pload)+32*sizeof(int), 32*sizeof(int) );
	}
	proto->fi.payloadType = (pload->dataPatternType);
	proto->fi.crc = pload->crc;
	proto->fi.pattern.payloadOffset = (pload->poffset);
	proto->fi.pattern.validSize = (pload->pvalidSize);

	if( FrameDataTypeRepeating == proto->fi.payloadType ) {
		int i,j;
		for( i=0; 
				i<proto->fi.pattern.validSize-(proto->fi.pattern.validSize%4); 
				i+=4 ) {
			for( j=0; j<4; j++ ) 
				proto->fi.pattern.payload[i+3-j] = pload->pattern[i+j];
		}
		for( i, j=0; j<proto->fi.pattern.validSize%4; i++,j++ ) {
			proto->fi.pattern.payload[i] = pload->pattern[proto->fi.pattern.validSize-1-j];
		}
	} 
	else {
		memcpy( proto->fi.pattern.payload, pload->pattern, 
				proto->fi.pattern.validSize );
	}

	BSL_MSG(("%s: Exit\n",__func__ ));
}
	
void print_msgid_110_req( T_MSGIF_110_REQ_UNIT* requnit, int reqlen )
{
	int fsizerandlen = 0;

printf("J:%s> Line %d reqlen %d\n", __func__, __LINE__, reqlen );
	BSL_MSG(("cardid            : %d\n", requnit->cardid) );
	BSL_MSG(("portid            : %d\n", requnit->portid) );
	BSL_MSG(("streamid          : %d\n", requnit->streamid) );
	BSL_MSG(("groupid           : %d\n", requnit->groupid) );							// 2017.7.15 by dgjung
	BSL_MSG(("enable            : %d\n", (requnit->enable) ));

	BSL_MSG(("control           : %d\n", (requnit->control.control) ));
	BSL_MSG(("returnToId        : %d\n", (requnit->control.returnToId) ));
	BSL_MSG(("loopCount         : %d\n", (requnit->control.loopCount) ));
	BSL_MSG(("pktsPerBurst      : %llu\n", (requnit->control.pktsPerBurst) ));
	BSL_MSG(("burstsPerStream   : %d\n", (requnit->control.burstsPerStream) ));
	BSL_MSG(("rateControl       : %d\n", (requnit->control.rateControl) ));
	BSL_MSG(("rateControlIntPart: %d\n", (requnit->control.rateControlIntPart) ));
	BSL_MSG(("rateControlFracPart : %d\n", (requnit->control.rateControlFracPart) ));
	BSL_MSG(("startTxDelay      : %d\n", (requnit->control.startTxDelay) ));
	BSL_MSG(("interBurstGapInt  : %d\n", (requnit->control.interBurstGapIntPart) ));
	BSL_MSG(("interBurstGapFrac : %d\n", (requnit->control.interBurstGapFracPart) ));
	BSL_MSG(("interStreamGapInt : %d\n", (requnit->control.interStreamGapIntPart) ));
	BSL_MSG(("interStreamGapFrac: %d\n", (requnit->control.interStreamGapFracPart) ));
	BSL_MSG(("ifg: %d\n", (requnit->control.ifg) ));

	BSL_MSG(("frameSizeSpec     : %d\n", (requnit->framesize.fsizeSpec) ));
	BSL_MSG(("sizeOrStep        : %d\n", (requnit->framesize.sizeOrStep) ));
	BSL_MSG(("frameSizeMin      : %d\n", (requnit->framesize.fsizeMin) ));
	BSL_MSG(("frameSizeMax      : %d\n", (requnit->framesize.fsizeMax) ));

	fsizerandlen = 
		requnit->framesize.fsizeSpec == FrameSizeRandom ? ( 32+32 )*sizeof(int) : 0;

	if( fsizerandlen > 0 ) {
		int i=0;
		unsigned int* randp = (unsigned int*)requnit->pload;
        BSL_MSG(("%s: randp %p \n\t", __func__, randp));
        for( i=0; i<32; i++ ) {
            BSL_MSG(("%04X ", randp[i]));
            if(i%10==9) BSL_MSG(("\n\t"));
        }
        BSL_MSG(("\n"));
	}

	T_TUPLE_PLOAD_INFO* pload = (T_TUPLE_PLOAD_INFO*)((unsigned char*)(requnit->pload) + fsizerandlen);

	BSL_MSG(("crc               : %d\n", (pload->crc) ));
	BSL_MSG(("dataPatternType   : %d\n", (pload->dataPatternType) ));
	BSL_MSG(("payloadOffset     : %d\n", (pload->poffset) ));
	BSL_MSG(("payloadValidSize  : %d\n", (pload->pvalidSize) ));
	BSL_MSG(("pattern           : %p\n", pload->pattern ));

	if( (pload->pvalidSize) != 0 ) {
		unsigned char* pchar = pload->pattern;
		int i=0;
		for( i=0; i<(pload->pvalidSize); i++ ) {
			if( i%16 == 0 ) BSL_MSG(("\n[%p]", pchar+i ));
			if( i%4 == 0 ) BSL_MSG((" "));
			BSL_MSG(("%02X", *pchar++ ));
			if( i > 2000 ) {
				BSL_MSG(("Too Long to print out. skipped.\n"));
				break;
			}
		}
	}

	int alignedlen = pload->pvalidSize % 4 == 0 ? pload->pvalidSize :
				pload->pvalidSize + 4 - ( pload->pvalidSize % 4 );

//	reqlen -= ( sizeof(T_MSGIF_110_REQ_UNIT) + alignedlen );
	reqlen -= ( sizeof(T_MSGIF_110_REQ_UNIT) + sizeof(T_TUPLE_PLOAD_INFO) + 
			alignedlen + fsizerandlen );

printf("J:%s> Line %d reqlen %d\n", __func__, __LINE__, reqlen );
	if( reqlen <= 0 ) {
		BSL_MSG(("%s:%d Exit reqlen\n",__func__, __LINE__ ));
		return;
	}

	if( pload->dataPatternType == FrameDataTypeDownload ) {
		BSL_MSG(("%s:%d Exit FrameDataTypeDownload\n",__func__, __LINE__ ));
		return;
	}

	T_PDR* pdr = (T_PDR*)( pload->pattern + alignedlen );
	BSL_MSG(("%s: pdr = %p\n", __func__, pdr ));
	BSL_CHECK_NULL( pdr, );

	while( reqlen > sizeof(T_PDR) ) { //reqlen should be bigger than sizeof(T_PDR)
		int protocolid = pdr->protocolid;
		int pdrlen = pdr->length;
		reqlen -= ( pdrlen + sizeof(T_PDR) );

		if( protocolid >= SIZE_MAX_PROTOCOL_ID ) {
			fprintf( stderr, "%s: Out Of Range. protocolid %d, pdrlen %d\n", 
					__func__, protocolid, pdrlen );
			continue;
		}
		if( print_T_PDR_f_ptr[protocolid] ) 
			print_T_PDR_f_ptr[protocolid]( pdr );

		pdr = (void*)pdr + pdrlen + sizeof(T_PDR);
	}

	BSL_MSG(("%s: Exit\n",__func__ ));
}

/*****************************************************************************/
/******** For PDR functions **************************************************/
/*****************************************************************************/
static void printPdrTupleEtherAddr( T_EtherAddrTuple* pMac )
{
	BSL_MSG(("\tmode         %d\n", pMac->mode ));
	BSL_MSG(("\taddr         %02X:%02X:%02X:%02X:%02X:%02X\n", \
				pMac->addr[0], pMac->addr[1], pMac->addr[2], \
				pMac->addr[3], pMac->addr[4], pMac->addr[5] ));
	BSL_MSG(("\trepeat count %d\n", pMac->repeatCount ));
	BSL_MSG(("\tstep         %d\n", pMac->step ));
}

static void printPdrTupleChecksum( T_ChecksumTuple* chksum )
{
	BSL_MSG(("\ttype         %d\n", chksum->type ));
	BSL_MSG(("\tvalue        %d(0x%X)\n", chksum->value, chksum->value ));
}

static void printPdrTupleIp4Addr( T_Ip4AddrTuple* ip4 )
{
	struct in_addr addr;
	addr.s_addr = htonl( ip4->addr );
	BSL_MSG(("\taddr         %08X(%s)\n", ip4->addr, inet_ntoa( addr ) ));
	addr.s_addr = htonl( ip4->mask );
	BSL_MSG(("\tmask         %08X(%s)\n", ip4->mask, inet_ntoa( addr ) ));
	BSL_MSG(("\trepeat       %d(0x%X)\n", ip4->repeat, ip4->repeat ));
	BSL_MSG(("\tmode         %d(0x%X)\n", ip4->mode, ip4->mode ));
}

static void printPdrTupleIp6Addr( T_Ip6AddrTuple* ip6 )
{
	int i;

    for( i=0; i<16; i+=4 ) 
        *(unsigned int*)(ip6->addr+i) = BSLSWAP32(*(unsigned int*)(ip6->addr+i));

	BSL_MSG(("\taddr         "));
	for( i=0; i<16; i+=2 ) {
		BSL_MSG(("%02X%02X", ip6->addr[i], ip6->addr[i+1] ));
		if( i<14 ) BSL_MSG((":"));
	}
	BSL_MSG(("\trepeat       %d(0x%X)\n", ip6->repeat, ip6->repeat ));
	BSL_MSG(("\tstep         %d(0x%X)\n", ip6->step, ip6->step ));
	BSL_MSG(("\tmode         %d(0x%X)\n", ip6->mode, ip6->mode ));
	BSL_MSG(("\tmask         %d(0x%X)\n", ip6->mask, ip6->mask ));
}

static void printPdrTupleInteger( T_CustomIntegerTuple* intval )
{
	BSL_MSG(("\tmode         %d(0x%X)\n", intval->mode, intval->mode ));
	BSL_MSG(("\tvalue        %d(0x%X)\n", intval->value, intval->value ));
	BSL_MSG(("\tstep         %d(0x%X)\n", intval->step, intval->step ));
	BSL_MSG(("\trepeat       %d(0x%X)\n", intval->repeat, intval->repeat ));
}

static void printPdrEthernet( T_PDR* pdr )
{
	T_PDR_Ethernet* ppdr;

	BSL_CHECK_NULL( pdr, );
	ppdr = (T_PDR_Ethernet*)pdr->pinfo;

	BSL_MSG(("%s: ---------------\n", __func__ ));
	BSL_MSG(("\tprotocolid   %d\n", pdr->protocolid ));
	BSL_MSG(("\tlength       %d\n", pdr->length ));
	BSL_MSG(("\tDest Ethernet Address\n"));
	printPdrTupleEtherAddr( &ppdr->dest );
	BSL_MSG(("\tSource Ethernet Address\n"));
	printPdrTupleEtherAddr( &ppdr->src );
}

static void printPdrVLAN( T_PDR* pdr )
{
	T_PDR_802_1Q_VLAN* ppdr;

	BSL_CHECK_NULL( pdr, );
	ppdr = (T_PDR_802_1Q_VLAN*)pdr->pinfo;

	BSL_MSG(("%s: ---------------\n", __func__ ));
	BSL_MSG(("\tprotocolid   %d\n", pdr->protocolid ));
	BSL_MSG(("\tlength       %d\n", pdr->length ));
	BSL_MSG(("\tmode         %d\n", ppdr->mode ));
	BSL_MSG(("\tpriority     %d(0x%X)\n", ppdr->vlan.priority, ppdr->vlan.priority ));
	BSL_MSG(("\tcfi          %d(0x%X)\n", ppdr->vlan.cfi     , ppdr->vlan.cfi      ));
	BSL_MSG(("\tvlanid       %d(0x%X)\n", ppdr->vlan.vlanid  , ppdr->vlan.vlanid   ));
	BSL_MSG(("\ttagProtocolId %d(0x%X)\n", 
				ppdr->vlan.tagProtocolId  , ppdr->vlan.tagProtocolId   ));
	BSL_MSG(("\tvlanIdCountMode %d(0x%X)\n", 
				ppdr->vlan.vlanIdCountMode, ppdr->vlan.vlanIdCountMode ));
	BSL_MSG(("\tcount        %d(0x%X)\n", ppdr->vlan.count   , ppdr->vlan.count    ));
	BSL_MSG(("\tbitMask      %d(0x%X)\n", ppdr->vlan.bitMask , ppdr->vlan.bitMask  ));

	if( ppdr->mode == VlanModeStack ) {
		BSL_MSG(("\t[2]priority     %d(0x%X)\n", 
					ppdr->vlanDup->priority, ppdr->vlanDup->priority ));
		BSL_MSG(("\t[2]cfi          %d(0x%X)\n", 
					ppdr->vlanDup->cfi     , ppdr->vlanDup->cfi      ));
		BSL_MSG(("\t[2]vlanid       %d(0x%X)\n", 
					ppdr->vlanDup->vlanid  , ppdr->vlanDup->vlanid   ));
		BSL_MSG(("\t[2]tagProtocolId %d(0x%X)\n", 
					ppdr->vlanDup->tagProtocolId  , ppdr->vlanDup->tagProtocolId   ));
		BSL_MSG(("\t[2]vlanIdCountMode %d(0x%X)\n", 
					ppdr->vlanDup->vlanIdCountMode, ppdr->vlanDup->vlanIdCountMode ));
		BSL_MSG(("\t[2]count        %d(0x%X)\n", 
					ppdr->vlanDup->count   , ppdr->vlanDup->count    ));
		BSL_MSG(("\t[2]bitMask      %d(0x%X)\n", 
					ppdr->vlanDup->bitMask , ppdr->vlanDup->bitMask  ));
	}
}

static void printPdrISL( T_PDR* pdr )
{
	T_PDR_ISL* ppdr;

	BSL_CHECK_NULL( pdr, );
	ppdr = (T_PDR_ISL*)pdr->pinfo;

	BSL_MSG(("%s: ---------------\n", __func__ ));
	BSL_MSG(("\tprotocolid   %d\n", pdr->protocolid ));
	BSL_MSG(("\tlength       %d\n", pdr->length ));
	BSL_MSG(("\tdest         %02X:%02X:%02X\n", \
				ppdr->dest[0], ppdr->dest[1], ppdr->dest[2] ));
	BSL_MSG(("\ttype         %d(0x%X)\n", ppdr->type    , ppdr->type     ));
	BSL_MSG(("\tuser         %d(0x%X)\n", ppdr->user  , ppdr->user   ));
	BSL_MSG(("\tsrc          %02X:%02X:%02X:%02X:%02X:%02X\n", \
				ppdr->src[0], ppdr->src[1], ppdr->src[2],\
				ppdr->src[3], ppdr->src[4], ppdr->src[5] ));
	BSL_MSG(("\tlength       %d(0x%X)\n", ppdr->length       , ppdr->length        ));
	BSL_MSG(("\tvlanid       %d(0x%X)\n", ppdr->vlanid, ppdr->vlanid ));
	BSL_MSG(("\tbpduInd      %d(0x%X)\n", ppdr->bpduInd, ppdr->bpduInd ));
	BSL_MSG(("\tindex        %d(0x%X)\n", ppdr->index, ppdr->index ));
}

static void printPdrMPLS( T_PDR* pdr )
{
	T_PDR_MPLS* ppdr;

	BSL_CHECK_NULL( pdr, );
	ppdr = (T_PDR_MPLS*)pdr->pinfo;

	BSL_MSG(("%s: ---------------\n", __func__ ));
	BSL_MSG(("\tprotocolid         %d\n", pdr->protocolid ));
	BSL_MSG(("\tlength             %d\n", pdr->length ));
	BSL_MSG(("\ttype               %d\n", ppdr->type ));
	BSL_MSG(("\tmpls.label         %d(0x%X)\n", ppdr->mpls.label, ppdr->mpls.label ));
	BSL_MSG(("\tmpls.exp           %d(0x%X)\n", ppdr->mpls.exp, ppdr->mpls.exp ));
	BSL_MSG(("\tmpls.s             %d(0x%X)\n", ppdr->mpls.s, ppdr->mpls.s ));
	BSL_MSG(("\tmpls.ttl           %d(0x%X)\n", ppdr->mpls.ttl, ppdr->mpls.ttl ));
	if( ppdr->type == MplsTypeMulticast ) {
		BSL_MSG(("\tmplsDup->label   %d(0x%X)\n", 
					ppdr->mplsDup->label, ppdr->mplsDup->label ));
		BSL_MSG(("\tmplsDup->exp     %d(0x%X)\n", 
					ppdr->mplsDup->exp, ppdr->mplsDup->exp ));
		BSL_MSG(("\tmplsDup->s       %d(0x%X)\n", 
					ppdr->mplsDup->s, ppdr->mplsDup->s ));
		BSL_MSG(("\tmplsDup->ttl     %d(0x%X)\n", 
					ppdr->mplsDup->ttl, ppdr->mplsDup->ttl ));
	}
}

static void printPdrIp4( T_PDR* pdr )
{
	T_PDR_Ip4* ppdr;

	BSL_CHECK_NULL( pdr, );
	ppdr = (T_PDR_Ip4*)pdr->pinfo;

	BSL_MSG(("%s: ---------------\n", __func__ ));
	BSL_MSG(("\tprotocolid   %d\n", pdr->protocolid ));
	BSL_MSG(("\tlength       %d\n", pdr->length ));
	BSL_MSG(("\tversion      %d(0x%X)\n", ppdr->version, ppdr->version ));
	BSL_MSG(("\ttos          %d(0x%X)\n", ppdr->tos, ppdr->tos ));
	BSL_MSG(("\ttlen         %d(0x%X)\n", ppdr->tlen, ppdr->tlen ));
	BSL_MSG(("\tid           %d(0x%X)\n", ppdr->id, ppdr->id ));
	BSL_MSG(("\tflags        %d(0x%X)\n", ppdr->flags, ppdr->flags ));
	BSL_MSG(("\tfragoffset   %d(0x%X)\n", ppdr->fragoffset, ppdr->fragoffset ));
	BSL_MSG(("\tttl          %d(0x%X)\n", ppdr->ttl, ppdr->ttl ));
	BSL_MSG(("\tproto        %d(0x%X)\n", ppdr->proto, ppdr->proto ));
	printPdrTupleChecksum( &ppdr->checksum );
	BSL_MSG(("\tSource Ip4 Address\n"));
	printPdrTupleIp4Addr( &ppdr->sip );
	BSL_MSG(("\tDest Ip4 Address\n"));
	printPdrTupleIp4Addr( &ppdr->dip );
	
	//TODO: IP4 options
}

static void printPdrIp6( T_PDR* pdr )
{
	T_PDR_Ip6* ppdr;

	BSL_CHECK_NULL( pdr, );
	ppdr = (T_PDR_Ip6*)pdr->pinfo;

	BSL_MSG(("%s: ---------------\n", __func__ ));
	BSL_MSG(("\tprotocolid   %d\n", pdr->protocolid ));
	BSL_MSG(("\tlength       %d\n", pdr->length ));
	BSL_MSG(("\tversion      %d(0x%X)\n", ppdr->version, ppdr->version ));
	BSL_MSG(("\ttclass       %d(0x%X)\n", ppdr->tclass, ppdr->tclass ));
	BSL_MSG(("\tflabel       %d(0x%X)\n", ppdr->flabel, ppdr->flabel ));
	BSL_MSG(("\tplen         %d(0x%X)\n", ppdr->plen, ppdr->plen ));
	BSL_MSG(("\tnextheader   %d(0x%X)\n", ppdr->nextheader, ppdr->nextheader ));
	BSL_MSG(("\thoplimit     %d(0x%X)\n", ppdr->hoplimit, ppdr->hoplimit ));
	BSL_MSG(("\tSource Ip6 Address\n"));
	printPdrTupleIp6Addr( &ppdr->sip );
	BSL_MSG(("\tDest Ip6 Address\n"));
	printPdrTupleIp6Addr( &ppdr->dip );
	
	//TODO: IP6 Next Header
}

static void printPdrIp4OverIp6( T_PDR* pdr )
{
	T_PDR_Ip6* ppdr6;
	T_PDR_Ip4* ppdr4;

	BSL_CHECK_NULL( pdr, );
	ppdr6 = (T_PDR_Ip6*)pdr->pinfo;

	printPdrIp6( (T_PDR*)ppdr6 );

	ppdr4 = ( (void*)ppdr6 ) + sizeof(T_PDR) + ((T_PDR*)ppdr6)->length;
	printPdrIp4( (T_PDR*)ppdr4 );
}

static void printPdrIp6OverIp4( T_PDR* pdr )
{
	T_PDR_Ip4* ppdr4;
	T_PDR_Ip6* ppdr6;

	BSL_CHECK_NULL( pdr, );
	ppdr4 = (T_PDR_Ip4*)pdr->pinfo;

	printPdrIp4( (T_PDR*)ppdr4 );

	ppdr6 = ( (void*)ppdr4 ) + sizeof(T_PDR) + ((T_PDR*)ppdr4)->length;
	printPdrIp6( (T_PDR*)ppdr6 );
}

static void printPdrArp( T_PDR* pdr )
{
	T_PDR_ARP* ppdr;

	BSL_CHECK_NULL( pdr, );
	ppdr = (T_PDR_ARP*)pdr->pinfo;

	BSL_MSG(("%s: ---------------\n", __func__ ));
	BSL_MSG(("\tprotocolid   %d\n", pdr->protocolid ));
	BSL_MSG(("\tlength       %d\n", pdr->length ));
	BSL_MSG(("\ttype      %d(0x%X)\n", ppdr->type, ppdr->type ));
	BSL_MSG(("\thwtype      %d(0x%X)\n", ppdr->hwtype, ppdr->hwtype ));
	BSL_MSG(("\tprotocoltype      %d(0x%X)\n", ppdr->protocoltype, ppdr->protocoltype ));
	BSL_MSG(("\thalength      %d(0x%X)\n", ppdr->halength, ppdr->halength ));
	BSL_MSG(("\tpalength      %d(0x%X)\n", ppdr->palength, ppdr->palength ));
	BSL_MSG(("\toperation      %d(0x%X)\n", ppdr->operation, ppdr->operation ));
	BSL_MSG(("\tSenderMac\n"));
	printPdrTupleEtherAddr( &ppdr->senderMac );
	BSL_MSG(("\tSender Ip Address\n"));
	printPdrTupleIp4Addr( &ppdr->senderIp );
	BSL_MSG(("\tTargetMac\n"));
	printPdrTupleEtherAddr( &ppdr->targetMac );
	BSL_MSG(("\tTarget Ip Address\n"));
	printPdrTupleIp4Addr( &ppdr->targetIp );
}

static void printPdrTcp( T_PDR* pdr )
{
	T_PDR_TCP* ppdr;

	BSL_CHECK_NULL( pdr, );
	ppdr = (T_PDR_TCP*)pdr->pinfo;

	BSL_MSG(("%s: ---------------\n", __func__ ));
	BSL_MSG(("\tprotocolid   %d\n", pdr->protocolid ));
	BSL_MSG(("\tlength       %d\n", pdr->length ));

	BSL_MSG(("\tSource Port\n"));
	printPdrTupleInteger( &ppdr->sport );
	BSL_MSG(("\tDest Port\n"));
	printPdrTupleInteger( &ppdr->dport );
	BSL_MSG(("\tseqnum       %d(0x%X)\n", ppdr->seqnum, ppdr->seqnum ));
	BSL_MSG(("\tacknum       %d(0x%X)\n", ppdr->acknum, ppdr->acknum ));
	BSL_MSG(("\toffset       %d(0x%X)\n", ppdr->offset, ppdr->offset ));
	BSL_MSG(("\tflag_res     %d(0x%X)\n", ppdr->flag_res, ppdr->flag_res ));
	BSL_MSG(("\tflag_urg     %d(0x%X)\n", ppdr->flag_urg, ppdr->flag_urg ));
	BSL_MSG(("\tflag_ack     %d(0x%X)\n", ppdr->flag_ack, ppdr->flag_ack ));
	BSL_MSG(("\tflag_psh     %d(0x%X)\n", ppdr->flag_psh, ppdr->flag_psh ));
	BSL_MSG(("\tflag_rst     %d(0x%X)\n", ppdr->flag_rst, ppdr->flag_rst ));
	BSL_MSG(("\tflag_syn     %d(0x%X)\n", ppdr->flag_syn, ppdr->flag_syn ));
	BSL_MSG(("\tflag_fin     %d(0x%X)\n", ppdr->flag_fin, ppdr->flag_fin ));
	BSL_MSG(("\twindows      %d(0x%X)\n", ppdr->windows, ppdr->windows ));
	printPdrTupleChecksum( &ppdr->checksum );
	BSL_MSG(("\turgent       %d(0x%X)\n", ppdr->urgent, ppdr->urgent ));
	BSL_MSG(("\toptionLength %d(0x%X)\n", ppdr->optionLength, ppdr->optionLength ));
}

static void printPdrUdp( T_PDR* pdr )
{
	T_PDR_UDP* ppdr;

	BSL_CHECK_NULL( pdr, );
	ppdr = (T_PDR_UDP*)pdr->pinfo;

	BSL_MSG(("%s: ---------------\n", __func__ ));
	BSL_MSG(("\tprotocolid   %d\n", pdr->protocolid ));
	BSL_MSG(("\tlength       %d\n", pdr->length ));

	BSL_MSG(("\tSource Port\n"));
	printPdrTupleInteger( &ppdr->sport );
	BSL_MSG(("\tDest Port\n"));
	printPdrTupleInteger( &ppdr->dport );
	BSL_MSG(("\toverride     %d(0x%X)\n", ppdr->override, ppdr->override ));
	BSL_MSG(("\tval          %d(0x%X)\n", ppdr->val, ppdr->val ));
	printPdrTupleChecksum( &ppdr->checksum );
}

static void printPdrIcmp( T_PDR* pdr )
{
	T_PDR_ICMP* ppdr;

	BSL_CHECK_NULL( pdr, );
	ppdr = (T_PDR_ICMP*)pdr->pinfo;

	BSL_MSG(("%s: ---------------\n", __func__ ));
	BSL_MSG(("\tprotocolid   %d\n", pdr->protocolid ));
	BSL_MSG(("\tlength       %d\n", pdr->length ));

	BSL_MSG(("\ttype         %d(0x%X)\n", ppdr->type, ppdr->type ));
	BSL_MSG(("\tcode         %d(0x%X)\n", ppdr->code, ppdr->code ));
	printPdrTupleChecksum( &ppdr->checksum );
	BSL_MSG(("\tdata1         %d(0x%X)\n", ppdr->data1, ppdr->data1 ));
	BSL_MSG(("\tdata2         %d(0x%X)\n", ppdr->data2, ppdr->data2 ));
}

static void printPdrIgmpv2( T_PDR* pdr )
{
	T_PDR_IGMPv2* ppdr;

	BSL_CHECK_NULL( pdr, );
	ppdr = (T_PDR_IGMPv2*)pdr->pinfo;

	BSL_MSG(("%s: ---------------\n", __func__ ));
	BSL_MSG(("\tprotocolid   %d\n", pdr->protocolid ));
	BSL_MSG(("\tlength       %d\n", pdr->length ));

	BSL_MSG(("\tver          %d(0x%X)\n", ppdr->ver, ppdr->ver ));
	BSL_MSG(("\ttype         %d(0x%X)\n", ppdr->type, ppdr->type ));
	BSL_MSG(("\tmaxRespTime  %d(0x%X)\n", ppdr->maxRespTime, ppdr->maxRespTime ));
	printPdrTupleChecksum( &ppdr->checksum );
	BSL_MSG(("\tGroup Address\n"));
	printPdrTupleIp4Addr( &ppdr->group );
}

static void printPdrUDF( T_PDR* pdr )
{
	T_PDR_UDF* ppdr;

	BSL_CHECK_NULL( pdr, );
	ppdr = (T_PDR_UDF*)pdr->pinfo;

	BSL_MSG(("%s: ---------------\n", __func__ ));
	BSL_MSG(("\tprotocolid   %d\n", pdr->protocolid ));
	BSL_MSG(("\tlength       %d\n", pdr->length ));

	BSL_MSG(("\tid           %d(0x%X)\n", ppdr->id, ppdr->id ));
	BSL_MSG(("\tenable       %d(0x%X)\n", ppdr->enable, ppdr->enable ));
	BSL_MSG(("\ttype         %d(0x%X)\n", ppdr->type, ppdr->type ));
	BSL_MSG(("\tisRandom     %d(0x%X)\n", ppdr->isRandom, ppdr->isRandom ));
	BSL_MSG(("\toffset       %d(0x%X)\n", ppdr->offset, ppdr->offset ));
	BSL_MSG(("\trepeatCount  %d(0x%X)\n", ppdr->repeatCount, ppdr->repeatCount ));
	BSL_MSG(("\tmode         %d(0x%X)\n", ppdr->mode, ppdr->mode ));
	BSL_MSG(("\tstep         %d(0x%X)\n", ppdr->step, ppdr->step ));
	BSL_MSG(("\tinitval      %d(0x%X)\n", ppdr->initval, ppdr->initval ));
	BSL_MSG(("\tinitvalType  %d(0x%X)\n", ppdr->initvalType, ppdr->initvalType ));
	BSL_MSG(("\tbitmask      %d(0x%X)\n", ppdr->bitmask, ppdr->bitmask ));
}

static void assignPdrNull( T_PDR* pdr, T_Protocol* proto )							// 2016.12.20 by dgjung
{
	BSL_CHECK_NULL( pdr, );

	proto->l2.protocol = ProtocolNull;
	proto->l2.length = pdr->length;
	proto->l2.pdr = pdr->pinfo; 
}

static void assignPdrEthernet( T_PDR* pdr, T_Protocol* proto )
{
	BSL_CHECK_NULL( pdr, );

	proto->l2.protocol = ProtocolEthernet;
	proto->l2.length = pdr->length;
	proto->l2.pdr = pdr->pinfo; 
}

static void assignPdrVLAN( T_PDR* pdr, T_Protocol* proto )
{
	BSL_CHECK_NULL( pdr, );

	proto->l2tag.protocol = ProtocolVLAN;
	proto->l2tag.length = pdr->length;
	proto->l2tag.pdr = (void*)pdr->pinfo; 
}

static void assignPdrISL( T_PDR* pdr, T_Protocol* proto )
{
	BSL_CHECK_NULL( pdr, );

	proto->l2tag.protocol = ProtocolISL;
	proto->l2tag.length = pdr->length;
	proto->l2tag.pdr = (void*)pdr->pinfo; 
}

static void assignPdrMPLS( T_PDR* pdr, T_Protocol* proto )
{
	BSL_CHECK_NULL( pdr, );

	proto->l2tag.protocol = ProtocolMPLS;
	proto->l2tag.length = pdr->length;
	proto->l2tag.pdr = (void*)pdr->pinfo; 
}

static void assignPdrIp4( T_PDR* pdr, T_Protocol* proto )
{
	BSL_CHECK_NULL( pdr, );

	proto->l3.protocol = ProtocolIP4;
	proto->l3.length = pdr->length;
	proto->l3.pdr = (void*)pdr->pinfo; 
}

static void assignPdrIp6( T_PDR* pdr, T_Protocol* proto )
{
	BSL_CHECK_NULL( pdr, );

	proto->l3.protocol = ProtocolIP6;
	proto->l3.length = pdr->length;
	proto->l3.pdr = (void*)pdr->pinfo; 
}

static void assignPdrIp4OverIp6( T_PDR* pdr, T_Protocol* proto )
{
	BSL_CHECK_NULL( pdr, );

	proto->l3.protocol = ProtocolIP4OverIP6;
	proto->l3.length = pdr->length;
	proto->l3.pdr = (void*)pdr->pinfo; 
}

static void assignPdrIp6OverIp4( T_PDR* pdr, T_Protocol* proto )
{
	BSL_CHECK_NULL( pdr, );

	proto->l3.protocol = ProtocolIP6OverIP4;
	proto->l3.length = pdr->length;
	proto->l3.pdr = (void*)pdr->pinfo; 
}

static void assignPdrArp( T_PDR* pdr, T_Protocol* proto )
{
	BSL_CHECK_NULL( pdr, );

	proto->l3.protocol = ProtocolARP;
	proto->l3.length = pdr->length;
	proto->l3.pdr = (void*)pdr->pinfo; 
}

static void assignPdrTcp( T_PDR* pdr, T_Protocol* proto )
{
	BSL_CHECK_NULL( pdr, );

	proto->l4.protocol = ProtocolTCP;
	proto->l4.length = pdr->length;
	proto->l4.pdr = (void*)pdr->pinfo; 
}

static void assignPdrUdp( T_PDR* pdr, T_Protocol* proto )
{
	BSL_CHECK_NULL( pdr, );

	proto->l4.protocol = ProtocolUDP;
	proto->l4.length = pdr->length;
	proto->l4.pdr = (void*)pdr->pinfo; 
}

static void assignPdrIcmp( T_PDR* pdr, T_Protocol* proto )
{
	BSL_CHECK_NULL( pdr, );

	proto->l4.protocol = ProtocolICMP;
	proto->l4.length = pdr->length;
	proto->l4.pdr = (void*)pdr->pinfo; 
}

static void assignPdrIgmpv2( T_PDR* pdr, T_Protocol* proto )
{
	BSL_CHECK_NULL( pdr, );

	proto->l4.protocol = ProtocolIGMPv2;
	proto->l4.length = pdr->length;
	proto->l4.pdr = (void*)pdr->pinfo; 
}

#if 0
static void assignPdrUdf( T_PDR* pdr, T_Protocol* proto )
{
	BSL_CHECK_NULL( pdr, );

	//TODO:
}
#endif

void bsl_swap32( void* ptr, int length )
{
	unsigned int* swapp = (unsigned int*)ptr;
	BSL_CHECK_NULL( ptr, );

	while( length > 0 ) {
		*swapp = BSLSWAP32( *swapp );
		swapp++;
		length -= sizeof( unsigned int );
	};
}

void bsl_swap64( void* ptr, int length )
{
	unsigned long long* swapp = (unsigned long long*)ptr;
	BSL_CHECK_NULL( ptr, );

	while( length > 0 ) {
		*swapp = BSLSWAP64( *swapp );
		swapp++;
		length -= sizeof( unsigned long long );
	};
}

/***************************************************************************** 
 * Message Id 111 : Direct Registor I/O for GUI
 *****************************************************************************/
EnumResultCode 
bsl_handle_msgid_111(
		void* msgdata, 
		unsigned int msglen, 
		void** msgreply, 
		unsigned int* msgreplylen )
{
	T_MSGIF_111_REQ_UNIT* reqpunit = NULL;
	T_MSGIF_111_RESP_UNIT* respunit = NULL;
	T_MSGIF_111_RESP* replyp = NULL;
	T_MSGIF_HDR* hdr = NULL;
	const static int nport = 1;
	EnumResultCode retval = 0;
	int nrecord = 0;

	BSL_MSG(("%s enter\n", __func__));

	nrecord = MSGIF_GET_NRECORD( msgdata );

	replyp = malloc( sizeof( T_MSGIF_HDR ) + nrecord*sizeof( T_MSGIF_111_RESP_UNIT ) );
	BSL_CHECK_NULL( replyp, ResultMsgIfMsgHandleError );

	reqpunit = (T_MSGIF_111_REQ_UNIT*)( msgdata + sizeof( T_MSGIF_HDR ) );
	respunit = (T_MSGIF_111_RESP_UNIT*)(replyp+1);

	*msgreply = replyp;
	*msgreplylen = nrecord*sizeof( T_MSGIF_111_RESP_UNIT ) + sizeof(T_MSGIF_HDR);

	hdr = (T_MSGIF_HDR*)replyp;
	memcpy( hdr, msgdata, sizeof( T_MSGIF_HDR ) );
	MSGIF_SET_TYPE( hdr, MSGIF_TYPE_REPLY );
	MSGIF_SET_LENGTH( hdr, *msgreplylen-sizeof(T_MSGIF_HDR) );
	MSGIF_SET_NRECORD( hdr, nrecord );

	unsigned long long value = ntohll(reqpunit->value);
	do {
		//Register Read/Write
		retval = bsl_setRegister( 
				ntohl(reqpunit->cardid), 
				ntohl(reqpunit->command),
				ntohl(reqpunit->addr),
				&value
				); 

		respunit->result = htonl(retval);
		respunit->value = htonll(value);

		reqpunit++;
		respunit++;
	} while( --nrecord > 0 );

	free( msgdata );

	BSL_MSG(("%s: reply messages ---------\n", __func__ ));
	print_msg_hdr( replyp );
	respunit = (T_MSGIF_111_RESP_UNIT*)(replyp+1);
	do {
		BSL_MSG(("[%d] result %d value %llX\n", nrecord, ntohl(respunit->result), ntohll(respunit->value)));
		respunit++;
	} while( --nrecord > 0 );

	return ResultSuccess;
}

/***************************************************************************** 
 * Message Id 112 : Shell script execution
 *****************************************************************************/
#define SIZE_MAX_STR_SHELL 256
EnumResultCode 
bsl_handle_msgid_112(
		void* msgdata, 
		unsigned int msglen, 
		void** msgreply, 
		unsigned int* msgreplylen )
{
	T_MSGIF_112_REQ_UNIT* reqpunit = NULL;
	T_MSGIF_112_RESP_UNIT* respunit = NULL;
	T_MSGIF_112_RESP* replyp = NULL;
	T_MSGIF_HDR* hdr = NULL;
	const static int nport = 1;
	EnumResultCode retval = 0;
	int nrecord = 0;
	char strshell[SIZE_MAX_STR_SHELL] = {0,};

	BSL_MSG(("%s enter\n", __func__));

	nrecord = MSGIF_GET_NRECORD( msgdata );

	replyp = malloc( sizeof( T_MSGIF_HDR ) + nrecord*sizeof( T_MSGIF_112_RESP_UNIT ) );
	BSL_CHECK_NULL( replyp, ResultMsgIfMsgHandleError );

	reqpunit = (T_MSGIF_112_REQ_UNIT*)( msgdata + sizeof( T_MSGIF_HDR ) );
	respunit = (T_MSGIF_112_RESP_UNIT*)(replyp+1);

	*msgreply = replyp;
	*msgreplylen = nrecord*sizeof( T_MSGIF_112_RESP_UNIT ) + sizeof(T_MSGIF_HDR);

	hdr = (T_MSGIF_HDR*)replyp;
	memcpy( hdr, msgdata, sizeof( T_MSGIF_HDR ) );
	MSGIF_SET_TYPE( hdr, MSGIF_TYPE_REPLY );
	MSGIF_SET_LENGTH( hdr, *msgreplylen-sizeof(T_MSGIF_HDR) );
	MSGIF_SET_NRECORD( hdr, nrecord );

	reqpunit->length = ntohl( reqpunit->length );
	if( reqpunit->length > SIZE_MAX_STR_SHELL ) {
		BSL_MSG(("%s: oversize length %d ---------\n", __func__, reqpunit->length ));
		respunit->result = ResultOutOfRange;
		goto ret_resp;
	}

	memcpy( strshell, reqpunit->strshell, reqpunit->length );
	BSL_MSG(("%s: \"%s\" to be executed....\n", __func__, strshell ));

	system(strshell);
	respunit->result = ResultSuccess;

ret_resp:

	respunit->result = htonl( respunit->result ); 
	free( msgdata );

	BSL_MSG(("%s: reply messages ---------\n", __func__ ));
	print_msg_hdr( replyp );
	respunit = (T_MSGIF_112_RESP_UNIT*)(replyp+1);
	BSL_MSG(("[%d] result %d\n", nrecord, ntohl(respunit->result) ));

	return ResultSuccess;
}

