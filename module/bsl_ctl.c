#include "bsl.h"
#include "bsl_ctl.h"
//#include "bsl_stat.h"
//#include "bsl_stat_timer.h"

#include "bsl_pci.h"
//#include "bsl_acl.h"
//#include "bsl_syncookie.h"
//#include "bsl_ratelimit.h"
#include "bsl_util.h"

//joon_s
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#define SZ_1K (1<<10)
#define SZ_1M (1<<20)

unsigned long int  g_dev_count = 0;

struct cma_allocation {
	struct list_head list;
	size_t size;
	dma_addr_t dma;
	void *virt;
	bool inuse;
};

//static LIST_HEAD(cma_allocations);
//static DEFINE_SPINLOCK(cma_lock);
extern uint16_t bsl_read_mdio_data(bsl_pci_t *bsl_pci, uint8_t phyaddr, uint8_t devaddr, uint16_t regaddr);
extern int bsl_write_mdio_data(bsl_pci_t *bsl_pci, uint8_t phyaddr, uint8_t devaddr, uint16_t regaddr, uint16_t value); 

/*
 * any read request will free the 1st allocated coherent memory, eg.
 * cat /dev/cma_test
 */
static ssize_t
cma_test_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    struct cma_allocation *alloc = NULL;
	int devid = 0;
	bsl_pci_t* bsl_pcip = NULL;
	struct device *cma_dev;
	spinlock_t* cma_lock = NULL; 
	struct list_head* cma_allocations = NULL;

	if( ( devid = iminor(file->f_dentry->d_inode) ) >= bsl_devp->ndev ) {
		printk("%s: Error. device range over. iminor(file->f_dentry->d_inode) %d \
				bsl_devp->ndev %d\n",
				__func__, iminor(file->f_dentry->d_inode), bsl_devp->ndev );
		return -ENODEV;
	}

	printk("%s: devid %d\n", __func__, devid );

	bsl_pcip = &bsl_devp->bsl_pci[devid];
	cma_dev = &bsl_pcip->pdev->dev;
	cma_lock = &bsl_pcip->lock_cma;
	cma_allocations = &bsl_pcip->cma_allocations;

    spin_lock(cma_lock);
    if (!list_empty(cma_allocations)) {
        alloc = list_first_entry(cma_allocations,
            struct cma_allocation, list);
        list_del(&alloc->list);
    }
    spin_unlock(cma_lock);

    if (!alloc)
        return -EIDRM;

    dmam_free_coherent(cma_dev, alloc->size, alloc->virt,
        alloc->dma);

    _dev_info(cma_dev, "free: CM virt: %p dma: %p size:%zuK\n",
        alloc->virt, (void *)alloc->dma, alloc->size / SZ_1K);
    kfree(alloc);

    return 0;
}

/*
 * any write request will alloc a new coherent memory, eg.
 * echo 1024 > /dev/cma_test
 * will request 1024KiB by CMA
 */
