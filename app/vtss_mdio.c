#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>

#include "../module/bsl_register.h"

#define READ64(base, reg)	(*(unsigned long long *)((base) + ((reg) << 3)))
#define WRITE64(base, reg, value)	(*(unsigned long long *)((base) + ((reg) << 3)) = (value))

// MDIO Opmode flags
#define MDIO_OPMODE_ADDR				0x0
#define MDIO_OPMODE_WRITE				0x1
#define MDIO_OPMODE_POST_READ_INC_ADDR	0x2
#define MDIO_OPMODE_READ				0x3

typedef union {
	struct {
		unsigned long long mcmr;
	} value;
	struct {
		unsigned long long data			: 16;
		unsigned long long dev_addr		: 5;
		unsigned long long phy_addr		: 5;
		unsigned long long opmode		: 2;
		unsigned long long unused		: 36;
	} field;
} mdio_req_entry_t;

void my_msleep(unsigned int msec)
{
	while (msec--) {
		usleep(1000);
	}
}

unsigned short read_mdio_data(unsigned char phyaddr, unsigned char devaddr, unsigned short regaddr)
{
	int timeout = 10000;
	mdio_req_entry_t entry;

	memset(&entry, 0, sizeof(mdio_req_entry_t));

	// 1. address operation
	entry.field.opmode = MDIO_OPMODE_ADDR;
	entry.field.phy_addr = phyaddr;
	entry.field.dev_addr = devaddr;
	entry.field.data = regaddr;
	printf ("MDIO READ OPERATION 0 !!!\n");
	printf ("base addr = %c\n", base_addr);

	WRITE64(base_addr, MCMR, entry.value.mcmr);
	printf ("MDIO READ OPERATION 1 !!!\n");
	WRITE64(base_addr, MRWR, MRWR_SW_REQUEST_FLAG);
	printf ("MDIO READ OPERATION 2 !!!\n");

	while ((READ64(base_addr, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
		usleep(1000);
	printf ("MDIO READ OPERATION 3 !!!\n");
		if (timeout-- == 0) {
			printf("err: Address operation failed (timeout)\n");
			return -1;		// timeout
		}
	}
	printf ("MDIO READ OPERATION 4 !!!\n");

	my_msleep(1);

	timeout = 10000;


	// 2. read data operation
	entry.field.opmode = MDIO_OPMODE_READ;
	entry.field.phy_addr = phyaddr;
	entry.field.dev_addr = devaddr;
	entry.field.data = 0;		// unused

	WRITE64(base_addr, MCMR, entry.value.mcmr);
	WRITE64(base_addr, MRWR, MRWR_SW_REQUEST_FLAG);

	while ((READ64(base_addr, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
		usleep(1000);
		if (timeout-- == 0) {
			printf("err: Read operation failed (timeout)\n");
			return -1;		// timeout
		}
	}

	my_msleep(1);

	// data read

	return (READ64(base_addr, MRDR));
}

int write_mdio_data(unsigned char phyaddr, unsigned char devaddr, unsigned short regaddr, unsigned short value)
{
	int timeout = 10000;
	mdio_req_entry_t entry;

	memset(&entry, 0, sizeof(mdio_req_entry_t));

	// 1. address operation
	entry.field.opmode = MDIO_OPMODE_ADDR;
	entry.field.phy_addr = phyaddr;
	entry.field.dev_addr = devaddr;
	entry.field.data = regaddr;

	WRITE64(base_addr, MCMR, entry.value.mcmr);
	WRITE64(base_addr, MRWR, MRWR_SW_REQUEST_FLAG);

	while ((READ64(base_addr, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
		usleep(1000);
		if (timeout-- == 0) {
			printf("err: Address operation failed (timeout)\n");
			return -1;		// timeout
		}
	}

	my_msleep(1);

	timeout = 10000;

	// 2. write data operation
	entry.field.opmode = MDIO_OPMODE_WRITE;
	entry.field.phy_addr = phyaddr;
	entry.field.dev_addr = devaddr;
	entry.field.data = value;

	WRITE64(base_addr, MCMR, entry.value.mcmr);
	WRITE64(base_addr, MRWR, MRWR_SW_REQUEST_FLAG);

	while ((READ64(base_addr, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
		usleep(1000);
		if (timeout-- == 0) {
			printf("err: Write operation failed (timeout)\n");
			return -1;		// timeout
		}
	}

	my_msleep(1);

	return 0;
	printf ("MDIO WRITE OPERATION !!!\n");
}

int read32_mdio_data(unsigned char phyaddr, unsigned char devaddr, unsigned short regaddr)
{
	int timeout = 10000;
	unsigned long long temp_read, temp_post_read_inc;
	int return_value;
	mdio_req_entry_t entry;

	memset(&entry, 0, sizeof(mdio_req_entry_t));

	// 1. address operation
	entry.field.opmode = MDIO_OPMODE_ADDR;
	entry.field.phy_addr = phyaddr;
	entry.field.dev_addr = devaddr;
	entry.field.data = regaddr;

	WRITE64(base_addr, MCMR, entry.value.mcmr);
	WRITE64(base_addr, MRWR, MRWR_SW_REQUEST_FLAG);

	while ((READ64(base_addr, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
		usleep(1000);
		if (timeout-- == 0) {
			printf("err: Address operation failed (timeout)\n");
			return -1;		// timeout
		}
	}

	my_msleep(1);

	timeout = 10000;

	// 2. post read increament address operation
	entry.field.opmode = MDIO_OPMODE_POST_READ_INC_ADDR;
	entry.field.phy_addr = phyaddr;
	entry.field.dev_addr = devaddr;
	entry.field.data = 0;		// unused

	WRITE64(base_addr, MCMR, entry.value.mcmr);
	WRITE64(base_addr, MRWR, MRWR_SW_REQUEST_FLAG);

	while ((READ64(base_addr, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
		usleep(1000);
		if (timeout-- == 0) {
			printf("err: Read operation failed (timeout)\n");
			return -1;		// timeout
		}
	}

	my_msleep(1);

	temp_post_read_inc = (READ64(base_addr, MRDR));

	timeout = 10000;

	// 3. read data operation
	entry.field.opmode = MDIO_OPMODE_READ;
	entry.field.phy_addr = phyaddr;
	entry.field.dev_addr = devaddr;
	entry.field.data = 0;		// unused

	WRITE64(base_addr, MCMR, entry.value.mcmr);
	WRITE64(base_addr, MRWR, MRWR_SW_REQUEST_FLAG);

	//printf("2.read : mcmr : %016llx\n", entry.value.mcmr);

	while ((READ64(base_addr, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
		usleep(1000);
		if (timeout-- == 0) {
			printf("err: Read operation failed (timeout)\n");
			return -1;		// timeout
		}
	}

	my_msleep(1);

	temp_read = (READ64(base_addr, MRDR));

	return_value = (temp_read << 16) | (0x000000000000ffff & temp_post_read_inc);

	// data read

//	return (READ64(base_addr, MRDR));
	return (return_value);
	printf ("MDIO READ32 OPERATION !!!\n");
}
