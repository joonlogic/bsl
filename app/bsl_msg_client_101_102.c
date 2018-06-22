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

#include "bsl_type.h"
#include "bsl_system.h"
#include "bsl_dbg.h"
#include "bsl_api.h"
#include "bsl_msgif.h"

const static char* this_version = "20131230";
extern char* bsl_getErrorStr( EnumResultCode );

typedef void (*bsl_app_handle_msg_f_ptr_type) ( void );
#if 0
bsl_app_handle_msg_f_ptr_type \
	bsl_app_handle_msgid_101,
	bsl_app_handle_msgid_102; 
#endif

#if 0
void bsl_app_handle_msgid_101( void );
void bsl_app_handle_msgid_102( void );

static bsl_app_handle_msg_f_ptr_type
	bsl_app_handle_msg_f_ptr[SIZE_MAX_MSGIF_ID] = 
{
	NULL,
	(bsl_app_handle_msg_f_ptr_type)&bsl_app_handle_msgid_101, //101
	(bsl_app_handle_msg_f_ptr_type)&bsl_app_handle_msgid_102, //102
};
#else
void bsl_app_handle_msgid_101_102( void );
#endif

void help(char *progname)
{
	fprintf( stderr, "%s BSL message socket client program. Ver %s\n", 
			progname, this_version );
	fprintf( stderr, "      -h : Help\n");
	fprintf( stderr, "      -v : Version\n");
	fprintf( stderr, "      -m msgid : request msg id\n");
}