static ssize_t
cma_test_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    struct cma_allocation *alloc;
	int devid = 0;
	bsl_pci_t* bsl_pcip = NULL;
	struct device *cma_dev;
	spinlock_t* cma_lock = NULL;
	struct list_head* cma_allocations = NULL;

    unsigned long size;
    int ret;

	if( ( devid = iminor(file->f_dentry->d_inode) ) >= bsl_devp->ndev ) {
		printk("%s: Error. device range over. iminor(file->f_dentry->d_inode) %d \
				bsl_devp->ndev %d\n",
				__func__, iminor(file->f_dentry->d_inode), bsl_devp->ndev );
		return -ENODEV;
	}
	bsl_pcip = &bsl_devp->bsl_pci[devid];
	cma_dev =&bsl_pcip->pdev->dev;
	cma_lock = &bsl_pcip->lock_cma;
	cma_allocations = &bsl_pcip->cma_allocations;

    ret = kstrtoul_from_user(buf, count, 0, &size);
    if (ret)
        return ret;

    if (!size)
        return -EINVAL;

    if (size > ~(size_t)0 / SZ_1K)
        return -EOVERFLOW;

    alloc = kmalloc(sizeof *alloc, GFP_KERNEL);
    if (!alloc)
        return -ENOMEM;

    alloc->size = size * SZ_1K;
    alloc->virt = dmam_alloc_coherent(cma_dev, alloc->size,
        &alloc->dma, GFP_KERNEL);

    if (alloc->virt) {
        _dev_info(cma_dev, "alloc: virt: %p dma: %p size: %zuK\n",
            alloc->virt, (void *)alloc->dma, alloc->size / SZ_1K);

        spin_lock(cma_lock);
        list_add_tail(&alloc->list, cma_allocations);
        spin_unlock(cma_lock);

        return count;
    } else {
        dev_err(cma_dev, "no mem in CMA area\n");
        kfree(alloc);
        return -ENOSPC;
    }
}

//joon_e for CMA

#ifdef BSL_DEBUG
	#define BSL_IOCTL_DEBUG
#endif

#ifndef VM_RESERVED
#define VM_RESERVED (VM_DONTEXPAND | VM_DONTDUMP)
#endif

ioc_io_buffer_t g_io_buffer;

   
void *share_memory;

static void bsl_vma_open(struct vm_area_struct *vma)
{
	DEBUG_FUNC("bsl_vma_open\n");
}

static void bsl_vma_close(struct vm_area_struct *vma)
{
	DEBUG_FUNC("bsl_vma_close\n");
}
  
static struct vm_operations_struct bsl_remap_vm_ops = {
	.open = bsl_vma_open,
	.close = bsl_vma_close,
};

static int bsl_remap_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int is_device_mem;
	off_t offset;
	uint32_t address_to_map;
	int devid = 0;
	bsl_pci_t* bsl_pcip = NULL;

	DEBUG_FUNC("skel_remap_mmap\n");

	if( ( devid = iminor(filp->f_dentry->d_inode) ) >= bsl_devp->ndev ) {
		printk("%s: Error. device range over. iminor(file->f_dentry->d_inode) %d \
				bsl_devp->ndev %d\n",
				__func__, iminor(filp->f_dentry->d_inode), bsl_devp->ndev );
		return -ENODEV;
	}
	bsl_pcip = &bsl_devp->bsl_pci[devid];
	
	offset = vma->vm_pgoff;

	switch (offset) {
		case 0:
			address_to_map = bsl_pcip->bar0_phys_addr;
			is_device_mem = 1;
			break;
		default:
			address_to_map = (uint32_t)offset << PAGE_SHIFT;
			is_device_mem = 0;
			break;
	}

	if (address_to_map == 0) {
		return -ENODEV;
	}

	//vma->vm_ops = &bsl_remap_vm_ops;
	vma->vm_flags |= VM_RESERVED;

	if (is_device_mem) {
		vma->vm_flags |= VM_IO;
	}

	if (remap_pfn_range(vma, vma->vm_start,
		address_to_map >> PAGE_SHIFT,
		vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
		return -EAGAIN;
	}

	//bsl_vma_open(vma);

	return 0;
}

static int bsl_ctl_open(struct inode *pnop, struct file *pfilep)
{
	DEBUG_FUNC("bsl_ctl_open\n");

	if( iminor( pnop ) >= bsl_devp->ndev ) {
		printk("%s: Error. device range over. iminor(pnop) %d bsl_devp->ndev %d\n",
				__func__, iminor(pnop), bsl_devp->ndev );
		return -ENODEV;
	}

	pfilep->private_data = bsl_devp;

	return 0;
}


static int bsl_ctl_release(struct inode *pnop, struct file *pfilep)
{
	DEBUG_FUNC("bsl_ctl_release\n");
	return 0;
}

