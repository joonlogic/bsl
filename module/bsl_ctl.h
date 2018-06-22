

#ifndef _BSL_MODULE_CTL_H_
#define _BSL_MODULE_CTL_H_

#include "../api/bsl_system.h"


#define BSL_DEV_NAME	"bsl_ctl"
#define BSL_DEV_MAJOR	245
#define BSL_DEV_MAX     4
#define SIZE_SUPPORT_DMA	( 1<<30 )   //1GB

// Information about contiguous buffer
typedef struct {
	unsigned long long pKernelVa;
	unsigned long long PhysicalAddr;
	unsigned long long CpuPhysical;
	unsigned int Size;
} ioc_io_buffer_t;

typedef struct dmaparams
{
	unsigned long long UserVa;       //User Address for Buffer
	unsigned long long BusAddr_Buff; //Bus Address for Buffer //unused for SGL
	unsigned long long BusAddr_Desc; //Bus Address for Descriptors when SGL is used
	unsigned long long VirtAddr_Desc;
	unsigned int       ByteCount;    //Total Size for Buffer
	unsigned int       SglSize_Desc; //Total Size for Descriptors
	struct page**      PageList;
	unsigned int       Channel;
} __attribute__((packed)) DmaParams_t;

// ioctl structure for register read/write
typedef struct {
	int addr;
	unsigned long long value;
} ioc_reg_access_t;

// for wait interrupt
typedef struct {
	int irq_index;
	int timeout_ms;
} ioc_int_notification_t;

// ioctl structure for mdion register read/write
typedef struct {
	unsigned char phyaddr;
	unsigned char devaddr;
	unsigned short regaddr;
	unsigned short value;
} ioc_mdio_access_t;

#define BSL_IOC_MAGIC	'n'

#define CMD_UTIL_VERSION_FPGA_GET   _IOR(BSL_IOC_MAGIC, 43, char[24])
#define CMD_UTIL_VERSION_SW_GET     _IOR(BSL_IOC_MAGIC, 44, char[24])

#define CMD_UTIL_PORT_LINK_GET      _IOR(BSL_IOC_MAGIC, 45, int)

#define CMD_UTIL_STAGE_SET          _IOW(BSL_IOC_MAGIC, 46, int)
#define CMD_UTIL_STAGE_GET          _IOR(BSL_IOC_MAGIC, 47, int)

#define CMD_REG_WRITE               _IOWR(BSL_IOC_MAGIC, 50, ioc_reg_access_t)
#define CMD_REG_READ                _IOWR(BSL_IOC_MAGIC, 51, ioc_reg_access_t)

#define CMD_INT_NOTIFICATION_WAIT   _IOWR(BSL_IOC_MAGIC, 52, ioc_int_notification_t)

#define CMD_DMA_TRANSFER_UBUFFER    _IOWR(BSL_IOC_MAGIC, 53, DmaParams_t)
#define CMD_DMA_CHANNEL_CLOSE       _IOWR(BSL_IOC_MAGIC, 54, DmaParams_t)

#define CMD_PHY_MEM_ALLOC           _IOWR(BSL_IOC_MAGIC, 55, ioc_io_buffer_t)
#define CMD_PHY_MEM_FREE            _IOWR(BSL_IOC_MAGIC, 56, ioc_io_buffer_t) 
#define CMD_PHY_MEM_MAP             _IOWR(BSL_IOC_MAGIC, 57, ioc_io_buffer_t)
#define CMD_PHY_MEM_UNMAP           _IOWR(BSL_IOC_MAGIC, 58, ioc_io_buffer_t)

#define CMD_GET_INTERRUPT_COUNT 		_IOR(BSL_IOC_MAGIC, 97, unsigned long int)
#define CMD_GET_DEV_COUNT 		    	_IOR(BSL_IOC_MAGIC, 98, unsigned long int)

#define CMD_READ_MDIO 		    	    _IOWR(BSL_IOC_MAGIC, 99, ioc_mdio_access_t)
#define CMD_WRITE_MDIO 		    	    _IOWR(BSL_IOC_MAGIC, 100, ioc_mdio_access_t)

#define CMD_IOC_MAXNR	110

#if defined(__KERNEL__)
int bsl_ctl_init(struct cdev *bsl_cdevp, int count);
void bsl_ctl_cleanup(struct cdev *bsl_cdevp, int count);
#endif

#endif
