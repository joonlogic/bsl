#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h> 
#include <arpa/inet.h> 
#include <netinet/ether.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "bsl_type.h"
#include "bsl_system.h"
#include "bsl_dbg.h"
#include "bsl_api.h"
#include "bsl_msgif.h"
#include "bsl_proto.h"

const static char* this_version = "20131230";
extern char* bsl_getErrorStr( EnumResultCode );
int gcardid=0;
int gportid=0;
int gstreamid=0;
int gversion=0;
int genable=0;
char gdocname[128] = {0,};

extern int bsl_socket_init( unsigned int addr, 
		unsigned short port, struct sockaddr_in* server );
extern void* bsl_read_msg( int sd, int* msglen );
extern EnumResultCode bsl_socket_connect( int socket, struct sockaddr_in* server );
extern EnumResultCode bsl_socket_shutdown( int socket );
extern void print_msgid_101( T_MSGIF_101_RESP* resp );
extern void print_msgid_103( T_MSGIF_103_RESP* resp );
extern void print_msgid_setcmd_resp( T_MSGIF_SETCMD_RESP* replyp );
extern void bsl_swap32( void* ptr, unsigned int length );

void str2hex( int size, char* str, unsigned char* outhexval );

typedef void (*bsl_app_handle_msg_f_ptr_type) ( void );
void bsl_app_handle_msgid_101( void );
void bsl_app_handle_msgid_102( void );
void bsl_app_handle_msgid_103( void );
void bsl_app_handle_msgid_104( void );
void bsl_app_handle_msgid_105( void );
void bsl_app_handle_msgid_105_2( void );
void bsl_app_handle_msgid_106( void );
void bsl_app_handle_msgid_108( void );
void bsl_app_handle_msgid_110( void );

static bsl_app_handle_msg_f_ptr_type
	bsl_app_handle_msg_f_ptr[SIZE_MAX_MSGIF_ID] = 
{
	[0] = NULL,
	[1] = (bsl_app_handle_msg_f_ptr_type)&bsl_app_handle_msgid_101, //101
	[2] = (bsl_app_handle_msg_f_ptr_type)&bsl_app_handle_msgid_102, //102
	[3] = (bsl_app_handle_msg_f_ptr_type)&bsl_app_handle_msgid_103, //103
	[4] = (bsl_app_handle_msg_f_ptr_type)&bsl_app_handle_msgid_104, //104
//	[5] = (bsl_app_handle_msg_f_ptr_type)&bsl_app_handle_msgid_105_2, //105 //for test simultaneous 2 ports
	[5] = (bsl_app_handle_msg_f_ptr_type)&bsl_app_handle_msgid_105, //105 
	[6] = (bsl_app_handle_msg_f_ptr_type)&bsl_app_handle_msgid_106, //106 
	[8] = (bsl_app_handle_msg_f_ptr_type)&bsl_app_handle_msgid_108, //108
	[10] = (bsl_app_handle_msg_f_ptr_type)&bsl_app_handle_msgid_110, //110
};


typedef void (*bsl_app_xml_pdr_f_ptr_type) (
		xmlNodePtr node, T_PDR* pdrp );

static void 
	xmlPdrEthernet( xmlNodePtr node, T_PDR* pdrp ),
	xmlPdrVLAN( xmlNodePtr node, T_PDR* pdrp ),
	xmlPdrISL( xmlNodePtr node, T_PDR* pdrp ),
	xmlPdrIp4( xmlNodePtr node, T_PDR* pdrp ),
	xmlPdrIp6( xmlNodePtr node, T_PDR* pdrp ),
	xmlPdrIp4OverIp6( xmlNodePtr node, T_PDR* pdrp ),
	xmlPdrIp6OverIp4( xmlNodePtr node, T_PDR* pdrp ),
	xmlPdrArp( xmlNodePtr node, T_PDR* pdrp ),
	xmlPdrTcp( xmlNodePtr node, T_PDR* pdrp ),
	xmlPdrUdp( xmlNodePtr node, T_PDR* pdrp ),
	xmlPdrIcmp( xmlNodePtr node, T_PDR* pdrp ),
	xmlPdrIgmpv2( xmlNodePtr node, T_PDR* pdrp );

static bsl_app_xml_pdr_f_ptr_type xml_pdr_f_ptr[SIZE_MAX_PROTOCOL_ID] = 
{
	[0] = NULL,
	[21] = (bsl_app_xml_pdr_f_ptr_type)&xmlPdrEthernet,
	[22] = (bsl_app_xml_pdr_f_ptr_type)&xmlPdrVLAN,
	[23] = (bsl_app_xml_pdr_f_ptr_type)&xmlPdrISL,
//  [24] = (bsl_app_xml_pdr_f_ptr_type)&xmlPdrMPLS, 
	[31] = (bsl_app_xml_pdr_f_ptr_type)&xmlPdrIp4,
	[32] = (bsl_app_xml_pdr_f_ptr_type)&xmlPdrIp6,
	[33] = (bsl_app_xml_pdr_f_ptr_type)&xmlPdrIp4OverIp6,
	[34] = (bsl_app_xml_pdr_f_ptr_type)&xmlPdrIp6OverIp4,
	[35] = (bsl_app_xml_pdr_f_ptr_type)&xmlPdrArp,
	[41] = (bsl_app_xml_pdr_f_ptr_type)&xmlPdrTcp,
	[42] = (bsl_app_xml_pdr_f_ptr_type)&xmlPdrUdp,
	[43] = (bsl_app_xml_pdr_f_ptr_type)&xmlPdrIcmp,
	[44] = (bsl_app_xml_pdr_f_ptr_type)&xmlPdrIgmpv2,
//	[51] = (bsl_app_xml_pdr_f_ptr_type)&xmlPdrUDF,
};

typedef void (*print_T_PDR_f_ptr_type) ( T_PDR* pdr );
extern print_T_PDR_f_ptr_type print_T_PDR_f_ptr[SIZE_MAX_PROTOCOL_ID];

static EnumResultCode bsl_parseXml( T_MSGIF_110_REQ_UNIT* reqp, int* datalen );
static EnumResultCode bsl_parseSearchXml( T_MSGIF_110_REQ_UNIT* reqp, xmlDocPtr doc, int* );