static int bsl_alloc_cma( bsl_pci_t* bsl_pci, unsigned long size, int count )
{	
    struct cma_allocation *alloc = NULL;
	int i=0;
	struct device* cma_dev = &bsl_pci->pdev->dev;
	spinlock_t* cma_lock = &bsl_pci->lock_cma;
	struct list_head* cma_allocations = &bsl_pci->cma_allocations;

	for( i=0; i<count; i++ ) {
		alloc = kmalloc(sizeof *alloc, GFP_KERNEL);
		if (!alloc) {
			dev_err(cma_dev, "kmalloc error\n");
			return -ENOMEM;
		}

		alloc->size = size;
		alloc->virt = dmam_alloc_coherent( cma_dev,
				alloc->size, 
				&alloc->dma, 
				GFP_KERNEL | __GFP_NOWARN
				);                                                                                           
		if (alloc->virt == NULL) {        
			kfree( alloc );
			dev_err(cma_dev, "[count %d] no mem in CMA area\n", i);
			return NULL;
		}    

		alloc->inuse = false; 
		
		_dev_info(cma_dev, "[count %d] alloc: virt: %p dma: %p size: %zuM\n",
			i, alloc->virt, (void *)alloc->dma, alloc->size / SZ_1M);

		spin_lock(cma_lock);
		list_add_tail(&alloc->list, cma_allocations);
		spin_unlock(cma_lock);
	}

	return 0;

}

static int bsl_release_cma( bsl_pci_t* bsl_pci )
{
    struct cma_allocation *alloc = NULL;
	struct list_head* ptr;
	int i=0;
	struct device* cma_dev = &bsl_pci->pdev->dev;
	spinlock_t* cma_lock = &bsl_pci->lock_cma;
	struct list_head* cma_allocations = &bsl_pci->cma_allocations;

    spin_lock(cma_lock);
    list_for_each(ptr, cma_allocations ) { 
        alloc = list_entry(ptr, struct cma_allocation, list);
		// Release the buffer
		dmam_free_coherent( 
				cma_dev, 
				alloc->size,
				alloc->virt,
				alloc->dma );

		_dev_info(cma_dev, "[count %d] free: CM virt: %p dma: %p size:%zuM\n",
			i++, alloc->virt, (void *)alloc->dma, alloc->size / SZ_1M);
		kfree(alloc);
	}
    spin_unlock(cma_lock);


	/*
    spin_lock(&cma_lock);
    if (!list_empty(&cma_allocations)) {
        alloc = list_first_entry(&cma_allocations,
            struct cma_allocation, list);

    }
    spin_unlock(&cma_lock);

    if (!alloc)
        return -EIDRM;

	// Release the buffer
	dmam_free_coherent( 
			&(bsl_pci->pdev->dev),
			alloc->size,
			alloc->virt,
			alloc->dma );

    _dev_info(cma_dev, "free: CM virt: %p dma: %p size:%zuM\n",
        alloc->virt, (void *)alloc->dma, alloc->size / SZ_1M);
    kfree(alloc);
	*/
}

static unsigned long long bsl_dma_buffer_alloc(bsl_pci_t *bsl_pci, ioc_io_buffer_t *pio_buffer)
{
    struct cma_allocation *alloc = NULL;
	struct list_head* ptr;
	struct device* cma_dev = &bsl_pci->pdev->dev;
	spinlock_t* cma_lock = &bsl_pci->lock_cma;
	struct list_head* cma_allocations = &bsl_pci->cma_allocations;

    spin_lock(cma_lock);
    list_for_each(ptr, cma_allocations ) {
        alloc = list_entry(ptr, struct cma_allocation, list);
        if (alloc->inuse == false )
            break;
	}
    spin_unlock(cma_lock);

    if ( alloc->inuse ) {
        dev_err(cma_dev, "all in use in CMA area\n");
        return NULL;
	}

	alloc->inuse = true;
	pio_buffer->Size = alloc->size;
	pio_buffer->PhysicalAddr = alloc->dma;
	pio_buffer->pKernelVa = alloc->virt;

    _dev_info(cma_dev, "mark inuse: CM virt: %p dma: %p size:%zuM\n",
        alloc->virt, (void *)alloc->dma, alloc->size / SZ_1M);
	
    // Get CPU physical address of buffer
    pio_buffer->CpuPhysical = pio_buffer->PhysicalAddr;

	return pio_buffer->pKernelVa;
}

