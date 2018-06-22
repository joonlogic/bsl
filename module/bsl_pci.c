#include "bsl.h"
#include "bsl_pci.h"

#include "bsl_register.h"
#include "bsl_phy.h"
#include "bsl_util.h"

static struct pci_device_id pci_tbl[] = {
	{ PCI_DEVICE(VENDOR_ID, DEVICE_ID), },
	{ 0, },
};
MODULE_DEVICE_TABLE(pci, pci_tbl);

//static bsl_pci_t *bsl_pci;
volatile unsigned long int  g_interrupt_count = 0;
static irqreturn_t bsl_int_handler(int irq, void *dev_id)
{
	bsl_irq_entry_t *irq_entry = dev_id;
	
	g_interrupt_count++;
#if USE_INT_NOTIFICATION
	if (!test_and_set_bit(0x1, &irq_entry->flags)) {
		wake_up_interruptible(&irq_entry->waitq);
	} else {
		DEBUG_MSG("Ignore interrupt %d, too busy.\n", irq_entry->irq_index);
	}
#endif
	return IRQ_HANDLED;
}

static int bsl_show_configuration(struct pci_dev *dev)
{
	uint16_t word;

	DEBUG_FUNC("bsl_show_configuration\n");

	pci_read_config_word(dev, PCI_VENDOR_ID, &word);
	DEBUG_MSG("vendor id : %04x\n", word);
	pci_read_config_word(dev, PCI_DEVICE_ID, &word);
	DEBUG_MSG("device id : %04x\n", word);
	pci_read_config_word(dev, PCI_COMMAND, &word);
	DEBUG_MSG("command : %x\n", word);

	return 0;
}

#ifdef USE_INT_MSIX_MODE
static int bsl_enable_msix(struct pci_dev *dev, int irq_cnt)
{
	int i, rc;
	struct msix_entry msix_ent[IRQ_MAX_NUMBER];

	DEBUG_FUNC("bsl_enable_msix\n");

	for (i = 0; i < IRQ_MAX_NUMBER; i++) {
		msix_ent[i].entry = i;
		msix_ent[i].vector = 0;
	}

	rc = pci_enable_msix(dev, msix_ent, irq_cnt);
	if (rc < 0) {
		return -1;
	} else if (rc != 0) {
		if (pci_enable_msix(dev, msix_ent, rc)) {
			return -1;
		}
		DEBUG_MSG("Requested %d MSI-X vectors, received %d\n", irq_cnt, rc);
		bsl_pci->irq_cnt = rc;
	} else {
		bsl_pci->irq_cnt = irq_cnt;
	}

	for (i = 0; i < IRQ_MAX_NUMBER; i++) {
		bsl_pci->bsl_irq_table[i].irq_index = i;
		bsl_pci->bsl_irq_table[i].irq_vec = msix_ent[i].vector;
	}

	return 0;
}

static void bsl_disable_msix(struct pci_dev *dev)
{
	DEBUG_FUNC("bsl_disable_msix\n");

	pci_disable_msix(dev);
}

static int bsl_request_irq(bsl_irq_entry_t *irq_entry)
{
	irq_handler_t fn;
	char *name;

	DEBUG_FUNC("bsl_request_irq\n");

	name = irq_entry->irq_name;

	fn = bsl_int_handler;
	sprintf(name, "bsl-msix-%d", irq_entry->irq_index);

	return request_irq(irq_entry->irq_vec, fn, 0, name, irq_entry);
}
#endif

bsl_pci_t* get_bsl_pcip( struct pci_dev* dev ) 
{
	int i=0;
	bsl_pci_t* bsl_pcip = NULL;

	for( i=0; i<bsl_devp->ndev; i++ ) {
		if( bsl_devp->bsl_pci[i].pdev == dev ) {
			bsl_pcip = &bsl_devp->bsl_pci[i];
			break;
		}
	}

	if( !bsl_pcip ) {
		printk("%s: Error. No such device registered.\n"
				"vendor %04X device %04X bsl_devp->ndev %d\n", 
				__func__, dev->vendor, dev->device, bsl_devp->ndev );
		return NULL;
	}

	return bsl_pcip; 
}

