/********************************************************************
 *  FILE   : bsl_def.h
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

#ifndef BSL_DEF_H
#define BSL_DEF_H

#define SIZE_STRING_STREAM_NAME         64
#define SIZE_MAX_STREAM                 256
#define SIZE_MAX_PORT                   2
#define SIZE_MAX_CARD                   4
#define SIZE_MAX_PORTID                 1  //From 0
#define SIZE_MAX_CARDID                 3  //From 0
#define SIZE_MAX_PAYLOAD                65536
#define SIZE_MAX_VERSION_STRING         64        

#define ID_CARD_0                       0
#define ID_CARD_1                       1
#define ID_CARD(ID)                     (ID)

#define ID_PORT_0                       0
#define ID_PORT_1                       1
#define ID_PORT(ID)                     (ID)

#ifdef _TARGET_
#define TCP_PORT_LISTEN                 7788
#define TCP_PORT_SEND                   7789
#else
#define TCP_PORT_LISTEN                 8900
#define TCP_PORT_SEND                   8901
#endif
//#define IP_LOCALHOST                    "127.0.0.1"
#define IP_LOCALHOST                    "0.0.0.0"

#define SIZE_MSGIF_BUF                  1024
#define SIZE_MSGIF_HEADER               20

#define TIMEOUT_SEC_MSGIF               0 //10

#define SIZE_FRAME_MIN                  60
#define SIZE_FRAME_MAX                  1514

//UTIL                                                                                      
#ifdef WORDS_BIGENDIAN                                                                      
#define htonll(x)   (x)                                                                     
#define ntohll(x)   (x)                                                                     
#else                                                                                       
#define htonll(x) ((((unsigned long long)htonl(x)) << 32) + htonl(x >> 32))                 
#define ntohll(x) ((((unsigned long long)ntohl(x)) << 32) + ntohl(x >> 32))                 
#endif                                                                                      

#endif //BSL_DEF_H