static void bsl_dma_buffer_free(bsl_pci_t *bsl_pci, ioc_io_buffer_t *pio_buffer)
{
    struct cma_allocation *alloc = NULL;
    struct list_head* ptr;
	struct device* cma_dev = &bsl_pci->pdev->dev;
	spinlock_t* cma_lock = &bsl_pci->lock_cma;
	struct list_head* cma_allocations = &bsl_pci->cma_allocations;

    _dev_info(cma_dev, "enter: pio_buffer->PhysicalAddr %p\n", pio_buffer->PhysicalAddr );

    spin_lock(cma_lock);
    list_for_each(ptr, cma_allocations) {
        alloc = list_entry(ptr, struct cma_allocation, list);
        if( pio_buffer->PhysicalAddr == NULL ) {
            alloc->inuse = false;
            _dev_info(cma_dev, "unmark inuse: CM virt: %p dma: %p size:%zuM\n",
                alloc->virt, (void *)alloc->dma, alloc->size / SZ_1M);
        }
        else if (alloc->dma == pio_buffer->PhysicalAddr ) {
            alloc->inuse = false;
            _dev_info(cma_dev, "unmark inuse: CM virt: %p dma: %p size:%zuM\n",
                alloc->virt, (void *)alloc->dma, alloc->size / SZ_1M);
            break;
        }
    }
    spin_unlock(cma_lock);
}