void help(char *progname)
{
	fprintf( stderr, "%s BSL message socket client program. Ver %s\n", 
			progname, this_version );
	fprintf( stderr, "      -h : Help\n");
	fprintf( stderr, "      -v : Version\n");
	fprintf( stderr, "      -m msgid    : request msg id\n");
	fprintf( stderr, "      -c cardid   : request card id\n");
	fprintf( stderr, "      -p portid   : request port id\n");
	fprintf( stderr, "      -s streamid : request stream id\n");
	fprintf( stderr, "      -e enable   : enable(1), disable(0)\n");
	fprintf( stderr, "      -f xmlfile  : script xml filename\n");
}

int main(int argc, char *argv[])
{
	int opt, opt_ok = 0;
	int msgid  = 0;

	opt_ok = 0;
	while ((opt = getopt(argc, argv, "hm:c:p:s:e:f:v")) != -1) {
		switch (opt) {
			case 'h':
				break;
			case 'm':
				opt_ok = 1;
				msgid=strtol( optarg, NULL, 10 );
				break;
			case 'c':
				opt_ok = 1;
				gcardid=strtol( optarg, NULL, 10 );
				break;
			case 'p':
				opt_ok = 1;
				gportid=strtol( optarg, NULL, 10 );
				break;
			case 's':
				opt_ok = 1;
				gstreamid=strtol( optarg, NULL, 10 );
				break;
			case 'e':
				opt_ok = 1;
				genable=strtol( optarg, NULL, 10 );
				break;
			case 'f':
				opt_ok = 1;
				strcpy( gdocname, optarg );
				break;
			case 'v':
				opt_ok = 1;
				gversion = 1;
				break;
		}
	}

	if (opt_ok != 1) {
		help(argv[0]);
		exit(0);
	}

	if( gversion == 1 ) {
		T_SystemVersion ver;
		bsl_getVersionInfo( gcardid, &ver );
		printf("board = %s\n", ver.board );
		printf("fpga = %s\n", ver.fpga );
		printf("driver = %s\n", ver.driver );
		printf("api = %s\n", ver.api );
		printf("gui = %s\n", ver.gui );
		exit(0);
	}

	if( ( msgid <= VALUE_MSGID_OFFSET ) || ( msgid > VALUE_MAX_MSGIF_ID ) ) {
		fprintf( stderr, "%s: msgid out of range error %d ( 101 ~ 200 )\n",
				__func__, msgid );
		help(argv[0]);
		return 0;
	}

	msgid -= VALUE_MSGID_OFFSET;

	if( bsl_app_handle_msg_f_ptr[msgid] == NULL ) {
		fprintf( stderr, "%s: Not Yet Implemented!!\n", __func__ );
		return 0;
	}

	bsl_app_handle_msg_f_ptr[msgid]();

	return 0;
}

void bsl_app_make_msghdr( T_MSGIF_HDR* hdr )
{
	MSGIF_SET_DELIM( hdr );
	MSGIF_SET_TYPE( hdr, MSGIF_TYPE_REQUEST );
	MSGIF_SET_LENGTH( hdr, 0 );
	MSGIF_SET_NRECORD( hdr, 0 );
}

int bsl_app_open_client_socket( void )
{
	struct sockaddr_in server;
	static const char* socketip = "0.0.0.0";
//	static const char* socketip = "127.0.0.1";
//	static const char* socketip = "192.168.0.202";
	static const unsigned short socketport = TCP_PORT_LISTEN;
	int socketfd = 0;
	int timeout_cnt = 10;
	EnumResultCode ret;

	memset( &server, 0, sizeof( server ) );
	socketfd = bsl_socket_init( inet_addr(socketip), socketport, &server );
	if( socketfd < 0 ) {
		fprintf( stderr, "%s: socket open error socketfd %d\n", 
				__func__, socketfd );
		return socketfd;
	}

	do {
		ret = bsl_socket_connect( socketfd, &server );
		if( ret == ResultSuccess ) break;
		else usleep(100);

		if( timeout_cnt-- < 0 ) {
			fprintf( stderr, "%s: socket connect error socketfd %d\n", 
				__func__, socketfd );
			return -1;
		}
	} while(1);

	return socketfd;
}


void 
bsl_app_handle_msgid_101( void )
{
	int socketfd = 0;
	T_MSGIF_HDR* reqp = NULL; 
	T_MSGIF_101_REQ_UNIT* requnitp = NULL; 
	T_MSGIF_101_RESP* resp = NULL;
	int msglen = 0;

	reqp = malloc( sizeof(T_MSGIF_HDR) + sizeof(T_MSGIF_101_REQ_UNIT) );
	BSL_CHECK_NULL( reqp, );
	requnitp = (T_MSGIF_101_REQ_UNIT*)(reqp+1);

	socketfd = bsl_app_open_client_socket();
	if( socketfd < 0 ) {
		fprintf( stderr, "%s: open client socket failure. socketfd %d\n", 
				__func__, socketfd );
		return;
	}

	bsl_app_make_msghdr( reqp );
	MSGIF_SET_ID( reqp, 101 );
	MSGIF_SET_LENGTH( reqp, 4 );
	MSGIF_SET_NRECORD( reqp, 1 );
	requnitp->cardid = htonl( gcardid );

	if( write( socketfd, reqp, sizeof( T_MSGIF_HDR )+sizeof(T_MSGIF_101_REQ_UNIT) ) <= 0 )
	{
		fprintf( stderr, "%s: write failure. socketfd %d\n", 
				__func__, socketfd );
		free( reqp );
		bsl_socket_shutdown( socketfd );
		return;
	}

	fprintf( stderr, "%s: Sent Message Request\n", __func__ );

	resp = bsl_read_msg( socketfd, &msglen );

	if ( resp ) {
		fprintf( stderr, "[Client]%s: Received Messages =======\n", __func__ );
		print_msgid_101( resp );

		free( reqp );
		free( resp );
	}
	else
		fprintf( stderr, "[Client]%s: No Response  =======\n", __func__ );

}


