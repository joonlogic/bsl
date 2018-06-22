
#ifndef _BSL_MODULE_BSL_H_
#define _BSL_MODULE_BSL_H_

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fcntl.h>
#include <linux/bitops.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>
#pragma pack(1)

#define MAX_CARDS_SUPPORTED    4

typedef struct {
	struct proc_dir_entry *bsl_proc;
	struct timer_list bsl_timer;
} bsl_stat_t;

#define IRQ_MAX_NUMBER	32
#define IRQ_NAME_SIZE	20
typedef struct {
	uint32_t irq_index;
	uint32_t irq_vec;
	char irq_name[IRQ_NAME_SIZE];
	wait_queue_head_t waitq;		// for wait interrupt
	unsigned long flags;
	cpumask_var_t affinity_mask;
} bsl_irq_entry_t;

typedef struct {
	struct pci_dev *pdev;
	uint32_t bar0_phys_addr;		// 32bit address
	uint8_t *bar0_virt_addr;
	uint32_t bar0_size;
	uint32_t bar0_flag;
	spinlock_t lock_hwaccess;		// spinlock for register access
	spinlock_t lock_msix;			// spinlock for msix
	spinlock_t lock_cma;			// spinlock for cma

	struct list_head cma_allocations;
	uint32_t irq_cnt;
	bsl_irq_entry_t bsl_irq_table[IRQ_MAX_NUMBER];
} bsl_pci_t;

typedef struct {
	int ndev;
	char version[24];
	struct cdev bsl_cdev;
	bsl_stat_t bsl_stat;
	bsl_pci_t bsl_pci[MAX_CARDS_SUPPORTED];
} bsl_dev_t;

#pragma pack()	/*default*/

extern int debug_cnt;
extern bsl_dev_t *bsl_devp;

// delay macro
#ifndef msec_delay
#define msec_delay(x)   do { if(in_interrupt()) { \
	BUG(); \
} else { \
	msleep(x); \
} } while (0)
#endif

#define msec_delay_irq(x) mdelay(x)


// debug macro
//#define MSG_OUT(fmt, args...) printk(KERN_DEBUG fmt, ## args)
#define MSG_OUT(fmt, args...) printk(KERN_ERR fmt, ## args)

#undef DEBUG_MSG
#ifdef BSL_DEBUG
	#define DEBUG_MSG(fmt, args...) printk(KERN_DEBUG "[%d]bsl: " fmt, debug_cnt++, ## args)
#else
	#define DEBUG_MSG(fmt, args...)	/*nothing*/
#endif

#define DEBUG_FUNC(f)	DEBUG_MSG(f)


// device register access
#define BSL_WRITE_REG(a, reg, value) ( \
	writeq((value), (a)->bar0_virt_addr + \
		((reg) << 3)))

#define BSL_READ_REG(a, reg) ( \
	readq((a)->bar0_virt_addr + ((reg) << 3)))

#endif