#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 0, 0)
static int bsl_ctl_ioctl(struct inode *pnop, struct file *pfilep, unsigned int cmd, unsigned long pbuf)
#else
static int bsl_ctl_ioctl(struct file *pfilep, unsigned int cmd, unsigned long pbuf)
#endif
{
	int err = 0, size;
	int ret = 0;
	int devid = 0;
	unsigned int value = 0;
	char str[24];
	ioc_reg_access_t reg;
	ioc_int_notification_t intinfo;
	ioc_io_buffer_t io_buffer;
	ioc_mdio_access_t mdio;
	DmaParams_t dmaParams;
	bsl_pci_t* bsl_pcip = NULL;

	DEBUG_FUNC("bsl_ctl_ioctl\n");

	if( ( devid = iminor(pfilep->f_dentry->d_inode) ) >= bsl_devp->ndev ) {
		printk("%s: Error. device range over. iminor(pfilep->f_dentry->d_inode) %d bsl_devp->ndev %d\n",
				__func__, iminor(pfilep->f_dentry->d_inode), bsl_devp->ndev );
		return -ENODEV;
	}

	bsl_pcip = &bsl_devp->bsl_pci[devid];

	memset(&reg, 0, sizeof(ioc_reg_access_t));
	memset(&mdio, 0, sizeof(ioc_mdio_access_t));
	memset(&str, 0, sizeof(str));
	memset(&intinfo, 0, sizeof(ioc_int_notification_t));
	memset(&io_buffer, 0, sizeof(ioc_io_buffer_t));
	memset(&dmaParams, 0, sizeof(DmaParams_t));

	if (_IOC_TYPE(cmd) != BSL_IOC_MAGIC) {
		DEBUG_MSG("not valid magic number\n");
		return -ENOTTY;
	}
	
	if (_IOC_NR(cmd) > CMD_IOC_MAXNR) {
		DEBUG_MSG("over ioctl max number\n");
		return -ENOTTY;
	}

	size = _IOC_SIZE(cmd);

	if (size) {
		if (_IOC_DIR(cmd) & _IOC_READ) {
			err = !access_ok(VERIFY_WRITE, (void *)pbuf, size);
		} else if (_IOC_DIR(cmd) & _IOC_WRITE) {
			err = !access_ok(VERIFY_READ, (void *)pbuf, size);
		}
		if (err) {
			return -EFAULT;
		}
	}

	switch (cmd) {

		case CMD_REG_WRITE:
			DEBUG_MSG("CMD_REG_WRITE\n");
			if (copy_from_user((void *)&reg, (const void *)pbuf, (unsigned long)size)) {
				ret = -EFAULT;
				break;
			}
			spin_lock(&(bsl_pcip->lock_hwaccess));

			BSL_WRITE_REG(bsl_pcip, reg.addr, reg.value);

			spin_unlock(&(bsl_pcip->lock_hwaccess));

			break;

		case CMD_REG_READ:
			DEBUG_MSG("CMD_REG_READ\n");
			if (copy_from_user((void *)&reg, (const void *)pbuf, (unsigned long)size)) {
				ret = -EFAULT;
				break;
			}
			spin_lock(&(bsl_pcip->lock_hwaccess));

			reg.value = BSL_READ_REG(bsl_pcip, reg.addr);

			spin_unlock(&(bsl_pcip->lock_hwaccess));

			if (copy_to_user((void *)pbuf, (const void *)&reg, (unsigned long)size)) {
				ret = -EFAULT;
			}
			break;

#if USE_INT_NOTIFICATION
		case CMD_INT_NOTIFICATION_WAIT:
			DEBUG_MSG("CMD_INT_NOTIFICATION_WAIT\n");
			if (copy_from_user((void *)&intinfo, (const void *)pbuf, (unsigned long)size)) {
				ret = -EFAULT;
				break;
			}
			ret = bsl_int_notification_wait(intinfo.irq_index, intinfo.timeout_ms);
			break;
#endif
		case CMD_GET_INTERRUPT_COUNT:
			DEBUG_MSG("CMD_GET_INTERRUPT_COUNT\n");
			if (copy_to_user((void *)pbuf, (const void *)&g_interrupt_count, (unsigned long int)size)) {
				ret = -EFAULT;
			}else{
			   ret = 0;
			}
			break;

		case CMD_GET_DEV_COUNT:
			DEBUG_MSG("CMD_GET_DEV_COUNT\n");
			if (copy_to_user((void *)pbuf, (const void *)&g_dev_count, (unsigned long int)size)) {
				ret = -EFAULT;
			}else{
			   ret = 0;
			}
			break;


		case CMD_UTIL_VERSION_FPGA_GET:
			DEBUG_MSG("CMD_UTIL_VERSION_FPGA_GET\n");
			ret = bsl_get_fpga_version(bsl_pcip, str);
			if (copy_to_user((void *)pbuf, (const void *)str, (unsigned long)size)) {
				ret = -EFAULT;
			}
			break;

		case CMD_UTIL_VERSION_SW_GET:
			DEBUG_MSG("CMD_UTIL_VERSION_SW_GET\n");
			strncpy(str, bsl_devp->version, 24);
			if (copy_to_user((void *)pbuf, (const void *)str, (unsigned long)size)) {
				ret = -EFAULT;
			}
			break;

		case CMD_UTIL_PORT_LINK_GET:
			DEBUG_MSG("CMD_UTIL_PORT_LINK_GET\n");
			ret = bsl_get_port_link(bsl_pcip, &value);
			if (copy_to_user((void *)pbuf, (const void *)&value, (unsigned long)size)) {
				ret = -EFAULT;
			}
			break;

		case CMD_PHY_MEM_ALLOC:
			DEBUG_MSG("CMD_PHY_MEM_ALLOC\n");
			if (copy_from_user((void *)&io_buffer, (const void *)pbuf, (unsigned long)size)) {
				ret = -EFAULT;
				break;
			}
			io_buffer.pKernelVa = bsl_dma_buffer_alloc(bsl_pcip, &io_buffer);
			if (io_buffer.pKernelVa == 0) {
				printk(KERN_ERR "Physical memory allocation failed\n");
				ret = -EFAULT;
				break;
			}

			if (copy_to_user((void *)pbuf, (const void *)&io_buffer, (unsigned long)size)) {
				ret = -EFAULT;
			}
			break;

		case CMD_PHY_MEM_FREE:
			DEBUG_MSG("CMD_PHY_MEM_FREE\n");
			if (copy_from_user((void *)&io_buffer, (const void *)pbuf, (unsigned long)size)) {
				ret = -EFAULT;
				break;
			}
			bsl_dma_buffer_free(bsl_pcip, &io_buffer);
			break;

		case CMD_READ_MDIO:
			DEBUG_MSG("CMD_READ_MDIO\n");
			if (copy_from_user((void *)&mdio, (const void *)pbuf, (unsigned long)size)) {
				ret = -EFAULT;
				break;
			}

			mdio.value = bsl_read_mdio_data(bsl_pcip, mdio.phyaddr, mdio.devaddr, mdio.regaddr );

			if (copy_to_user((void *)pbuf, (const void *)&mdio, (unsigned long)size)) {
				ret = -EFAULT;
			}
			break;

		case CMD_WRITE_MDIO:
			DEBUG_MSG("CMD_WRITE_MDIO\n");
			if (copy_from_user((void *)&mdio, (const void *)pbuf, (unsigned long)size)) {
				ret = -EFAULT;
				break;
			}

			bsl_write_mdio_data(bsl_pcip, mdio.phyaddr, mdio.devaddr, mdio.regaddr, mdio.value );

			break;

		default:
			return -ENOTTY;
	}
	return ret;
}

