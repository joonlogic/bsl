
#ifndef _BSL_PHY_H_
#define _BSL_PHY_H_

// MDIO Opmode flags
typedef enum {
	MDIO_OPMODE_ADDR  = 0x0,
	MDIO_OPMODE_WRITE = 0x1 ,
	MDIO_OPMODE_READ  = 0x3,
} mdio_opmode_t;


#pragma pack(1)

typedef union {
	struct {
		uint64_t mcmr;
	} value;
	struct {
		uint64_t data         : 16;
		uint64_t dev_addr     : 5;
		uint64_t phy_addr     : 5;
		uint64_t opmode       : 2;
		uint64_t unused       : 36;
	} field;
} hw_mdio_req_entry_t;

#pragma pack()	/*default*/

uint16_t bsl_read_mdio_data(bsl_pci_t *bsl_pci, uint8_t phyaddr, uint8_t devaddr, uint16_t regaddr);
int bsl_write_mdio_data(bsl_pci_t *bsl_pci, uint8_t phyaddr, uint8_t devaddr, uint16_t regaddr, uint16_t value);
int bsl_phy_init(bsl_pci_t *bsl_pci);

#endif