void 
bsl_app_handle_msgid_102( void )
{
	int socketfd = 0;
	T_MSGIF_102_REQ* reqp = NULL; 
	T_MSGIF_102_REQ_UNIT* requnit = NULL; 
	T_MSGIF_102_RESP* resp = NULL;
	int msglen = 0;

	reqp = malloc( sizeof(T_MSGIF_102_REQ) + sizeof(T_MSGIF_102_REQ_UNIT) );
	BSL_CHECK_NULL( reqp, );
	memset( reqp, sizeof(T_MSGIF_102_REQ) + sizeof(T_MSGIF_102_REQ_UNIT), 0x00 );
	requnit = (T_MSGIF_102_REQ_UNIT*)( reqp + 1 );

	socketfd = bsl_app_open_client_socket();
	if( socketfd <= 0 ) {
		fprintf( stderr, "%s: open client socket failure. socketfd %d\n", 
				__func__, socketfd );
		return;
	}

	bsl_app_make_msghdr( (T_MSGIF_HDR*)reqp );
	MSGIF_SET_ID( reqp, 102 );
	MSGIF_SET_NRECORD( reqp, 1 );
	MSGIF_SET_LENGTH( reqp, sizeof( *reqp ) + sizeof( *requnit ) - sizeof( T_MSGIF_HDR ) );
	requnit->cardid = htonl(gcardid);
	requnit->portid = htonl(gportid);
//	requnit->portmode = htonl(0); //normal
	requnit->portmode = htonl(genable); //normal or interleave

	if( write( socketfd, reqp, sizeof( T_MSGIF_102_REQ )+sizeof(T_MSGIF_102_REQ_UNIT) ) < 0 )
	{
		fprintf( stderr, "%s: write failure. socketfd %d\n", 
				__func__, socketfd );
		free( reqp );
		bsl_socket_shutdown( socketfd );
		return;
	}

	fprintf( stderr, "%s: Sent Message Request\n", __func__ );

	resp = bsl_read_msg( socketfd, &msglen );

	fprintf( stderr, "[Client]%s: Received Messages =======\n", __func__ );
	print_msgid_setcmd_resp( resp );

	free( reqp );
	free( resp );
}

void 
bsl_app_handle_msgid_103( void )
{	
	int socketfd = 0;
	T_MSGIF_103_REQ* reqp = NULL; 
	T_MSGIF_103_REQ_UNIT* requnit = NULL; 
	T_MSGIF_103_RESP* resp = NULL;
	int msglen = 0;

	reqp = malloc( sizeof(T_MSGIF_103_REQ) + sizeof(T_MSGIF_103_REQ_UNIT) );
	BSL_CHECK_NULL( reqp, );
	memset( reqp, sizeof(T_MSGIF_103_REQ) + sizeof(T_MSGIF_103_REQ_UNIT), 0x00 );
	requnit = (T_MSGIF_103_REQ_UNIT*)( reqp + 1 );

	socketfd = bsl_app_open_client_socket();
	if( socketfd <= 0 ) {
		fprintf( stderr, "%s: open client socket failure. socketfd %d\n", 
				__func__, socketfd );
		return;
	}

	bsl_app_make_msghdr( (T_MSGIF_HDR*)reqp );
	MSGIF_SET_ID( reqp, 103 );
	MSGIF_SET_NRECORD( reqp, 1 );
	MSGIF_SET_LENGTH( reqp, sizeof( T_MSGIF_103_REQ_UNIT ) );
	requnit->cardid = htonl(gcardid);
	requnit->portid = htonl(gportid);

	if( write( socketfd, reqp, sizeof(T_MSGIF_103_REQ)+sizeof(T_MSGIF_103_REQ_UNIT) ) < 0 )
	{
		fprintf( stderr, "%s: write failure. socketfd %d\n", 
				__func__, socketfd );
		free( reqp );
		bsl_socket_shutdown( socketfd );
		return;
	}

	fprintf( stderr, "%s: Sent Message Request\n", __func__ );

	resp = bsl_read_msg( socketfd, &msglen );

	fprintf( stderr, "[Client]%s: Received Messages =======\n", __func__ );
	print_msgid_103( resp );

	free( reqp );
	free( resp );
}

void 
bsl_app_handle_msgid_104( void )
{	
	int socketfd = 0;
	T_MSGIF_104_REQ* reqp = NULL; 
	T_MSGIF_104_REQ_UNIT* requnit = NULL; 
	T_MSGIF_104_RESP* resp = NULL;
	int msglen = 0;

	reqp = malloc( sizeof(T_MSGIF_104_REQ) + sizeof(T_MSGIF_104_REQ_UNIT) );
	BSL_CHECK_NULL( reqp, );
	memset( reqp, sizeof(T_MSGIF_104_REQ) + sizeof(T_MSGIF_104_REQ_UNIT), 0x00 );
	requnit = (T_MSGIF_104_REQ_UNIT*)( reqp + 1 );

	socketfd = bsl_app_open_client_socket();
	if( socketfd <= 0 ) {
		fprintf( stderr, "%s: open client socket failure. socketfd %d\n", 
				__func__, socketfd );
		return;
	}

	bsl_app_make_msghdr( (T_MSGIF_HDR*)reqp );
	MSGIF_SET_ID( reqp, 104 );
	MSGIF_SET_NRECORD( reqp, 1 );
	MSGIF_SET_LENGTH( reqp, sizeof( T_MSGIF_104_REQ_UNIT ) );
	requnit->cardid = htonl(gcardid);
	requnit->portid = htonl(gportid);
	requnit->enable = htonl(genable);

	if( write( socketfd, reqp, sizeof(T_MSGIF_104_REQ)+sizeof(T_MSGIF_104_REQ_UNIT) ) < 0 )
	{
		fprintf( stderr, "%s: write failure. socketfd %d\n", 
				__func__, socketfd );
		free( reqp );
		bsl_socket_shutdown( socketfd );
		return;
	}

	fprintf( stderr, "%s: Sent Message Request\n", __func__ );

	resp = bsl_read_msg( socketfd, &msglen );

	fprintf( stderr, "[Client]%s: Received Messages =======\n", __func__ );
	print_msgid_setcmd_resp( resp );

	free( reqp );
	free( resp );
}