int main(int argc, char *argv[])
{
	int opt, opt_ok = 0;
	int msgid  = 0;
	int fd, ret;
	char *map;
	int i;
	int dump, wr, rd, val;	// options
	unsigned int addr, value;

	opt_ok = 0;
	dump = wr = rd = val = 0;
#if 0
	while ((opt = getopt(argc, argv, "hm:")) != -1) {
		switch (opt) {
			case 'h':
				break;
			case 'm':
				opt_ok = 1;
				msgid=strtol( optarg, NULL, 10 );
				break;
		}
	}

	if (opt_ok != 1) {
		help(argv[0]);
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
#else
	bsl_app_handle_msgid_101_102( );
#endif

	return 0;
}

EnumResultCode
bsl_app_get_msg( void* resp, int resplen, int socketfd )
{
	fd_set readfds;
	int maxfd;
	int retval;
	int msgsize;
	struct timeval timeout;

	FD_ZERO( &readfds );
	FD_SET( socketfd, &readfds );
	maxfd = socketfd;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	retval = select( maxfd+1, &readfds, NULL, NULL, &timeout );
	BSL_CHECK_TRUE( retval, ResultSocketError );

	msgsize = read( socketfd, resp, resplen );
	if( msgsize <= 0 ) {
		fprintf( stderr, "%s: read error msgsize %d\n", __func__, msgsize );
		return ResultSocketError;
	}

	return ResultSuccess;
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
	static const char* socketip = "127.0.0.1";
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
bsl_app_handle_msgid_101_102( void )
{
	int socketfd = 0;
	T_MSGIF_HDR* reqp101 = NULL; 
	T_MSGIF_102_REQ* reqp102 = NULL; 
	T_MSGIF_101_RESP* resp101 = NULL;
	T_MSGIF_102_RESP* resp102 = NULL;

	socketfd = bsl_app_open_client_socket();
	if( socketfd < 0 ) {
		fprintf( stderr, "%s: open client socket failure. socketfd %d\n", 
				__func__, socketfd );
		return;
	}
//101 Start
	reqp101 = malloc( sizeof(T_MSGIF_HDR) );
	BSL_CHECK_NULL( reqp101, ResultMallocError );

	resp101 = malloc( sizeof(T_MSGIF_101_RESP) );
	BSL_CHECK_NULL( resp101, ResultMallocError );

	bsl_app_make_msghdr( reqp101 );
	MSGIF_SET_ID( reqp101, 101 );

	if( write( socketfd, reqp101, sizeof( T_MSGIF_HDR ) ) <= 0 )
	{
		fprintf( stderr, "%s: write failure. socketfd %d\n", 
				__func__, socketfd );
		free( reqp101 );
		bsl_socket_shutdown( socketfd );
		return;
	}

	fprintf( stderr, "%s: Sent Message Request\n", __func__ );

	bsl_app_get_msg( resp101, sizeof( T_MSGIF_101_RESP ), socketfd );

	fprintf( stderr, "[Client]%s: Received Messages =======\n", __func__ );
	print_msgid_101( resp101 );

	free( reqp101 );
	free( resp101 );
//101 End
//102 Start
	reqp102 = malloc( sizeof(T_MSGIF_102_REQ) );
	BSL_CHECK_NULL( reqp102, ResultMallocError );
	memset( reqp102, sizeof(T_MSGIF_102_REQ), 0x00 );

	resp102 = malloc( sizeof(T_MSGIF_102_RESP) );
	BSL_CHECK_NULL( resp102, ResultMallocError );
	memset( resp102, sizeof(T_MSGIF_102_RESP), 0x00 );

	bsl_app_make_msghdr( (T_MSGIF_HDR*)reqp102 );
	MSGIF_SET_ID( reqp102, 102 );
	MSGIF_SET_NRECORD( reqp102, 1 );
	MSGIF_SET_LENGTH( reqp102, sizeof( *reqp102 ) - sizeof( T_MSGIF_HDR ) );
	reqp102->cardid = htonl(1);
	reqp102->portid = htonl(2);
	reqp102->portmode = htonl(1);

	if( write( socketfd, reqp102, sizeof( T_MSGIF_102_REQ ) ) < 0 )
	{
		fprintf( stderr, "%s: write failure. socketfd %d\n", 
				__func__, socketfd );
		free( reqp102 );
		bsl_socket_shutdown( socketfd );
		return;
	}

	fprintf( stderr, "%s: Sent Message Request\n", __func__ );

	bsl_app_get_msg( resp102, sizeof( T_MSGIF_102_RESP ), socketfd );

	fprintf( stderr, "[Client]%s: Received Messages =======\n", __func__ );
	print_msgid_102( resp102 );

	free( reqp102 );
	free( resp102 );
//102 End

	sleep(10000);
	bsl_socket_shutdown( socketfd );
}

#if 0
void 
bsl_app_handle_msgid_102( void )
{
	int socketfd = 0;
	T_MSGIF_102_REQ* reqp = NULL; 
	T_MSGIF_102_RESP* resp = NULL;

	socketfd = bsl_app_open_client_socket();
	if( socketfd <= 0 ) {
		fprintf( stderr, "%s: open client socket failure. socketfd %d\n", 
				__func__, socketfd );
		return;
	}

	reqp = malloc( sizeof(T_MSGIF_102_REQ) );
	BSL_CHECK_NULL( reqp, ResultMallocError );
	memset( reqp, sizeof(T_MSGIF_102_REQ), 0x00 );

	resp = malloc( sizeof(T_MSGIF_102_RESP) );
	BSL_CHECK_NULL( reqp, ResultMallocError );
	memset( reqp, sizeof(T_MSGIF_102_RESP), 0x00 );

	bsl_app_make_msghdr( (T_MSGIF_HDR*)reqp );
	MSGIF_SET_ID( reqp, 102 );
	MSGIF_SET_NRECORD( reqp, 1 );
	MSGIF_SET_LENGTH( reqp, sizeof( *reqp ) - sizeof( T_MSGIF_HDR ) );
	reqp->cardid = htonl(1);
	reqp->portid = htonl(2);
	reqp->portmode = htonl(1);

	if( write( socketfd, reqp, sizeof( T_MSGIF_102_REQ ) ) < 0 )
	{
		fprintf( stderr, "%s: write failure. socketfd %d\n", 
				__func__, socketfd );
		free( reqp );
		bsl_socket_shutdown( socketfd );
		return;
	}

	fprintf( stderr, "%s: Sent Message Request\n", __func__ );

	bsl_app_get_msg( resp, sizeof( T_MSGIF_102_RESP ), socketfd );

	fprintf( stderr, "[Client]%s: Received Messages =======\n", __func__ );
	print_msgid_102( resp );

	free( reqp );
	free( resp );
}
#endif

