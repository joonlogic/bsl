#include "bsl.h"
#include "bsl_util.h"

#include "bsl_register.h"
#include "bsl_phy.h"

int bsl_get_fpga_version(bsl_pci_t *bsl_pci, char *ver)
{
	uint8_t *ptr;
	uint32_t *date;
	uint64_t value;

	DEBUG_FUNC("bsl_fpga_version\n");

	spin_lock(&(bsl_pci->lock_hwaccess));

	value = BSL_READ_REG(bsl_pci, VERR);

	ptr = (uint8_t *)&value;
	date = (uint32_t *)&ptr[0];

	sprintf(ver, "%c%c%c%c-%x", ptr[7], ptr[6], ptr[5], ptr[4], *date);

	spin_unlock(&(bsl_pci->lock_hwaccess));

	return 0;
}

int bsl_get_port_link(bsl_pci_t *bsl_pci, int *status)
{
#define PHY_LINK_STATUS_BIT	4
	uint16_t value;
	int port;

	DEBUG_FUNC("bsl_get_port_link\n");

	for (port = 0; port < 2; port++) {
		value = bsl_read_mdio_data(bsl_pci, port, 1, 1);
		if (value & PHY_LINK_STATUS_BIT) {
			*status |= (1 << port);
		}
	}

	return 0;
}

int bsl_sw_reset(bsl_pci_t *bsl_pci)
{
	DEBUG_FUNC("bsl_sw_reset\n");

	spin_lock(&(bsl_pci->lock_hwaccess));

	BSL_WRITE_REG(bsl_pci, SWRR, 0);
	msec_delay_irq(1000);		// 1second delay
	BSL_WRITE_REG(bsl_pci, SWRR, 1);

	spin_unlock(&(bsl_pci->lock_hwaccess));

	return 0;
}