void 
bsl_app_handle_msgid_105_2( void )
{	
	int socketfd = 0;
	T_MSGIF_105_REQ* reqp = NULL; 
	T_MSGIF_105_REQ_UNIT* requnit = NULL; 
	T_MSGIF_105_RESP* resp = NULL;
	int msglen = 0;

	reqp = malloc( sizeof(T_MSGIF_105_REQ) + 2*sizeof(T_MSGIF_105_REQ_UNIT) );
	BSL_CHECK_NULL( reqp, );
	memset( reqp, sizeof(T_MSGIF_105_REQ) + 2*sizeof(T_MSGIF_105_REQ_UNIT), 0x00 );
	requnit = (T_MSGIF_105_REQ_UNIT*)( reqp + 1 );

	socketfd = bsl_app_open_client_socket();
	if( socketfd <= 0 ) {
		fprintf( stderr, "%s: open client socket failure. socketfd %d\n", 
				__func__, socketfd );
		return;
	}

	bsl_app_make_msghdr( (T_MSGIF_HDR*)reqp );
	MSGIF_SET_ID( reqp, 105 );
	MSGIF_SET_NRECORD( reqp, 2 );
	MSGIF_SET_LENGTH( reqp, 2*sizeof( T_MSGIF_105_REQ_UNIT ) );
	requnit->cardid = htonl(gcardid);
	requnit->portid = htonl(0);
	requnit->mode = 0;
	//requnit->size = htonl(0x10000000); //256M
	requnit->size = htonl(0x40000000); //1G
	requnit->start = htonl(genable);  //start or stop
	requnit++;
	requnit->cardid = htonl(gcardid);
	requnit->portid = htonl(1);
	requnit->mode = 0;
	//requnit->size = htonl(0x10000000); //256M
	requnit->size = htonl(0x40000000); //1G
	requnit->start = htonl(genable);  //start or stop


	if( write( socketfd, reqp, sizeof(T_MSGIF_105_REQ)+3*sizeof(T_MSGIF_105_REQ_UNIT) ) < 0 )
	{
		fprintf( stderr, "%s: write failure. socketfd %d\n", 
				__func__, socketfd );
		free( reqp );
		bsl_socket_shutdown( socketfd );
		return;
	}

	fprintf( stderr, "%s: Sent Message Request\n", __func__ );

	resp = bsl_read_msg( socketfd, &msglen );

	fprintf( stderr, "[Client]%s: Received Messages =======\n", __func__ );
	print_msgid_setcmd_resp( resp );

	free( reqp );
	free( resp );
}

void 
bsl_app_handle_msgid_105( void )
{	
	int socketfd = 0;
	T_MSGIF_105_REQ* reqp = NULL; 
	T_MSGIF_105_REQ_UNIT* requnit = NULL; 
	T_MSGIF_105_RESP* resp = NULL;
	int msglen = 0;

	reqp = malloc( sizeof(T_MSGIF_105_REQ) + sizeof(T_MSGIF_105_REQ_UNIT) );
	BSL_CHECK_NULL( reqp, );
	memset( reqp, sizeof(T_MSGIF_105_REQ) + sizeof(T_MSGIF_105_REQ_UNIT), 0x00 );
	requnit = (T_MSGIF_105_REQ_UNIT*)( reqp + 1 );

	socketfd = bsl_app_open_client_socket();
	if( socketfd <= 0 ) {
		fprintf( stderr, "%s: open client socket failure. socketfd %d\n", 
				__func__, socketfd );
		return;
	}

	bsl_app_make_msghdr( (T_MSGIF_HDR*)reqp );
	MSGIF_SET_ID( reqp, 105 );
	MSGIF_SET_NRECORD( reqp, 1 );
	MSGIF_SET_LENGTH( reqp, sizeof( T_MSGIF_105_REQ_UNIT ) );
	requnit->cardid = htonl(gcardid);
	requnit->portid = htonl(gportid);
	requnit->mode = 0;
	requnit->size = htonl(0x10000000); //256M
	requnit->start = htonl(genable);  //start or stop

	if( write( socketfd, reqp, sizeof(T_MSGIF_105_REQ)+sizeof(T_MSGIF_105_REQ_UNIT) ) < 0 )
	{
		fprintf( stderr, "%s: write failure. socketfd %d\n", 
				__func__, socketfd );
		free( reqp );
		bsl_socket_shutdown( socketfd );
		return;
	}

	fprintf( stderr, "%s: Sent Message Request\n", __func__ );

	resp = bsl_read_msg( socketfd, &msglen );

	fprintf( stderr, "[Client]%s: Received Messages =======\n", __func__ );
	print_msgid_setcmd_resp( resp );

	free( reqp );
	free( resp );
}

void 
bsl_app_handle_msgid_106( void )
{	
	int socketfd = 0;
	T_MSGIF_106_REQ* reqp = NULL; 
	T_MSGIF_106_REQ_UNIT* requnit = NULL; 
	T_MSGIF_106_RESP* resp = NULL;
	int msglen = 0;

	reqp = malloc( sizeof(T_MSGIF_106_REQ) + sizeof(T_MSGIF_106_REQ_UNIT) );
	BSL_CHECK_NULL( reqp, );
	memset( reqp, sizeof(T_MSGIF_106_REQ) + sizeof(T_MSGIF_106_REQ_UNIT), 0x00 );
	requnit = (T_MSGIF_106_REQ_UNIT*)( reqp + 1 );

	socketfd = bsl_app_open_client_socket();
	if( socketfd <= 0 ) {
		fprintf( stderr, "%s: open client socket failure. socketfd %d\n", 
				__func__, socketfd );
		return;
	}

	bsl_app_make_msghdr( (T_MSGIF_HDR*)reqp );
	MSGIF_SET_ID( reqp, 106 );
	MSGIF_SET_NRECORD( reqp, 1 );
	MSGIF_SET_LENGTH( reqp, sizeof( T_MSGIF_106_REQ_UNIT ) );
	requnit->cardid = htonl(gcardid);
	requnit->portid = htonl(gportid);
	requnit->enable = htonl(1);

	if( write( socketfd, reqp, sizeof(T_MSGIF_106_REQ)+sizeof(T_MSGIF_106_REQ_UNIT) ) < 0 )
	{
		fprintf( stderr, "%s: write failure. socketfd %d\n", 
				__func__, socketfd );
		free( reqp );
		bsl_socket_shutdown( socketfd );
		return;
	}

	fprintf( stderr, "%s: Sent Message Request\n", __func__ );

	resp = bsl_read_msg( socketfd, &msglen );

	fprintf( stderr, "[Client]%s: Received Messages =======\n", __func__ );
	print_msgid_setcmd_resp( resp );

	free( reqp );
	free( resp );
}



