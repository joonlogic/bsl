
#ifndef _BSL_API_COMMON_H_
#define _BSL_API_COMMON_H_

#if !defined(__KERNEL__)
	#include <stdio.h>
	#include <string.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <sys/ioctl.h>
	#include <fcntl.h>
	#include <termios.h>
#endif

typedef long long		 int64;
typedef int			 int32;
typedef short			 int16;
typedef char			 int8;
typedef unsigned long long	 uint64;
typedef unsigned int		 uint32;
typedef unsigned short		 uint16;
typedef unsigned char		 uint8;

#define DWORD	unsigned int
#define UINT64	unsigned long long
#define UINT32	unsigned int
#define UINT	unsigned int
#define PVOID	void *
#define UCHAR	unsigned char
#define BOOL	int
#define CHAR	char
#define TRUE	1
#define FALSE	0

#ifndef offsetof
#define offsetof(st, m) ((size_t)(&((st *)0)->m))
#endif
#define container_of(ptr, type, member) ({ \
				const typeof( ((type *)0)->member ) *__mptr = (ptr); \
				(type *)( (char *)__mptr - offsetof(type,member) );})

char getche_linux(void);

#endif

