#ifndef _BSL_MODULE_PCI_H_
#define _BSL_MODULE_PCI_H_

//#define VENDOR_ID	0x1556
//#define DEVICE_ID	0x1101
#define VENDOR_ID	0x10EE
#define DEVICE_ID	0x6028

#define BSL_PCI_DRIVER_NAME	"bsl_pci"

#define USE_INT_NOTIFICATION 	0	/* 0 : not use, 1 : use */

int bsl_int_notification_wait(int irq_index, int timeout_ms);
int bsl_pci_init(bsl_dev_t *);
void bsl_pci_cleanup(bsl_pci_t *bsl_pcip);

extern volatile unsigned long int  g_interrupt_count;
#endif