void 
bsl_app_handle_msgid_108( void )
{	
	int socketfd = 0;
	T_MSGIF_108_REQ* reqp = NULL; 
	T_MSGIF_108_REQ_UNIT* requnit = NULL; 
	T_MSGIF_108_RESP* resp = NULL;
	int msglen = 0;

	reqp = malloc( sizeof(T_MSGIF_108_REQ) + sizeof(T_MSGIF_108_REQ_UNIT) );
	BSL_CHECK_NULL( reqp, );
	memset( reqp, sizeof(T_MSGIF_108_REQ) + sizeof(T_MSGIF_108_REQ_UNIT), 0x00 );
	requnit = (T_MSGIF_108_REQ_UNIT*)( reqp + 1 );

	socketfd = bsl_app_open_client_socket();
	if( socketfd <= 0 ) {
		fprintf( stderr, "%s: open client socket failure. socketfd %d\n", 
				__func__, socketfd );
		return;
	}

	bsl_app_make_msghdr( (T_MSGIF_HDR*)reqp );
	MSGIF_SET_ID( reqp, 108 );
	MSGIF_SET_NRECORD( reqp, 1 );
	MSGIF_SET_LENGTH( reqp, sizeof( T_MSGIF_108_REQ_UNIT ) );
	requnit->cardid = htonl(gcardid);
	requnit->portid = htonl(gportid);
	requnit->streamid = 0;
	requnit->command = 0;
	requnit->timesec = 0x112233445566ll;

	if( write( socketfd, reqp, sizeof(T_MSGIF_108_REQ)+sizeof(T_MSGIF_108_REQ_UNIT) ) < 0 )
	{
		fprintf( stderr, "%s: write failure. socketfd %d\n", 
				__func__, socketfd );
		free( reqp );
		bsl_socket_shutdown( socketfd );
		return;
	}

	fprintf( stderr, "%s: Sent Message Request\n", __func__ );

	resp = bsl_read_msg( socketfd, &msglen );

	fprintf( stderr, "[Client]%s: Received Messages =======\n", __func__ );
	print_msgid_setcmd_resp( resp );

	free( reqp );
	free( resp );
}


void 
bsl_app_handle_msgid_110( void )
{	
	int socketfd = 0;
	T_MSGIF_110_REQ* reqp = NULL; 
	T_MSGIF_110_REQ_UNIT* requnit = NULL; 
	T_MSGIF_110_RESP* resp = NULL;
	int msglen = 0;
	EnumResultCode ret; 
	static const int SIZE_RESERVED_MEM_FOR_PDR = 2048;

	reqp = malloc( sizeof(T_MSGIF_110_REQ) + sizeof(T_MSGIF_110_REQ_UNIT) + 
			SIZE_RESERVED_MEM_FOR_PDR );
	BSL_CHECK_NULL( reqp, );
	memset( reqp, sizeof(T_MSGIF_110_REQ) + sizeof(T_MSGIF_110_REQ_UNIT) + 
			SIZE_RESERVED_MEM_FOR_PDR, 0x00 );
	requnit = (T_MSGIF_110_REQ_UNIT*)( reqp + 1 );

	ret = bsl_parseXml( requnit, &msglen );
	BSL_CHECK_RESULT( ret, );

	socketfd = bsl_app_open_client_socket();
	if( socketfd <= 0 ) {
		fprintf( stderr, "%s: open client socket failure. socketfd %d\n", 
				__func__, socketfd );
		return;
	}

	bsl_app_make_msghdr( (T_MSGIF_HDR*)reqp );
	MSGIF_SET_ID( reqp, 110 );
	MSGIF_SET_NRECORD( reqp, 1 );
	MSGIF_SET_LENGTH( reqp, msglen );

	bsl_swap32( requnit, msglen );

	//for message print after swap 
	do {
		unsigned char* ptr = (unsigned char*)requnit;
		int i;
		printf("Messages after swap (msglen %d) ----------------------- \n", msglen);
		for( i=0; i<msglen; i++ ) {
			if( i%16 == 0 ) printf("\n[%04d] ", i );
			printf("%02X", *(ptr+i) );
			if( i%4 == 3 ) printf(" ");
		}
		printf("\n");
	} while(0);

	if( write( socketfd, reqp, sizeof(T_MSGIF_110_REQ)+msglen ) < 0 )
	{
		fprintf( stderr, "%s: write failure. socketfd %d\n", 
				__func__, socketfd );
		free( reqp );
		bsl_socket_shutdown( socketfd );
		return;
	}

	fprintf( stderr, "%s: Sent Message Request\n", __func__ );

	resp = bsl_read_msg( socketfd, &msglen );

	fprintf( stderr, "[Client]%s: Received Messages =======\n", __func__ );
	print_msgid_setcmd_resp( resp );

	free( reqp );
	free( resp );
}

EnumResultCode bsl_parseXml( T_MSGIF_110_REQ_UNIT* reqp, int* datalen )
{
	EnumResultCode ret;
	xmlDocPtr doc;

	doc = xmlParseFile( gdocname );
	BSL_CHECK_NULL( doc, ResultNullArg );

	ret = bsl_parseSearchXml( reqp, doc, datalen );
	if( ret != ResultSuccess ) xmlFreeDoc( doc );

	BSL_CHECK_RESULT( ret, ret );

	return ret;
}

#define BSL_XML_IS_NODE_NAME(node, str) (!xmlStrcmp(node->name, (const xmlChar *)str))
#define BSL_XML_NEXT_NODE(node)         (node->next)
#define BSL_XML_FIND_NODE(node)         \
	do { \
		if( node->type == XML_ELEMENT_NODE ) break; \
		node = node->next; \
	} while( node ); \
    
static char * bsl_getNodeText(xmlNodePtr pNode)
{
	    BSL_CHECK_NULL( pNode->children, NULL );
		    return (char *)pNode->children->content;
}