static int bsl_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	int i, ret;
	uint32_t bar0_phys_addr, bar0_size, bar0_flag;
	void *ioaddr;
	bsl_irq_entry_t *irq_entry;
	bsl_pci_t* bsl_pcip=NULL;

	printk("%s: dev %p id %p\n", __func__, dev, id);
	
	if(( bsl_pcip = get_bsl_pcip( dev )) == NULL ) {
		return -ENODEV;
	}

	if ((ret = pci_enable_device(dev)) < 0) {
		return ret;
	}

	pci_set_master(dev);
	
	// 2013 1115 kyg, http://www.mjmwired.net/kernel/Documentation/DMA-mapping.txt
	if (!pci_set_dma_mask(dev, DMA_BIT_MASK(64))) {
		if(!pci_set_consistent_dma_mask(dev, DMA_BIT_MASK(64))){
		    printk(KERN_WARNING "bsl_PCI_DMA: 64 bit consistent DMA available.\n");
        }else{
		    printk(KERN_WARNING "bsl_PCI_DMA: 64 bit DMA available.\n");
		}		  
	} else if (!pci_set_dma_mask(dev, DMA_BIT_MASK(32))) {
        if(!pci_set_consistent_dma_mask(dev, DMA_BIT_MASK(32))){
		    printk(KERN_WARNING "bsl_PCI_DMA: 32 bit consistent DMA available.\n");
        }else{
		    printk(KERN_WARNING "bsl_PCI_DMA: 32 bit DMA available.\n");
		}					   
	} else {
        printk(KERN_WARNING "bsl_PCI_DMA: No suitable DMA available.\n");			   
		//goto ignore_this_device;
	}
	
	if (bsl_show_configuration(dev) == -1) {
		return -ENODEV;
	}

	bar0_phys_addr = pci_resource_start(dev, 0);
	bar0_size = pci_resource_len(dev, 0);
	bar0_flag = pci_resource_flags(dev, 0);

	DEBUG_MSG("phys addr : %x\n", bar0_phys_addr);
	DEBUG_MSG("size : %x\n", bar0_size);
	DEBUG_MSG("flag : %x\n",bar0_flag );

	if (bar0_flag & IORESOURCE_MEM) {
		DEBUG_MSG("(MEMORY)\n");
	}

	if (bar0_flag & IORESOURCE_IO) {
		DEBUG_MSG("(IO)\n");
	}

	ret = pci_request_regions(dev, BSL_PCI_DRIVER_NAME);
	if (ret != 0) {
		printk(KERN_ERR "Request region failed\n");
		goto clean_disable;
	}

	ioaddr = ioremap(bar0_phys_addr, bar0_size);
	if (ioaddr == NULL) {
		printk(KERN_ERR "ioremap failed\n");
		goto clean_regions;
	}

	bsl_pcip->bar0_phys_addr = bar0_phys_addr;
	bsl_pcip->bar0_virt_addr = ioaddr;
	bsl_pcip->bar0_size = bar0_size;
	bsl_pcip->bar0_flag = bar0_flag;

#ifdef USE_INT_MSIX_MODE
	ret = bsl_enable_msix(dev, 32);
	if (ret != 0) {
		printk(KERN_ERR "enable msi-x failed\n");
		goto clean_unmap;
	}

	// Initialize irq vectors
	for (i = 0; i <bsl_pcip->irq_cnt; i++) {

		irq_entry = &bsl_pcip->bsl_irq_table[i];

#if USE_INT_NOTIFICATION
		init_waitqueue_head(&irq_entry->waitq);
#endif

		ret = bsl_request_irq(irq_entry);
		if (ret != 0) {
			for (i--; i >= 0; i--) {
				free_irq(irq_entry->irq_vec, NULL);
			}
			bsl_pcip->irq_cnt = 0;
			goto disable_msix;
		}
	}

	// Allocate the each core for irq vectors
	for (i = 0; i < bsl_pcip->irq_cnt; i++) {

		irq_entry = &bsl_pcip->bsl_irq_table[i];
		if (!alloc_cpumask_var(&irq_entry->affinity_mask, GFP_KERNEL)) {
			goto disable_irq;
		}
		cpumask_set_cpu(i, irq_entry->affinity_mask);
		printk("%d, %x\n", i, irq_entry->affinity_mask);
		//irq_set_affinity_hint(irq_entry->irq_vec, irq_entry->affinity_mask);
	}
#else	// Legacy Interrupt

	irq_entry = &bsl_pcip->bsl_irq_table[0];
	irq_entry->irq_index = 0;
	irq_entry->irq_vec = dev->irq;
	set_bit(0x1, &irq_entry->flags); // 2013 1016 kyg 추가함,
	// 최초에 1회는 인터럽트가 발생하지 않았는데도 CMD_INT_NOTIFICATION_WAIT에 응답하여서, bsl_diag_dma_buf.c에 확인하는 코드가 삽입되어 있고, 
	// 인터럽트 발생횟수를 계산하는 데, 부가적인 노력을 해야 하므로, 강제로 초기화 함.
	strcpy(irq_entry->irq_name, "bsl_int");
#if USE_INT_NOTIFICATION
	init_waitqueue_head(&irq_entry->waitq);
#endif

	ret = request_irq(irq_entry->irq_vec, bsl_int_handler, IRQF_SHARED, irq_entry->irq_name, irq_entry);
	if (ret < 0) {
		goto disable_irq;
	}

#endif

	DEBUG_MSG("virt addr : %lx\n", (unsigned long)ioaddr);

	DEBUG_MSG("FPGA Version : %016llx\n" , BSL_READ_REG(bsl_pcip, VERR));

	return 0;

disable_irq:
#ifdef USE_INT_MSIX_MODE
	for (i = bsl_pcip->irq_cnt - 1; i >= 0; i--) {
		irq_entry = &bsl_pcip->bsl_irq_table[i];
		free_irq(irq_entry->irq_vec, irq_entry);
	}
