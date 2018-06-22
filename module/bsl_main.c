#include "bsl.h"
#include "bsl_pci.h"
#include "bsl_ctl.h"

int debug_cnt;
bsl_dev_t *bsl_devp;
static int ret_err;

//joon_s
//extern struct device* cma_dev;
//joon_e

int __init bsl_init(void)
{
	int ret;
	int i;
	int count;
	struct device* cma_dev;

	DEBUG_FUNC("bsl_init\n");

	bsl_devp = kmalloc(sizeof(bsl_dev_t), GFP_KERNEL);
	if (!bsl_devp) {
		printk(KERN_ERR "Bad kmalloc\n");
		return 1;
	}
	memset( (void*)bsl_devp, 0x00, sizeof(bsl_dev_t) );

	strncpy(bsl_devp->version, BSL_SW_VERSION, sizeof(bsl_devp->version));

	ret = bsl_pci_init(bsl_devp);
	if (ret < 0) {
		goto err;
	}
	count = ret;

	ret = bsl_ctl_init(&bsl_devp->bsl_cdev, count);
	if (ret < 0) {
		goto err;
	}

//joon_s
    for( i=0; i<count; i++ ) {
		cma_dev = &(bsl_devp->bsl_pci[i].pdev->dev);
		if (dma_set_mask_and_coherent(cma_dev, DMA_BIT_MASK(64))) {
			dev_warn(cma_dev, "mydev: No suitable DMA available\n");
		}
	}
//joon_e

	printk(KERN_INFO "BSL Module is now loaded. (Version : %s)\n", BSL_SW_VERSION);
	return 0;

err:
	ret_err = 1;	
	kfree(bsl_devp);
	bsl_devp = NULL;
	return 1;
}

void __exit bsl_cleanup(void)
{
	DEBUG_FUNC("bsl_cleanup\n");

	if (bsl_devp != NULL) {
		kfree(bsl_devp);
	}

	if (ret_err != 1) {
		bsl_ctl_cleanup(&bsl_devp->bsl_cdev, bsl_devp->ndev);
		bsl_pci_cleanup(&bsl_devp->bsl_pci[0]);
	}

	printk(KERN_INFO "BSL Module is now unloaded.\n");

}

MODULE_LICENSE("Dual BSD/GPL");
module_init(bsl_init);
module_exit(bsl_cleanup);