EnumResultCode bsl_parseSearchXml( T_MSGIF_110_REQ_UNIT* reqp, xmlDocPtr doc, int* datalen )
{
#if 0//XXXXXXXXXXXXXXXXXXXXXXX
	xmlNodePtr pRoot, nodeL1, nodeL2;

	//1. check messageSet
	pRoot = xmlDocGetRootElement( doc );
	BSL_CHECK_NULL( pRoot, ResultNullArg );

	BSL_CHECK_TRUE( \
			xmlStrcmp( pRoot->name, (xmlChar*)"messageSet" ) == 0, \
			ResultGeneralError );

	//2. check common
	nodeL1 = pRoot->xmlChildrenNode;
	BSL_CHECK_NULL( nodeL1, ResultGeneralError );
	BSL_XML_FIND_NODE( nodeL1 );
	BSL_CHECK_NULL( nodeL1, ResultGeneralError );

	BSL_CHECK_TRUE( \
		xmlStrcmp( nodeL1->name, (xmlChar*)"header" ) == 0, ResultGeneralError );

	//2.1 check msgid
	nodeL2 = nodeL1->xmlChildrenNode;
	BSL_CHECK_NULL( nodeL2, ResultGeneralError );
	BSL_XML_FIND_NODE( nodeL2 );
	BSL_CHECK_NULL( nodeL2, ResultGeneralError );

	if( BSL_XML_IS_NODE_NAME( nodeL2, "msgid" ) ) {
		BSL_CHECK_TRUE( !strcmp( bsl_getNodeText( nodeL2 ), "110" ), ResultGeneralError );
	}
	else {
		printf("node name is %s\n", nodeL2->name );
		BSL_CHECK_TRUE( 0, ResultGeneralError );
	}

	//2.2 common elements
	for( nodeL2=nodeL2; nodeL2 != NULL; nodeL2 = nodeL2->next ) {
		if( nodeL2->type != XML_ELEMENT_NODE ) continue;
		if( BSL_XML_IS_NODE_NAME( nodeL2, "cardid" ) ) 
			reqp->cardid = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "portid" ) ) 
			reqp->portid = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "streamid" ) ) 
			reqp->streamid = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "enable" ) ) 
			reqp->enable = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
	}

	//3. streamControlTuple
	nodeL1 = BSL_XML_NEXT_NODE( nodeL1 );
	BSL_CHECK_NULL( nodeL1, ResultGeneralError );
	BSL_XML_FIND_NODE( nodeL1 );
	BSL_CHECK_NULL( nodeL1, ResultGeneralError );

	BSL_CHECK_TRUE( \
		xmlStrcmp( nodeL1->name, (xmlChar*)"streamControlTuple" ) == 0,\
		ResultGeneralError );

	for( nodeL2 = nodeL1->xmlChildrenNode; nodeL2 != NULL; nodeL2 = nodeL2->next ) {
		if( nodeL2->type != XML_ELEMENT_NODE ) continue;
		if( BSL_XML_IS_NODE_NAME( nodeL2, "streamControlSpec" ) ) 
			reqp->control.control = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "returnToId" ) ) 
			reqp->control.returnToId = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "loopCount" ) ) 
			reqp->control.loopCount = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "packetsPerBurst" ) ) 
			reqp->control.pktsPerBurst = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "burstsPerStream" ) ) 
			reqp->control.burstsPerStream = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "rateControlSpec" ) ) 
			reqp->control.rateControl = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "rateControlInt" ) ) 
			reqp->control.rateControlIntPart = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "rateControlFrac" ) ) 
			reqp->control.rateControlFracPart = 
				strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "startTxDelay" ) ) 
			reqp->control.startTxDelay = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "interBurstGapInt" ) ) 
			reqp->control.interBurstGapIntPart = 
				strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "interBurstGapFrac" ) ) 
			reqp->control.interBurstGapFracPart = 
				strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "interStreamGapInt" ) ) 
			reqp->control.interStreamGapIntPart = 
				strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "interStreamGapFrac" ) ) 
			reqp->control.interStreamGapFracPart = 
				strtol( bsl_getNodeText(nodeL2), NULL, 10 );
	}

	//4. frameSizeTuple
	nodeL1 = BSL_XML_NEXT_NODE( nodeL1 );
	BSL_CHECK_NULL( nodeL1, ResultGeneralError );
	BSL_XML_FIND_NODE( nodeL1 );
	BSL_CHECK_NULL( nodeL1, ResultGeneralError );

	BSL_CHECK_TRUE( \
		xmlStrcmp( nodeL1->name, (xmlChar*)"frameSizeTuple" ) == 0,\
		ResultGeneralError );

	for( nodeL2 = nodeL1->xmlChildrenNode; nodeL2 != NULL; nodeL2 = nodeL2->next ) {
		if( nodeL2->type != XML_ELEMENT_NODE ) continue;
		if( BSL_XML_IS_NODE_NAME( nodeL2, "frameSizeSpec" ) ) 
			reqp->framesize.fsizeSpec = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "frameSizeOrStep" ) ) 
			reqp->framesize.sizeOrStep = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "frameSizeMin" ) ) 
			reqp->framesize.fsizeMin = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "frameSizeMax" ) ) 
			reqp->framesize.fsizeMax = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
	}

	//5. payloadTuple
	nodeL1 = BSL_XML_NEXT_NODE( nodeL1 );
	BSL_CHECK_NULL( nodeL1, ResultGeneralError );
	BSL_XML_FIND_NODE( nodeL1 );
	BSL_CHECK_NULL( nodeL1, ResultGeneralError );

	BSL_CHECK_TRUE( \
		xmlStrcmp( nodeL1->name, (xmlChar*)"payloadTuple" ) == 0,\
		ResultGeneralError );

	for( nodeL2 = nodeL1->xmlChildrenNode; nodeL2 != NULL; nodeL2 = nodeL2->next ) {
		if( nodeL2->type != XML_ELEMENT_NODE ) continue;
		if( BSL_XML_IS_NODE_NAME( nodeL2, "dataPatternType" ) ) 
			reqp->dataPatternType = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "payloadOffset" ) ) 
			reqp->poffset = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "payloadValidSize" ) ) 
			reqp->pvalidSize = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL2, "pattern" ) ) {
			//TODO: str or hex
			void* patternp = (void*)reqp->pattern;
			sprintf( patternp, "%s", bsl_getNodeText(nodeL2) );
		}
	}

	//6. PDR