static struct file_operations bsl_fops = {
	.owner = THIS_MODULE,
	.open = bsl_ctl_open,
	.release = bsl_ctl_release,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 35)
	.ioctl = bsl_ctl_ioctl,
#else
	.unlocked_ioctl = bsl_ctl_ioctl,
#endif
	.mmap = bsl_remap_mmap,
	.read = cma_test_read,
	.write = cma_test_write,
};

int bsl_ctl_init(struct cdev *bsl_cdevp, int count)
{
	int ret;
	dev_t devno = MKDEV(BSL_DEV_MAJOR, 0);
	int ncma = 2;
	int i=0;

	DEBUG_FUNC("bsl_ctl_init\n");
	ret = register_chrdev_region(devno, count, BSL_DEV_NAME);
	if (ret < 0) {
		printk(KERN_WARNING "Can't get major\n");
		return -1;
	}
	
	cdev_init(bsl_cdevp, &bsl_fops);
	bsl_cdevp->owner = THIS_MODULE;
	bsl_cdevp->ops = &bsl_fops;
	ret = cdev_add(bsl_cdevp, devno, count);
	if (ret) {
		printk(KERN_WARNING "Bad cdev\n");
		return -1;
	}

	g_dev_count = count;

	//should be ncma = 2*count
	//ncma = 2*count;

	for( i=0; i<bsl_devp->ndev; i++ ) {
		bsl_alloc_cma( &bsl_devp->bsl_pci[i], SIZE_SUPPORT_DMA, ncma );
	}

	return 0;
}

void bsl_ctl_cleanup(struct cdev *bsl_cdevp, int count)
{
	int i=0;
	DEBUG_FUNC("bsl_ctl_cleanup\n");

	for( i=0; i<bsl_devp->ndev; i++ ) {
		bsl_release_cma( &bsl_devp->bsl_pci[i] );
	}

	cdev_del(bsl_cdevp);
	unregister_chrdev_region(MKDEV(BSL_DEV_MAJOR, 0), count);
}