disable_msix:
	bsl_disable_msix(dev);
#endif
   
#ifdef USE_INT_MSIX_MODE
clean_unmap:
#endif
	iounmap(ioaddr);
clean_regions:
	pci_release_regions(dev);
clean_disable:
	pci_disable_device(dev);

	return -1;
}

static void bsl_remove(struct pci_dev *dev)
{
#ifdef USE_INT_MSIX_MODE
	int i;
#endif
	bsl_irq_entry_t *irq_entry = NULL;
	bsl_pci_t* bsl_pcip = NULL;

	printk("bsl_remove\n");

	if(( bsl_pcip = get_bsl_pcip( dev )) == NULL ) {
		return;
	}

#ifdef USE_INT_MSIX_MODE
	for (i = bsl_pcip->irq_cnt - 1; i >= 0; i--) {
		irq_entry = &bsl_pcip->bsl_irq_table[i];

		free_cpumask_var(irq_entry->affinity_mask);
		//irq_set_affinity_hint(irq_entry->irq_vec, NULL);

#if USE_INT_NOTIFICATION
		if (!test_and_set_bit(0x1, &irq_entry->flags)) {
			wake_up_interruptible(&irq_entry->waitq);
		}
#endif
		free_irq(irq_entry->irq_vec, irq_entry);
	}

	bsl_disable_msix(dev);

#else	// Legacy Interrupt

	irq_entry = &bsl_pcip->bsl_irq_table[0];

#if USE_INT_NOTIFICATION
	if (!test_and_set_bit(0x1, &irq_entry->flags)) {
		wake_up_interruptible(&irq_entry->waitq);
	}
#endif
	free_irq(irq_entry->irq_vec, irq_entry);
#endif

	if (bsl_pcip->bar0_virt_addr != NULL) {
		iounmap(bsl_pcip->bar0_virt_addr);
	}
	pci_release_regions(dev);
	pci_disable_device(dev);
}

static struct pci_driver bsl_pci_driver = {
	.name = BSL_PCI_DRIVER_NAME,
	.id_table = pci_tbl,
	.probe = bsl_probe,
	.remove = bsl_remove,
};

#if USE_INT_NOTIFICATION
int bsl_int_notification_wait(int irq_index, int timeout_ms)
{
	int wait_rc, ret = 0;
	unsigned int timeout_sec;
	bsl_irq_entry_t *irq_entry;

	timeout_sec = timeout_ms / 1000;

	timeout_ms = timeout_ms - (timeout_sec * 1000);

	timeout_sec = timeout_sec * HZ;
	timeout_ms = (timeout_ms * HZ) / 1000;

	timeout_ms = timeout_sec + timeout_ms;

	irq_entry = &bsl_pci->bsl_irq_table[irq_index];
	wait_rc = wait_event_interruptible_timeout(irq_entry->waitq, test_and_clear_bit(0x01, &irq_entry->flags), timeout_ms);

	if (wait_rc == 0) {
		DEBUG_MSG("Timeout waiting for interrupt %d\n", irq_index);
		ret = -1;
	}

	return ret;
}
#endif

int bsl_pci_init(bsl_dev_t *bsl_devp)
{
	int ret;
	struct pci_dev *pdev = NULL;
	const struct pci_device_id *ent;
	bsl_pci_t* bsl_pcip;
	int found = 0;
	int i=0;

	DEBUG_FUNC("bsl_pci_init\n");

	// try to find bsl pci device
	for_each_pci_dev(pdev) {
		ent = pci_match_id(pci_tbl, pdev);
		if (ent != NULL) {
			// find ok
			bsl_pcip = &bsl_devp->bsl_pci[found];
			bsl_pcip->pdev = pdev;
			bsl_devp->ndev++;
			found++;
		}
	}

	printk("%s: bsl_devp->ndev %d\n", __func__, bsl_devp->ndev );

	if (found == 0) {
		printk(KERN_ERR "BSL PCI device not found!!!\n");
		return 0;
	}

	// register pci driver
	ret = pci_register_driver(&bsl_pci_driver);
	if (ret < 0) {
		return -1;
	}

	// initialize hardware spinlock
	for(i=0; i<found; i++) {
		printk("%s: bsl_devp->bsl_pci[%d].pdev %p\n", 
				__func__, i, bsl_devp->bsl_pci[i].pdev );
		spin_lock_init(&bsl_devp->bsl_pci[i].lock_hwaccess);
		spin_lock_init(&bsl_devp->bsl_pci[i].lock_msix);
		spin_lock_init(&bsl_devp->bsl_pci[i].lock_cma);

		bsl_devp->bsl_pci[i].cma_allocations.next = 
			&bsl_devp->bsl_pci[i].cma_allocations;
		bsl_devp->bsl_pci[i].cma_allocations.prev = 
			&bsl_devp->bsl_pci[i].cma_allocations;
	}

	return found;
}

void bsl_pci_cleanup(bsl_pci_t *bsl_pcip)
{
	DEBUG_FUNC("bsl_pci_cleanup\n");

	pci_unregister_driver(&bsl_pci_driver);
}