//	void* pdrporg = NULL;
	T_PDR* pdrp = NULL;
	int pdrlen = 0;
	int alignedlen = reqp->pvalidSize % 4 == 0 ? reqp->pvalidSize :
		reqp->pvalidSize + 4 - reqp->pvalidSize%4;

	str2hex( reqp->pvalidSize, reqp->pattern, reqp->pattern );

	printf("%s: reqp->pvalidSize %d alignedlen %d\n", 
			__func__, reqp->pvalidSize, alignedlen );

	void* pdroffsetp = reqp->pattern + alignedlen;
	pdrp = pdroffsetp;
	do {
		int pidlocal = 0;
		int pdrlenlocal = 0;

		nodeL1 = BSL_XML_NEXT_NODE( nodeL1 );
		if( nodeL1 == NULL ) break;
		BSL_XML_FIND_NODE( nodeL1 );
		if( nodeL1 == NULL ) break;

		BSL_CHECK_TRUE( \
			xmlStrcmp( nodeL1->name, (xmlChar*)"PDR" ) == 0,\
			ResultGeneralError );

		for( nodeL2 = nodeL1->xmlChildrenNode; nodeL2 != NULL; nodeL2 = nodeL2->next ) {
		if( nodeL2->type != XML_ELEMENT_NODE ) continue;
			if( BSL_XML_IS_NODE_NAME( nodeL2, "protocolId" ) ) 
				pidlocal = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
			if( BSL_XML_IS_NODE_NAME( nodeL2, "length" ) ) 
				pdrlenlocal = strtol( bsl_getNodeText(nodeL2), NULL, 10 );
			if( BSL_XML_IS_NODE_NAME( nodeL2, "metadata" ) )  {
				pdrp = (T_PDR*)(pdroffsetp + pdrlen);
				pdrp->protocolid = pidlocal;
				pdrp->length = pdrlenlocal;
				pdrlen += sizeof( T_PDR ) + pdrlenlocal;

				if( xml_pdr_f_ptr[pdrp->protocolid] )
					xml_pdr_f_ptr[pdrp->protocolid]( nodeL2, pdrp );
				else 
					fprintf( stderr, "%s: No Such protocolid %d or NYI, pdrlen %d\n",
								__func__, pidlocal, pdrlenlocal );
			}
		}
		if( print_T_PDR_f_ptr[pdrp->protocolid] )
			print_T_PDR_f_ptr[pdrp->protocolid]( pdrp );
	} while(1);

	*datalen = sizeof(T_MSGIF_110_REQ_UNIT) + alignedlen + pdrlen;
#endif //if 0 XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXxx
	return ResultSuccess;
}

void xmlPdrTupleCustomInteger( xmlNodePtr node, T_CustomIntegerTuple* pdrp )
{
	xmlNodePtr nodeX;
	for( nodeX = node->xmlChildrenNode; nodeX != NULL; nodeX = nodeX->next ) {
		if( nodeX->type != XML_ELEMENT_NODE ) continue;
		if( BSL_XML_IS_NODE_NAME( nodeX, "mode" ) )
			pdrp->mode = strtol( bsl_getNodeText(nodeX), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeX, "stepOrValue" ) )
			pdrp->value = strtol( bsl_getNodeText(nodeX), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeX, "min" ) )
			pdrp->step = strtol( bsl_getNodeText(nodeX), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeX, "max" ) )
			pdrp->repeat = strtol( bsl_getNodeText(nodeX), NULL, 10 );
	} 
}

void xmlPdrTupleChecksum( xmlNodePtr node, T_ChecksumTuple* pdrp )
{
	xmlNodePtr nodeX;
	for( nodeX = node->xmlChildrenNode; nodeX != NULL; nodeX = nodeX->next ) {
		if( nodeX->type != XML_ELEMENT_NODE ) continue;
		if( BSL_XML_IS_NODE_NAME( nodeX, "type" ) )
			pdrp->type = strtol( bsl_getNodeText(nodeX), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeX, "value" ) )
			pdrp->value = strtol( bsl_getNodeText(nodeX), NULL, 10 );
	} 
}

void xmlPdrTupleIp4Addr( xmlNodePtr node, T_Ip4AddrTuple* pdrp )
{
	xmlNodePtr nodeX;
	for( nodeX = node->xmlChildrenNode; nodeX != NULL; nodeX = nodeX->next ) {
		if( nodeX->type != XML_ELEMENT_NODE ) continue;
		if( BSL_XML_IS_NODE_NAME( nodeX, "addr" ) )
			pdrp->addr = ntohl( inet_addr( bsl_getNodeText(nodeX) ) );
		if( BSL_XML_IS_NODE_NAME( nodeX, "mask" ) )
			pdrp->mask = strtol( bsl_getNodeText(nodeX), NULL, 16 );
		if( BSL_XML_IS_NODE_NAME( nodeX, "repeat" ) )
			pdrp->repeat = strtol( bsl_getNodeText(nodeX), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeX, "mode" ) )
			pdrp->mode = strtol( bsl_getNodeText(nodeX), NULL, 10 );
	} 
}

void xmlPdrTupleEtherAddr( xmlNodePtr node, T_EtherAddrTuple* pdrp )
{
	xmlNodePtr nodeX;
	for( nodeX = node->xmlChildrenNode; nodeX != NULL; nodeX = nodeX->next ) {
		if( nodeX->type != XML_ELEMENT_NODE ) continue;
		if( BSL_XML_IS_NODE_NAME( nodeX, "mode" ) )
			pdrp->mode = strtol( bsl_getNodeText(nodeX), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeX, "addr" ) ) {
			struct ether_addr* addr = ether_aton( bsl_getNodeText(nodeX) );
			if( addr ) memcpy( pdrp->addr, addr, SIZE_ETHER_ADDR );
			/* 
		    if( addr ) {
				pdrp->addr[0] = addr->ether_addr_octet[5];
				pdrp->addr[1] = addr->ether_addr_octet[4];
				pdrp->addr[2] = addr->ether_addr_octet[3];
				pdrp->addr[3] = addr->ether_addr_octet[2];
				pdrp->addr[4] = addr->ether_addr_octet[1];
				pdrp->addr[5] = addr->ether_addr_octet[0];
			}
			*/
		}
		if( BSL_XML_IS_NODE_NAME( nodeX, "repeatCount" ) )
			pdrp->repeatCount = strtol( bsl_getNodeText(nodeX), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeX, "step" ) )
			pdrp->step = strtol( bsl_getNodeText(nodeX), NULL, 10 );
	} 
}

static void xmlPdrEthernet( xmlNodePtr node, T_PDR* xmlpdrp )
{
	xmlNodePtr nodeL3;
	T_PDR_Ethernet* pdrp;
	BSL_CHECK_NULL( node, );
	BSL_CHECK_NULL( xmlpdrp, );

	pdrp = (T_PDR_Ethernet*)xmlpdrp->pinfo;

	for( nodeL3 = node->xmlChildrenNode; nodeL3 != NULL; nodeL3 = nodeL3->next ) {
		if( nodeL3->type != XML_ELEMENT_NODE ) continue;
		if( BSL_XML_IS_NODE_NAME( nodeL3, "destMac" ) )
			xmlPdrTupleEtherAddr( nodeL3, &pdrp->dest );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "srcMac" ) )
			xmlPdrTupleEtherAddr( nodeL3, &pdrp->src );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "etherType" ) )
			pdrp->type = strtol( bsl_getNodeText(nodeL3), NULL, 16 );
	}
}

