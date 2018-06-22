// 2013년 6월 18일 양상현 수정 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>

#include "../module/bsl_register.h"
#include "../module/bsl_ctl.h"
#include "../api/bsl_api.h"

#define FILESIZE			0x10000

// MDIO Opmode flags
#define MDIO_OPMODE_ADDR	0x0
#define MDIO_OPMODE_WRITE	0x1
#define MDIO_OPMODE_READ	0x3

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

extern unsigned int phy_init_code[][2];

int fd;

static unsigned long long READ64(char *base, unsigned int addr)
{
	int ret;
	ioc_reg_access_t reg;

	memset(&reg, 0, sizeof(ioc_reg_access_t));

	reg.addr = addr;

	ret = ioctl(fd, CMD_REG_READ, &reg);
	if (ret < 0) {
		fprintf(stderr, "error: ioctl failed\n");
		return -1;
	}

	return reg.value;
}


static int WRITE64(char *base, unsigned int addr, unsigned long long value)
{
	int ret;
	ioc_reg_access_t reg;

	memset(&reg, 0, sizeof(ioc_reg_access_t));

	reg.addr = addr;
	reg.value = value;

	ret = ioctl(fd, CMD_REG_WRITE, &reg);
	if (ret < 0) {
		fprintf(stderr, "error: ioctl failed\n");
		return -1;
	}

	return 1;
}



void my_msleep(unsigned int msec)
{
	while (msec--) {
		usleep(1000);
	}
}


static char *base_addr;		// mmio address

