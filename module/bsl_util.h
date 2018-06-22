#ifndef _BSL_UTIL_H_
#define _BSL_UTIL_H_

int bsl_get_fpga_version(bsl_pci_t *bsl_pci, char *ver);
int bsl_get_port_link(bsl_pci_t *bsl_pci, int *value);
int bsl_sw_reset(bsl_pci_t *bsl_pci);

#endif