static void xmlPdrIp4( xmlNodePtr node, T_PDR* xmlpdrp )
{
	xmlNodePtr nodeL3;
	T_PDR_Ip4* pdrp;
	BSL_CHECK_NULL( node, );
	BSL_CHECK_NULL( xmlpdrp, );

	pdrp = (T_PDR_Ip4*)xmlpdrp->pinfo;

	for( nodeL3 = node->xmlChildrenNode; nodeL3 != NULL; nodeL3 = nodeL3->next ) {
		if( nodeL3->type != XML_ELEMENT_NODE ) continue;
		if( BSL_XML_IS_NODE_NAME( nodeL3, "version" ) )
			pdrp->version = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "hlen" ) )
			pdrp->hlen = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "tos" ) )
			pdrp->tos = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "tlen" ) )
			pdrp->tlen = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "ident" ) )
			pdrp->id = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "flags" ) )
			pdrp->flags = strtol( bsl_getNodeText(nodeL3), NULL, 16 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "fragoffset" ) )
			pdrp->fragoffset = strtol( bsl_getNodeText(nodeL3), NULL, 16 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "flagoffset" ) )
			pdrp->fragoffset = strtol( bsl_getNodeText(nodeL3), NULL, 16 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "ttl" ) )
			pdrp->ttl = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "proto" ) )
			pdrp->proto = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "checksumTuple" ) )
			xmlPdrTupleChecksum( nodeL3, &pdrp->checksum );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "srcip" ) )
			xmlPdrTupleIp4Addr( nodeL3, &pdrp->sip );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "destip" ) )
			xmlPdrTupleIp4Addr( nodeL3, &pdrp->dip );
	}
}

static void xmlPdrUdp( xmlNodePtr node, T_PDR* xmlpdrp )
{
	xmlNodePtr nodeL3;
	T_PDR_UDP* pdrp;
	BSL_CHECK_NULL( node, );
	BSL_CHECK_NULL( xmlpdrp, );

	pdrp = (T_PDR_UDP*)xmlpdrp->pinfo;

	for( nodeL3 = node->xmlChildrenNode; nodeL3 != NULL; nodeL3 = nodeL3->next ) {
		if( nodeL3->type != XML_ELEMENT_NODE ) continue;
		if( BSL_XML_IS_NODE_NAME( nodeL3, "sport" ) )
			xmlPdrTupleCustomInteger( nodeL3, &pdrp->sport );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "dport" ) )
			xmlPdrTupleCustomInteger( nodeL3, &pdrp->dport );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "lengthOveride" ) )
			pdrp->override = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "lengthValue" ) )
			pdrp->val = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "checksumTuple" ) )
			xmlPdrTupleChecksum( nodeL3, &pdrp->checksum );
	}
}

static void 	xmlPdrTcp( xmlNodePtr node, T_PDR* xmlpdrp ) 
{
	xmlNodePtr nodeL3;
	T_PDR_TCP* pdrp;
	BSL_CHECK_NULL( node, );
	BSL_CHECK_NULL( xmlpdrp, );

	pdrp = (T_PDR_TCP*)xmlpdrp->pinfo;

	for( nodeL3 = node->xmlChildrenNode; nodeL3 != NULL; nodeL3 = nodeL3->next ) {
		if( nodeL3->type != XML_ELEMENT_NODE ) continue;
		if( BSL_XML_IS_NODE_NAME( nodeL3, "sport" ) )
			xmlPdrTupleCustomInteger( nodeL3, &pdrp->sport );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "dport" ) )
			xmlPdrTupleCustomInteger( nodeL3, &pdrp->dport );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "sequence" ) )
			pdrp->seqnum = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "ack" ) )
			pdrp->acknum = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "offset" ) )
			pdrp->offset = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "flag_res" ) )
			pdrp->flag_res = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "flag_urg" ) )
			pdrp->flag_urg = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "flag_ack" ) )
			pdrp->flag_ack = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "flag_psh" ) )
			pdrp->flag_psh = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "flag_rst" ) )
			pdrp->flag_rst = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "flag_syn" ) )
			pdrp->flag_syn = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "flag_fin" ) )
			pdrp->flag_fin = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "windows" ) )
			pdrp->windows = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "checksumTuple" ) )
			xmlPdrTupleChecksum( nodeL3, &pdrp->checksum );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "urgent" ) )
			pdrp->urgent = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
	}
}

static void 	xmlPdrIcmp( xmlNodePtr node, T_PDR* xmlpdrp ) 
{
	xmlNodePtr nodeL3;
	T_PDR_ICMP* pdrp;
	BSL_CHECK_NULL( node, );
	BSL_CHECK_NULL( xmlpdrp, );

	pdrp = (T_PDR_ICMP*)xmlpdrp->pinfo;

	for( nodeL3 = node->xmlChildrenNode; nodeL3 != NULL; nodeL3 = nodeL3->next ) {
		if( nodeL3->type != XML_ELEMENT_NODE ) continue;
		if( BSL_XML_IS_NODE_NAME( nodeL3, "type" ) )
			pdrp->type = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "code" ) )
			pdrp->code = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "data1" ) )
			pdrp->data1 = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "data2" ) )
			pdrp->data2 = strtol( bsl_getNodeText(nodeL3), NULL, 10 );
		if( BSL_XML_IS_NODE_NAME( nodeL3, "checksumTuple" ) )
			xmlPdrTupleChecksum( nodeL3, &pdrp->checksum );
	}
}

static void 	xmlPdrVLAN( xmlNodePtr node, T_PDR* xmlpdrp ) {}
static void 	xmlPdrISL( xmlNodePtr node, T_PDR* xmlpdrp ) {}
static void 	xmlPdrIp6( xmlNodePtr node, T_PDR* xmlpdrp ) {}
static void 	xmlPdrIp4OverIp6( xmlNodePtr node, T_PDR* xmlpdrp ){}
static void 	xmlPdrIp6OverIp4( xmlNodePtr node, T_PDR* xmlpdrp ){}
static void 	xmlPdrArp( xmlNodePtr node, T_PDR* xmlpdrp ) {}
static void 	xmlPdrIgmpv2( xmlNodePtr node, T_PDR* xmlpdrp ) {}

void str2hex( int size, char* str, unsigned char* outhexval )
{
	char c[2]={0,};
	int i=0;

	for( i=0; i<size; i++ ) {
		strncpy( c, str+(i*2), 2 );
		*(outhexval+i) = (unsigned char)strtol( c, 0, 16 );
	}
}