unsigned short read_mdio_data(unsigned char phyaddr, unsigned char devaddr, unsigned short regaddr)
{
	int timeout = 10000;
	mdio_req_entry_t entry;

	memset(&entry, 0, sizeof(mdio_req_entry_t));

	//printf("phyaddr: %x, devaddr: %x, regaddr: %x\n", phyaddr, devaddr, regaddr);

	// 1. address operation
	entry.field.opmode = MDIO_OPMODE_ADDR;
	entry.field.phy_addr = phyaddr;
	entry.field.dev_addr = devaddr;
	entry.field.data = regaddr;

	WRITE64(base_addr, MCMR, entry.value.mcmr);
	WRITE64(base_addr, MRWR, MRWR_WRITE_FLAG | MRWR_SW_REQUEST_FLAG);

	//printf("1.addr : mcmr : %016llx\n", entry.value.mcmr);

	while ((READ64(base_addr, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
		usleep(1000);
		if (timeout-- == 0) {
			fprintf(stderr, "err: Address operation failed (timeout)\n");
			return -1;		// timeout
		}
	}

	my_msleep(10);

	timeout = 10000;

	// 2. read data operation
	entry.field.opmode = MDIO_OPMODE_READ;
	entry.field.phy_addr = phyaddr;
	entry.field.dev_addr = devaddr;
	entry.field.data = 0;		// unused

	WRITE64(base_addr, MCMR, entry.value.mcmr);
	WRITE64(base_addr, MRWR, MRWR_READ_FLAG | MRWR_SW_REQUEST_FLAG);

	//printf("2.read : mcmr : %016llx\n", entry.value.mcmr);

	while ((READ64(base_addr, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
		usleep(1000);
		if (timeout-- == 0) {
			fprintf(stderr, "err: Read operation failed (timeout)\n");
			return -1;		// timeout
		}
	}

	my_msleep(10);

	// data read

	//printf("3.ret : mrdr : %016llx\n", (READ64(base_addr, MRDR)));


	return (READ64(base_addr, MRDR));
}

int write_mdio_data(unsigned char phyaddr, unsigned char devaddr, unsigned short regaddr, unsigned short value)
{
	int timeout = 10000;
	mdio_req_entry_t entry;

	memset(&entry, 0, sizeof(mdio_req_entry_t));

	//printf("phyaddr: %x, devaddr: %x, regaddr: %x value: %x\n", phyaddr, devaddr, regaddr, value);

	// 1. address operation
	entry.field.opmode = MDIO_OPMODE_ADDR;
	entry.field.phy_addr = phyaddr;
	entry.field.dev_addr = devaddr;
	entry.field.data = regaddr;

	WRITE64(base_addr, MCMR, entry.value.mcmr);
	WRITE64(base_addr, MRWR, MRWR_WRITE_FLAG | MRWR_SW_REQUEST_FLAG);

	//printf("1.addr : mcmr : %016llx\n", entry.value.mcmr);

// 올바른 데이터를 읽어올수 없으므로 제거///////////////////////
#if 0
	while ((READ64(base_addr, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
		usleep(1000);
		if (timeout-- == 0) {
			fprintf(stderr, "err: Address operation failed (timeout)\n");
			return -1;		// timeout
		}
	}
#endif
//////////////////////////////////////////////////

	my_msleep(10);

	timeout = 10000;

	// 2. write data operation
	entry.field.opmode = MDIO_OPMODE_WRITE;
	entry.field.phy_addr = phyaddr;
	entry.field.dev_addr = devaddr;
	entry.field.data = value;

	WRITE64(base_addr, MCMR, entry.value.mcmr);
	WRITE64(base_addr, MRWR, MRWR_WRITE_FLAG | MRWR_SW_REQUEST_FLAG);

	//printf("2.write : mcmr : %016llx\n", entry.value.mcmr);
// 올바른 데이터를 읽어올수 없으므로 제거	///////////////////////////
#if 0
	while ((READ64(base_addr, MRWR) & MRWR_SW_REQUEST_FLAG) == 1) {
		usleep(1000);
		if (timeout-- == 0) {
			fprintf(stderr, "err: Write operation failed (timeout)\n");
			return -1;		// timeout
		}
	}
#endif
//////////////////////////////////////////////////////////////////////

	my_msleep(10);

	return 0;
}

int write_mdio_data_by_code(unsigned char phyaddr, unsigned int addr, unsigned int value)
{
	unsigned char devaddr;
	unsigned short regaddr;
	int ret;

	printf("phy : %d, addr : %05x, value : %04x\n", phyaddr, addr, value);

	devaddr = (addr >> 16);
	regaddr = (addr & 0xffff);
	ret = write_mdio_data(phyaddr, devaddr, regaddr, value);

	return ret;
}

int init_ael2005c_phy(unsigned char phyaddr)
{
	int i;

	write_mdio_data_by_code(phyaddr, 0x10000, 0xa040);
	sleep(1);

	write_mdio_data_by_code(phyaddr, 0x1c017, 0xfeb0);

	write_mdio_data_by_code(phyaddr, 0x1c013, 0xf341);
	write_mdio_data_by_code(phyaddr, 0x1c210, 0x8000);
	write_mdio_data_by_code(phyaddr, 0x1c210, 0x8100);
	write_mdio_data_by_code(phyaddr, 0x1c210, 0x8000);
	write_mdio_data_by_code(phyaddr, 0x1c210, 0x0000);
	sleep(1);

	write_mdio_data_by_code(phyaddr, 0x1c003, 0x181);
	write_mdio_data_by_code(phyaddr, 0x1c010, 0x448a);

	write_mdio_data_by_code(phyaddr, 0x1c04a, 0x5200);
	sleep(1);
	
	i = 0;
	while (1) {
		if (phy_init_code[i][0] == 0) {
			break;
		}
		write_mdio_data_by_code(phyaddr, phy_init_code[i][0], phy_init_code[i][1]);
		i++;
	}

	write_mdio_data_by_code(phyaddr, 0x1ca00, 0x0080);
	write_mdio_data_by_code(phyaddr, 0x1ca12, 0x0000);

	write_mdio_data_by_code(phyaddr, 0x1c01f, 0x0428);

	write_mdio_data_by_code(phyaddr, 0x1c214, 0x0098);

	return 0;
}

int bsl_sw_reset(void)
{
	WRITE64(base_addr, SWRR, 0);
	sleep(1);
	WRITE64(base_addr, SWRR, 1);
	return 0;
}

void help(char *progname)
{
	printf("%s; 10GbE PHY read/write (MDIO Interface)\n", progname);
	printf("      -h : Help\n");
    printf("      -c : card id. default 0\n");
	printf("      -p <Phy Addr> -d <Dev Addr> -r <Reg Addr(hex)> -c <Card id(hex)> : Read Register\n");
	printf("      -p <Phy Addr> -d <Dev Addr> -w <Reg Addr(hex)> -v <Data(hex)> -c <Card id(hex)> : Write Register\n");
//M CRACK CODEC
}

int main(int argc, char *argv[])
{
	int opt, opt_ok;
	int ret;
	char *map;
	int pa, da, rd, wr, val, init, test;	// options
	int phy_addr, dev_addr, reg_addr;
	unsigned short value, cmp_value;
	int cardid = 0;

	opt_ok = 0;
	pa = da = rd = wr = val = init = test = 0;

	while ((opt = getopt(argc, argv, "hp:d:r:w:v:itc:")) != -1) {

		switch (opt) {
			case 'h':
				goto help;
				break;
			case 'p':
				pa = 1;
				phy_addr = strtol(optarg, NULL, 16);
				break;
			case 'd':
				da = 1;
				dev_addr = strtol(optarg, NULL, 16);
				break;
			case 'r':
				rd = 1;
				reg_addr = strtol(optarg, NULL, 16);
				break;
			case 'w':
				wr = 1;
				reg_addr = strtol(optarg, NULL, 16);
				break;
			case 'v':
				val = 1;
				value = strtol(optarg, NULL, 16);
				break;
			case 'c':
				cardid = strtol(optarg, NULL, 10);
				break;
			case 'i':
				init = 1;
				opt_ok = 1;
				break;
			case 't':
				test = 1;
				opt_ok = 1;
				break;
		}
	}

	if (pa && da && rd) {
		opt_ok = 1;
	}

	if (pa && da && wr && val) {
		opt_ok = 1;
	}


help:
	if (opt_ok != 1) {
		help(argv[0]);
		exit(0);
	}

//	fd = open("/dev/bsl_ctl", O_RDWR, (mode_t)0600);
	fd = OPEN_DEVICE( cardid ); 
	if (fd < 0) {
		printf("err: file open failed\n");
		goto exit_main;
	}

	map = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		printf("err: mmap failed\n");
		goto close_fd;
	}

	base_addr = map;

	if (wr & val)  {
		write_mdio_data(phy_addr, dev_addr, reg_addr, value);
		printf("write ok\n");
	}

	if (rd)  {
		printf("P(%x),D(%x),R(%04x) -> %04x\n", phy_addr, dev_addr, reg_addr, read_mdio_data(phy_addr, dev_addr, reg_addr));
	}

	if (init)  {
		bsl_sw_reset();
		init_ael2005c_phy(0);
		init_ael2005c_phy(1);
	}

	if (test) {
		printf("Now Testing...\n");
		reg_addr = 0xc019;	// any address for only test
		value = 0x0;
		while (1) {
			write_mdio_data(0, 1, reg_addr, value);
			cmp_value = read_mdio_data(0, 1, reg_addr);
			
			if (value != cmp_value) {
				printf("Writing : %x\n", value);
				printf("Reading : %x\n", cmp_value);
				printf("\n--> Not matched!!!\n");
			}
			value++;
		}
	}

	ret = munmap(map, FILESIZE);
	if (ret < 0) {
		printf("err: munmap failed\n");
		goto close_fd;
	}

	close(fd);
	return 0;

close_fd:
	close(fd);
exit_main:
	return -1;
}

unsigned int phy_init_code[][2] = {
	{ 0x1cc00, 0x20c5 },
	{ 0x1cc01, 0x3c05 },
	{ 0x1cc02, 0x6536 },
	{ 0x1cc03, 0x2fe4 },
	{ 0x1cc04, 0x3cd4 },
	{ 0x1cc05, 0x6624 },
	{ 0x1cc06, 0x2015 },
	{ 0x1cc07, 0x3145 },
	{ 0x1cc08, 0x6524 },
	{ 0x1cc09, 0x27ff },
	{ 0x1cc0a, 0x300f },
	{ 0x1cc0b, 0x2c8b },
	{ 0x1cc0c, 0x300b },
	{ 0x1cc0d, 0x4009 },
	{ 0x1cc0e, 0x400e },
	{ 0x1cc0f, 0x2f52 },
	{ 0x1cc10, 0x3002 },
	{ 0x1cc11, 0x1002 },
	{ 0x1cc12, 0x2202 },
	{ 0x1cc13, 0x3012 },
	{ 0x1cc14, 0x1002 },
	{ 0x1cc15, 0x2662 },
	{ 0x1cc16, 0x3012 },
	{ 0x1cc17, 0x1002 },
	{ 0x1cc18, 0xd01e },
	{ 0x1cc19, 0x2862 },
	{ 0x1cc1a, 0x3012 },
	{ 0x1cc1b, 0x1002 },
	{ 0x1cc1c, 0x2004 },
	{ 0x1cc1d, 0x3c84 },
	{ 0x1cc1e, 0x6436 },
	{ 0x1cc1f, 0x2007 },
	{ 0x1cc20, 0x3f87 },
	{ 0x1cc21, 0x8676 },
	{ 0x1cc22, 0x40b7 },
	{ 0x1cc23, 0xa746 },
	{ 0x1cc24, 0x4047 },
	{ 0x1cc25, 0x5673 },
	{ 0x1cc26, 0x29c2 },
	{ 0x1cc27, 0x3002 },
	{ 0x1cc28, 0x13d2 },
	{ 0x1cc29, 0x8bbd },
	{ 0x1cc2a, 0x28f2 },
	{ 0x1cc2b, 0x3012 },
	{ 0x1cc2c, 0x1002 },
	{ 0x1cc2d, 0x2122 },
	{ 0x1cc2e, 0x3012 },
	{ 0x1cc2f, 0x1002 },
	{ 0x1cc30, 0x5cc3 },
	{ 0x1cc31, 0x314 },
	{ 0x1cc32, 0x2982 },
	{ 0x1cc33, 0x3002 },
	{ 0x1cc34, 0x1002 },
	{ 0x1cc35, 0xd019 },
	{ 0x1cc36, 0x20c2 },
	{ 0x1cc37, 0x3012 },
	{ 0x1cc38, 0x1002 },
	{ 0x1cc39, 0x2a04 },
	{ 0x1cc3a, 0x3c74 },
	{ 0x1cc3b, 0x6435 },
	{ 0x1cc3c, 0x2fa4 },
	{ 0x1cc3d, 0x3cd4 },
	{ 0x1cc3e, 0x6624 },
	{ 0x1cc3f, 0x5563 },
	{ 0x1cc40, 0x2d82 },
	{ 0x1cc41, 0x3002 },
	{ 0x1cc42, 0x13d2 },
	{ 0x1cc43, 0x464d },
	{ 0x1cc44, 0x28f2 },
	{ 0x1cc45, 0x3012 },
	{ 0x1cc46, 0x1002 },
	{ 0x1cc47, 0x20c2 },
	{ 0x1cc48, 0x3012 },
	{ 0x1cc49, 0x1002 },
	{ 0x1cc4a, 0x2fb4 },
	{ 0x1cc4b, 0x3cd4 },
	{ 0x1cc4c, 0x6624 },
	{ 0x1cc4d, 0x5563 },
	{ 0x1cc4e, 0x2d82 },
	{ 0x1cc4f, 0x3002 },
	{ 0x1cc50, 0x13d2 },
	{ 0x1cc51, 0x2eb2 },
	{ 0x1cc52, 0x3002 },
	{ 0x1cc53, 0x1002 },
	{ 0x1cc54, 0x2002 },
	{ 0x1cc55, 0x3012 },
	{ 0x1cc56, 0x1002 },
	{ 0x1cc57, 0x004 },
	{ 0x1cc58, 0x2982 },
	{ 0x1cc59, 0x3002 },
	{ 0x1cc5a, 0x1002 },
	{ 0x1cc5b, 0x2122 },
	{ 0x1cc5c, 0x3012 },
	{ 0x1cc5d, 0x1002 },
	{ 0x1cc5e, 0x5cc3 },
	{ 0x1cc5f, 0x317 },
	{ 0x1cc60, 0x2f52 },
	{ 0x1cc61, 0x3002 },
	{ 0x1cc62, 0x1002 },
	{ 0x1cc63, 0x2982 },
	{ 0x1cc64, 0x3002 },
	{ 0x1cc65, 0x1002 },
	{ 0x1cc66, 0x22cd },
	{ 0x1cc67, 0x301d },
	{ 0x1cc68, 0x28f2 },
	{ 0x1cc69, 0x3012 },
	{ 0x1cc6a, 0x1002 },
	{ 0x1cc6b, 0x21a2 },
	{ 0x1cc6c, 0x3012 },
	{ 0x1cc6d, 0x1002 },
	{ 0x1cc6e, 0x5aa3 },
	{ 0x1cc6f, 0x2e02 },
	{ 0x1cc70, 0x3002 },
	{ 0x1cc71, 0x1312 },
	{ 0x1cc72, 0x2d42 },
	{ 0x1cc73, 0x3002 },
	{ 0x1cc74, 0x1002 },
	{ 0x1cc75, 0x2ff7 },
	{ 0x1cc76, 0x30f7 },
	{ 0x1cc77, 0x20c4 },
	{ 0x1cc78, 0x3c04 },
	{ 0x1cc79, 0x6724 },
	{ 0x1cc7a, 0x2807 },
	{ 0x1cc7b, 0x31a7 },
	{ 0x1cc7c, 0x20c4 },
	{ 0x1cc7d, 0x3c24 },
	{ 0x1cc7e, 0x6724 },
	{ 0x1cc7f, 0x1002 },
	{ 0x1cc80, 0x2807 },
	{ 0x1cc81, 0x3187 },
	{ 0x1cc82, 0x20c4 },
	{ 0x1cc83, 0x3c24 },
	{ 0x1cc84, 0x6724 },
	{ 0x1cc85, 0x2fe4 },
	{ 0x1cc86, 0x3cd4 },
	{ 0x1cc87, 0x6437 },
	{ 0x1cc88, 0x20c4 },
	{ 0x1cc89, 0x3c04 },
	{ 0x1cc8a, 0x6724 },
	{ 0x1cc8b, 0x1002 },
	{ 0x1cc8c, 0x2514 },
	{ 0x1cc8d, 0x3c64 },
	{ 0x1cc8e, 0x6436 },
	{ 0x1cc8f, 0xdff4 },
	{ 0x1cc90, 0x6436 },
	{ 0x1cc91, 0x1002 },
	{ 0x1cc92, 0x40a4 },
	{ 0x1cc93, 0x643c },
	{ 0x1cc94, 0x4016 },
	{ 0x1cc95, 0x8c6c },
	{ 0x1cc96, 0x2b24 },
	{ 0x1cc97, 0x3c24 },
	{ 0x1cc98, 0x6435 },
	{ 0x1cc99, 0x1002 },
	{ 0x1cc9a, 0x2b24 },
	{ 0x1cc9b, 0x3c24 },
	{ 0x1cc9c, 0x643a },
	{ 0x1cc9d, 0x4025 },
	{ 0x1cc9e, 0x8a5a },
	{ 0x1cc9f, 0x1002 },
	{ 0x1cca0, 0x27c1 },
	{ 0x1cca1, 0x3011 },
	{ 0x1cca2, 0x1001 },
	{ 0x1cca3, 0xc7a0 },
	{ 0x1cca4, 0x100 },
	{ 0x1cca5, 0xc502 },
	{ 0x1cca6, 0x53ac },
	{ 0x1cca7, 0xc503 },
	{ 0x1cca8, 0xd5d5 },
	{ 0x1cca9, 0xc600 },
	{ 0x1ccaa, 0x2a6d },
	{ 0x1ccab, 0xc601 },
	{ 0x1ccac, 0x2a4c },
	{ 0x1ccad, 0xc602 },
	{ 0x1ccae, 0x111 },
	{ 0x1ccaf, 0xc60c },
	{ 0x1ccb0, 0x5900 },
	{ 0x1ccb1, 0xc710 },
	{ 0x1ccb2, 0x700 },
	{ 0x1ccb3, 0xc718 },
	{ 0x1ccb4, 0x700 },
	{ 0x1ccb5, 0xc720 },
	{ 0x1ccb6, 0x4700 },
	{ 0x1ccb7, 0xc801 },
	{ 0x1ccb8, 0x7f50 },
	{ 0x1ccb9, 0xc802 },
	{ 0x1ccba, 0x7760 },
	{ 0x1ccbb, 0xc803 },
	{ 0x1ccbc, 0x7fce },
	{ 0x1ccbd, 0xc804 },
	{ 0x1ccbe, 0x5700 },
	{ 0x1ccbf, 0xc805 },
	{ 0x1ccc0, 0x5f11 },
	{ 0x1ccc1, 0xc806 },
	{ 0x1ccc2, 0x4751 },
	{ 0x1ccc3, 0xc807 },
	{ 0x1ccc4, 0x57e1 },
	{ 0x1ccc5, 0xc808 },
	{ 0x1ccc6, 0x2700 },
	{ 0x1ccc7, 0xc809 },
	{ 0x1ccc8, 0x000 },
	{ 0x1ccc9, 0xc821 },
	{ 0x1ccca, 0x002 },
	{ 0x1cccb, 0xc822 },
	{ 0x1cccc, 0x014 },
	{ 0x1cccd, 0xc832 },
	{ 0x1ccce, 0x1186 },
	{ 0x1cccf, 0xc847 },
	{ 0x1ccd0, 0x1e02 },
	{ 0x1ccd1, 0xc013 },
	{ 0x1ccd2, 0xf341 },
	{ 0x1ccd3, 0xc01a },
	{ 0x1ccd4, 0x446 },
	{ 0x1ccd5, 0xc024 },
	{ 0x1ccd6, 0x1000 },
	{ 0x1ccd7, 0xc025 },
	{ 0x1ccd8, 0xa00 },
	{ 0x1ccd9, 0xc026 },
	{ 0x1ccda, 0xc0c },
	{ 0x1ccdb, 0xc027 },
	{ 0x1ccdc, 0xc0c },
	{ 0x1ccdd, 0xc029 },
	{ 0x1ccde, 0x0a0 },
	{ 0x1ccdf, 0xc030 },
	{ 0x1cce0, 0xa00 },
	{ 0x1cce1, 0xc03c },
	{ 0x1cce2, 0x01c },
	{ 0x1cce3, 0xc005 },
	{ 0x1cce4, 0x7a06 },
	{ 0x1cce5, 0x000 },
	{ 0x1cce6, 0x27c1 },
	{ 0x1cce7, 0x3011 },
	{ 0x1cce8, 0x1001 },
	{ 0x1cce9, 0xc620 },
	{ 0x1ccea, 0x000 },
	{ 0x1cceb, 0xc621 },
	{ 0x1ccec, 0x03f },
	{ 0x1cced, 0xc622 },
	{ 0x1ccee, 0x000 },
	{ 0x1ccef, 0xc623 },
	{ 0x1ccf0, 0x000 },
	{ 0x1ccf1, 0xc624 },
	{ 0x1ccf2, 0x000 },
	{ 0x1ccf3, 0xc625 },
	{ 0x1ccf4, 0x000 },
	{ 0x1ccf5, 0xc627 },
	{ 0x1ccf6, 0x000 },
	{ 0x1ccf7, 0xc628 },
	{ 0x1ccf8, 0x000 },
	{ 0x1ccf9, 0xc62c },
	{ 0x1ccfa, 0x000 },
	{ 0x1ccfb, 0x000 },
	{ 0x1ccfc, 0x2806 },
	{ 0x1ccfd, 0x3cb6 },
	{ 0x1ccfe, 0xc161 },
	{ 0x1ccff, 0x6134 },
	{ 0x1cd00, 0x6135 },
	{ 0x1cd01, 0x5443 },
	{ 0x1cd02, 0x303 },
	{ 0x1cd03, 0x6524 },
	{ 0x1cd04, 0x00b },
	{ 0x1cd05, 0x1002 },
	{ 0x1cd06, 0x2104 },
	{ 0x1cd07, 0x3c24 },
	{ 0x1cd08, 0x2105 },
	{ 0x1cd09, 0x3805 },
	{ 0x1cd0a, 0x6524 },
	{ 0x1cd0b, 0xdff4 },
	{ 0x1cd0c, 0x4005 },
	{ 0x1cd0d, 0x6524 },
	{ 0x1cd0e, 0x1002 },
	{ 0x1cd0f, 0x5dd3 },
	{ 0x1cd10, 0x306 },
	{ 0x1cd11, 0x2ff7 },
	{ 0x1cd12, 0x38f7 },
	{ 0x1cd13, 0x60b7 },
	{ 0x1cd14, 0xdffd },
	{ 0x1cd15, 0x00a },
	{ 0x1cd16, 0x1002 },
	{ 0x1cd17, 0x000 },


	{ 0, 0 },	// end code
};

