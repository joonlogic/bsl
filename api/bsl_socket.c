/********************************************************************
 *  FILE   : bsl_socket.c
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

#include "bsl_type.h"
#include "bsl_def.h"
#include "bsl_dbg.h"
#include "bsl_msgif.h"

int 
bsl_socket_init( 
		unsigned int addr, 
		unsigned short port, 
		struct sockaddr_in* server )
{
	int sockdes;
	BSL_CHECK_NULL( server, ResultNullArg );

	server->sin_family = AF_INET;
	server->sin_addr.s_addr = addr;
	server->sin_port = htons( port );

	sockdes = socket( AF_INET, SOCK_STREAM, 0 );

	return sockdes == -1 ? (int)ResultGeneralError : sockdes;
}

EnumResultCode
bsl_socket_connect( 
		int socket, 
		struct sockaddr_in* server )
{
	int rvstat = 0;
	rvstat = connect( socket, (struct sockaddr*)server, sizeof(*server) );
	return rvstat < 0 ? ResultSocketError : ResultSuccess;
}

EnumResultCode
bsl_socket_bind( 
		int socket, 
		struct sockaddr_in* server )
{
	int val = 1;
	struct linger stl;

	stl.l_onoff = 1;
	stl.l_linger = 10; //timeout 10sec.
	setsockopt( socket, SOL_SOCKET, SO_LINGER, (char*)&stl, sizeof(stl));

	if( setsockopt( socket, 
				SOL_SOCKET, 
				SO_REUSEADDR, 
				&val, 
				sizeof(val) ) == -1 ) 
		return ResultSocketError;

	setsockopt( socket, 
			SOL_SOCKET, 
			SO_KEEPALIVE, 
			(void*)&val, 
			sizeof(val) );

	return bind( socket, 
			(struct sockaddr*)server, 
			sizeof(*server) ) == -1 ?  
		ResultSocketError : ResultSuccess;
}

EnumResultCode
bsl_socket_listen( int socket, int backlog )
{
	return listen( socket, backlog ) == -1 ? 
		ResultSocketError : ResultSuccess;
}

EnumResultCode
bsl_socket_shutdown( int socket )
{
	if( socket >= 3 ) close( socket );
	return ResultSuccess;
}

int
bsl_socket_accept( int socket, struct sockaddr_in* server )
{
	int newsocket = 0;
	struct sockaddr_in client;
	int socklen = sizeof( client );
	
	newsocket = accept( 
			socket, 
			(struct sockaddr*)&client, 
			(socklen_t*)&socklen );

	return newsocket==-1 ? (int)ResultGeneralError : newsocket;
}

int
bsl_socket_send( int socket, const char* lpbuf, int buflen )
{
	int wbytes = 0;
	int n;

	while( wbytes < buflen ) {
		n = send( socket, (char*)(lpbuf+wbytes), buflen-wbytes, 0 );
		if( n < 0 ) return (int)ResultGeneralError;

		wbytes += n;
	}

	return wbytes;
}

int 
bsl_socket_receive( int socket, char* lpbuf, int buflen )
{
	int rbytes = 0;
	int n;

	while( rbytes < buflen ) {
		n = recv( socket, lpbuf+rbytes, buflen-rbytes, 0 );
		if( n < 0 ) return (int)ResultGeneralError;
		else if( n == 0 ) //connection was closed
				return (int)ResultGeneralError;

		rbytes += n;
	}

	return rbytes;
}

EnumResultCode
bsl_socket_connect_nonblock( 
		int socket, 
		struct sockaddr* addr, 
		size_t addr_len,
		int timeout_sec,
		int timeout_msec )
{
	EnumResultCode ret;
	fd_set rset, wset;
	int flags = 0;
	socklen_t len;

	if( timeout_sec || timeout_msec ) {
		//Nonblocking I/O
		flags = fcntl( socket, F_GETFL, 0 );
		fcntl( socket, F_SETFL, flags | O_NONBLOCK );
	}

	//try to connect
	ret = connect( socket, addr, addr_len );
	if( ret == 0 ) goto done;

	if( timeout_sec == 0 && timeout_msec == 0 ) {
		ret = ResultSocketError;
		goto done;
	}

	if( errno != EINPROGRESS ) {
		ret = ResultSocketError;
		goto done;
	}

	FD_ZERO( &rset );
	FD_SET( (unsigned int)socket, &rset );
	wset = rset;

	if( FD_ISSET( socket, &rset ) || FD_ISSET( socket, &wset ) ) {
		len = sizeof( ret );
		ret = ResultSuccess;
		if( getsockopt( socket, SOL_SOCKET, SO_ERROR, (char*)&ret, &len ) < 0 ) {
			ret = ResultSocketError;
			goto done;
		}
		else if( ret ) {
			ret = ResultSocketError;
			goto done;
		}
	}
	else {
		ret = ResultSocketError;
		goto done;
	}

	ret = ResultSuccess;
done:
	if( timeout_sec || timeout_msec ) fcntl( socket, F_SETFL, flags );

	return ret;
}

int
bsl_read_select( int sd, char* buf, int len, int timeout_sec )
{
	int ret=0;
	fd_set readset;
	struct timeval timeout_;

	FD_ZERO(&readset);
	FD_SET((unsigned int)sd, &readset);

	if(timeout_sec) {
		timeout_.tv_sec = timeout_sec;
		timeout_.tv_usec = 0;
	}

	ret = select(sd+1, &readset, NULL, &readset, (timeout_sec)?&timeout_:NULL);
	BSL_CHECK_EXP( ret <= 0, -1 );

	ret = recv( sd, buf, len, 0 );

	return ret <= 0 ? -1 : ret;
}

int
bsl_read_data( int sd, char* buf, int len, int timeout_sec )
{
	int ret=0;
	int readlen = len;

	while( readlen > 0 ) {
		ret = bsl_read_select( sd, buf+(len-readlen), readlen, timeout_sec );
		if( ret <= 0 ) return -1;
		readlen -= ret;
	}
	return len;
}

void* 
bsl_read_msg( int sd, int* msglen )
{
	int ret=0;
	int len = SIZE_MSGIF_HEADER;
	int bodylen = 0;
	void* msgdata;

	*msglen = 0;
	msgdata = (void*)malloc( SIZE_MSGIF_HEADER );
	BSL_CHECK_NULL( msgdata, NULL );

	ret = bsl_read_data( sd, msgdata, len, TIMEOUT_SEC_MSGIF );
	BSL_CHECK_EXP( ret != len, NULL );

	bodylen = MSGIF_GET_LENGTH( msgdata );
	if( bodylen ) {
		msgdata = realloc( msgdata, SIZE_MSGIF_HEADER + bodylen );
		BSL_CHECK_NULL( msgdata, NULL );

		ret = bsl_read_data( sd, msgdata+len, bodylen, TIMEOUT_SEC_MSGIF );
		if( ret != bodylen ) {
			fprintf( stderr, "%s: (ERROR) bodylen %d is not equal to readlen %d.\n",
					__func__, bodylen, ret );
			bodylen = 0;
		}
	}

	*msglen = bodylen + SIZE_MSGIF_HEADER;

	//debug message
	BSL_MSG(("%s: msglen = %d\n", __func__, *msglen ));
	do {
		int i=0;
		char msgbuf[32768] = {0,};
		unsigned int* msgptr = (unsigned int*)msgdata;
		for(i=0; i<*msglen; i+=4, msgptr++)
		{
			if( *msglen > 2000 ) {
				printf("Message size is too long so truncated\n");
				break;
			}
			sprintf( msgbuf + strlen(msgbuf), "[%02d] %08X | (dec)%d\n", 
					i, ntohl( *msgptr ), ntohl( *msgptr ) );
		}

		BSL_MSG(("%s", msgbuf));
	} while(0);

	return msgdata;
}

