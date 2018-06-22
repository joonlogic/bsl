/********************************************************************
 *  FILE   : bsl_msgif.c
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
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "bsl_type.h"
#include "bsl_dbg.h"
#include "bsl_msgif.h"

static T_Thread thread_msgif;

static void* bsl_msgif_listen( void* arg );
typedef EnumResultCode (*bsl_handle_msgif_f_ptr_type) ( 
		void* msgdata, 
		unsigned int msglen, 
		void** msgreply, 
		unsigned int* msgreplylen );

extern bsl_handle_msgif_f_ptr_type \
	bsl_handle_msgid_100,
	bsl_handle_msgid_101,
	bsl_handle_msgid_102,
	bsl_handle_msgid_103,
	bsl_handle_msgid_104,
	bsl_handle_msgid_105,
	bsl_handle_msgid_106,
	bsl_handle_msgid_108,
	bsl_handle_msgid_109,
	bsl_handle_msgid_110,
	bsl_handle_msgid_111;
	bsl_handle_msgid_112;
	
static bsl_handle_msgif_f_ptr_type
    bsl_handle_msgif_f_ptr[SIZE_MAX_MSGIF_ID] = 
{
	[0] = (bsl_handle_msgif_f_ptr_type)&bsl_handle_msgid_100, //100
	[1] = (bsl_handle_msgif_f_ptr_type)&bsl_handle_msgid_101, //101
	[2] = (bsl_handle_msgif_f_ptr_type)&bsl_handle_msgid_102, //102
	[3] = (bsl_handle_msgif_f_ptr_type)&bsl_handle_msgid_103, //103
	[4] = (bsl_handle_msgif_f_ptr_type)&bsl_handle_msgid_104, //104
	[5] = (bsl_handle_msgif_f_ptr_type)&bsl_handle_msgid_105, //105
	[6] = (bsl_handle_msgif_f_ptr_type)&bsl_handle_msgid_106, //106
	[8] = (bsl_handle_msgif_f_ptr_type)&bsl_handle_msgid_108, //108
	[9] = (bsl_handle_msgif_f_ptr_type)&bsl_handle_msgid_109, //109
	[10] = (bsl_handle_msgif_f_ptr_type)&bsl_handle_msgid_110, //110
	[11] = (bsl_handle_msgif_f_ptr_type)&bsl_handle_msgid_111, //111
	[12] = (bsl_handle_msgif_f_ptr_type)&bsl_handle_msgid_112, //112
};

extern int 
bsl_socket_init( unsigned int addr, unsigned short port, struct sockaddr_in* server );
extern EnumResultCode bsl_socket_bind( int socket, struct sockaddr_in* server );
extern EnumResultCode bsl_socket_listen( int socket, int backlog );
extern int bsl_socket_accept( int socket, struct sockaddr_in* server );
extern void* bsl_read_msg( int sd, int* msglen );
extern EnumResultCode bsl_socket_shutdown( int socket );
extern EnumResultCode bsl_socket_connect( int socket, struct sockaddr_in* server );
static EnumResultCode bsl_handle_msg( int clientfd, void* msgdata, int msglen );

EnumResultCode
bsl_open_msg_listener( int socketport )
{
	int ret;

	ret = pthread_create( &thread_msgif.thrid, NULL, bsl_msgif_listen, (void*)&socketport );
	BSL_CHECK_RESULT( ret, ResultPthreadFailure );

	return ResultSuccess;
}

EnumResultCode
bsl_open_msg_listen_socket( char* ip, int port )
{
	int ret=0;
	struct sockaddr_in serverAddr;
	int serverfd;
	int clientfd;
	static const int backlog = 1;
	void* msgdata = NULL;
	int msglen = 0;
#ifdef _TARGET_
    pthread_mutex_t mutex_msg;
    ret = pthread_mutex_init( &mutex_msg, NULL );
    BSL_CHECK_RESULT( ret, ResultGeneralError );
#endif


	BSL_CHECK_NULL( ip, ResultSocketError );

	//socket
	memset( &serverAddr, 0, sizeof(serverAddr) );
	serverfd = bsl_socket_init( inet_addr(ip), port, &serverAddr );
	BSL_CHECK_EXP( ( serverfd == -1 ), ResultSocketError );

	//bind
	ret = bsl_socket_bind( serverfd, &serverAddr );
	BSL_CHECK_RESULT( ret, ResultBindError );

	//listen
	ret = bsl_socket_listen( serverfd, backlog );
	BSL_CHECK_RESULT( ret, ret );

	//accept
	clientfd = bsl_socket_accept( serverfd, &serverAddr );
	if( clientfd == -1 )
	{
		fprintf( stderr, "%s: bsl_socket_accept failed.\n", __func__ );
		return;
	}

	while(1)
	{
		msgdata = bsl_read_msg( clientfd, &msglen );
		if( !msgdata ) {
			BSL_MSG(("%s: msgdata NULL \n", __func__ ));
			break;
			continue;
		}
#ifdef _TARGET_
        pthread_mutex_lock( &mutex_msg );
#endif
		BSL_MSG(("%s: Message Arrived \n", __func__ ));

		bsl_handle_msg( clientfd, msgdata, msglen );

#ifdef _TARGET_
		pthread_mutex_unlock( &mutex_msg );
#endif
	}

#ifdef _TARGET_
        pthread_mutex_destroy( &mutex_msg );
#endif

	return bsl_socket_shutdown( serverfd );
}

void bsl_msgif_listen_loop( int socketport )
{
	char* serverip = IP_LOCALHOST;
	bsl_open_msg_listen_socket( serverip, socketport ); 
}

void* bsl_msgif_listen( void* arg ) 
{
    int ret;     
	int socketport = *(int*)arg;
	ret = pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, NULL ); 
	BSL_CHECK_RESULT( ret, NULL );

	ret = pthread_setcanceltype( PTHREAD_CANCEL_ASYNCHRONOUS, NULL ); 
	BSL_CHECK_RESULT( ret, NULL );

	bsl_msgif_listen_loop( socketport );

	pthread_exit( NULL );
	return NULL;
}

EnumResultCode
bsl_handle_msg( int clientfd, void* msgdata, int msglen )
{
	unsigned int* msgp = (unsigned int*)msgdata;
	unsigned int msgid = 0;
	void* replydata = NULL;
	unsigned int replylen = 0;
	unsigned int sendlen = 0;
	unsigned int sentlen = 0;
	EnumResultCode ret;

	BSL_CHECK_NULL( msgp, ResultMsgIfMsgHandleError );

	//check validity
	BSL_CHECK_EXP( VALUE_MSGIF_DELIM != MSGIF_GET_DELIM( msgp ), ResultMsgIfDelimError );
	msgid = MSGIF_GET_ID( msgp );
	BSL_CHECK_TRUE( msgid > VALUE_MSGID_OFFSET, ResultMsgIfMsgidError );
	BSL_CHECK_TRUE( msgid < VALUE_MAX_MSGIF_ID, ResultMsgIfMsgidError );
	msgid = msgid - VALUE_MSGID_OFFSET;
	BSL_CHECK_NULL( bsl_handle_msgif_f_ptr[msgid], ResultMsgIfMsgHandleNYI );

	ret = bsl_handle_msgif_f_ptr[msgid]( msgdata, msglen, &replydata, &replylen );
	BSL_CHECK_RESULT( ret, ret );

	do {
		if( msgid == 110 ) break; //for 110 request we don't send response.
		if( ( sentlen = write( clientfd, replydata, replylen ) ) <= 0 ) {
			fprintf( stderr, "%s: write error. clientfd %d, replylen %d, sendlen %d\n",
					__func__, clientfd, replylen, sendlen );
			free( replydata );
			return ResultMsgIfSendError;
		}
		sendlen += sentlen;
	} while( sendlen != replylen );

	free( replydata );
	return ResultSuccess;
}

EnumResultCode
bsl_open_client_socket( int socketport )
{
	EnumResultCode ret;
	struct sockaddr_in serveraddr;
	int socketfd;
	char* socketip = IP_LOCALHOST;
//	int socketport = TCP_PORT_SEND;

	memset( &serveraddr, 0, sizeof( serveraddr ) );

	//1. socket
	socketfd = bsl_socket_init( inet_addr(socketip), socketport, &serveraddr );
	BSL_CHECK_TRUE( socketfd != -1, ResultSocketError );

	//2. connect
	ret = bsl_socket_connect( socketfd, &serveraddr );
	if( ret != ResultSuccess )
	{
		bsl_socket_shutdown(socketfd);
		return ResultSocketError;
	}

//	sSocketfd = socketfd;

	return ResultSuccess;
}

