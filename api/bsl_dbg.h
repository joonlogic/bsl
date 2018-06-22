/********************************************************************
 *  FILE   : bsl_dbg.h
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

#ifndef BSL_DBG_H
#define BSL_DBG_H

#define BSL_TRACE(args)     printf args
#define BSL_INFO(args)      printf args
#define BSL_ERROR(args)     printf args
#define BSL_PRINT(args)     printf args
#define BSL_MSG(args)       printf args
#define BSL_DEV(args)       printf args

#define BSL_ASSERT(exp) \
	if( !(exp) ) { \
        fprintf(stderr, "%s:%s:%d BSL_ASSERT\n", __FILE__, __func__, __LINE__ ); \
		exit(0); \
	}

#define BSL_CHECK_NULL(exp, ret) \
	if( !(exp) ) { \
        fprintf(stderr, "%s:%s:%d BSL_CHECK_NULL\n", __FILE__, __func__, __LINE__ ); \
		return ret; \
	}

#define BSL_CHECK_EXP(exp, ret) \
	if( exp ) { \
        fprintf(stderr, "%s:%s:%d BSL_CHECK_EXP\n", __FILE__, __func__, __LINE__ ); \
		return ret; \
	}

#ifdef _TARGET_
#define BSL_CHECK_IOCTL(ret_ioctl, ret) \
	if( ret_ioctl < 0 ) { \
        fprintf(stderr, "%s:%s:%d BSL_CHECK_IOCTL ret_ioctl %d\n", \
				__FILE__, __func__, __LINE__, ret_ioctl ); \
		return ret; \
	}
#else
#define BSL_CHECK_IOCTL(ret_ioctl, ret) 
#endif

#define BSL_CHECK_RESULT(exp, ret) \
	if( (exp) != 0 ) { \
        fprintf(stderr, "%s:%s:%d BSL_CHECK_RESULT\n", __FILE__, __func__, __LINE__ ); \
		return ret; \
	} // 0 means SUCCESS

#define BSL_CHECK_TRUE(exp, ret) \
	if( (exp) != TRUE ) { \
        fprintf(stderr, "%s:%s:%d BSL_CHECK_TRUE\n", __FILE__, __func__, __LINE__ ); \
		return ret; \
	} 

#define BSL_CHECK_IOCTL_GOTO(ret_ioctl, where) \
    if( ret_ioctl < 0 ) { \
        fprintf(stderr, "%s:%s:%d BSL_CHECK_IOCTL_GOTO ret_ioctl %d\n", \
                    __FILE__, __func__, __LINE__, ret_ioctl ); \
        goto where; \
    }

#define BSL_CHECK_NULL_GOTO(p, where) \
    if( p == 0 ) { \
        fprintf(stderr, "%s:%s:%d BSL_CHECK_IOCTL_GOTO ret_ioctl %d\n", \
                    __FILE__, __func__, __LINE__, p ); \
        goto where; \
    }

#endif //BSL_DBG_H
