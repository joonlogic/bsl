#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>

#include "../module/bsl_register.h"
#include "../api/bsl_api.h"

#define FILESIZE	0x10000
#define VERSION_REG_ADDR	(VERR * 8)

#define READ64(base, offset)	(*(unsigned long long *)(base + offset))
#define WRITE64(base, offset, value)	(*(unsigned long long *)(base + offset) = value)

void help(char *progname)
{
	printf("%s; pci dump/read/write\n", progname);
	printf("      -h : Help\n");
	printf("      -c : card id. default 0\n");
	printf("      -d : Dump Registers (0 ~ 0x10000)\n");
	printf("      -r <Addr(hex)> -c <Card id(hex)> : Read Register\n");
	printf("      -w <Addr(hex)> -v <Data(hex)> -c <Card id(hex)> : Write Register\n");
	printf("      -t : Simple Test for Read/Write Register\n");
}

int main(int argc, char *argv[])
{
	int opt, opt_ok;
	int fd, ret;
	char *map;
	int i;
	int dump, wr, rd, val, simple, test;	// options
	int addr;
	int cardid=0;
	unsigned long long value;

	opt_ok = 0;
	dump = wr = rd = val = simple = test = 0;

	while ((opt = getopt(argc, argv, "hdr:w:v:stc:")) != -1) {
		switch (opt) {
			case 'h':
				goto help;
				break;
			case 'd':
				dump = 1;
				opt_ok = 1;
				break;
			case 'r':
				rd = 1;
				addr = strtol(optarg, NULL, 16);
				opt_ok = 1;
				break;
			case 'w':
				wr = 1;
				addr = strtol(optarg, NULL, 16);
				break;
			case 'v':
				val = 1;
				value =  strtoull(optarg, NULL, 16);
			case 's':
				simple = 1;
				opt_ok = 1;
				break;
			case 't':
				test = 1;
				opt_ok = 1;
				break;
			case 'c':
				cardid = strtol ( optarg, NULL, 10 );
				opt_ok = 1;
				break;
		}
	}

	if (wr && val) {
		opt_ok = 1;
	}

help:
	if (opt_ok != 1) {
		help(argv[0]);
		exit(0);
	}

//	fd = open("/dev/bsl_ctl", O_RDWR, (mode_t)0600);
	fd = OPEN_DEVICE( cardid );
//	char strdev[32]={0,};
//	sprintf( strdev, "/dev/bsl_ctl%s", cardid==0?"": cardid==1?"-1": "");
//	printf("cardid %d strdev %s\n", cardid, strdev );
//	fd = open(strdev, O_RDWR, (mode_t)0600);
	if (fd < 0) {
		printf("err: file open failed\n");
		goto exit_main;
	}


	map = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		printf("err: mmap failed\n");
		goto close_fd;
	}

	if (dump) {

		// Memory dump
		for (i = 0; i < FILESIZE; i += 8) {
			printf("%04x -> %016llx ", i, READ64(map, i));
			if (i % 4 == 0) {
				printf("\n");
				usleep(10000);
			}
		}
		///////////////////////////////////////////////
	}

	if (wr && val) {
			WRITE64(map, addr, value);
//			printf("write ok\n");
	}

	if (rd) {
/*		if (addr == VERSION_REG_ADDR) {
			unsigned char *ptr;
			unsigned int *date;

			value = READ64(map, addr);
			ptr = (unsigned char *)&value;
			date = (unsigned int *)&ptr[0];

			printf("%04x -> ", addr);
			printf("%c%c%c%c ", ptr[7], ptr[6], ptr[5], ptr[4]);
			printf("%x\n", *date);
		} else {
			if (simple) {
*/				printf("%016llx\n", READ64(map, addr));
/*			} else {
				printf("%04x -> %016llx\n", addr, READ64(map, addr));
			}
		}
*/	}

	if (test) {
		printf("Now Testing...\n");
		addr = 0xa10;	// any address for only test
		value = 0x0;
		while (1) {
			WRITE64(map, addr, value);
			
			printf("\r%016llx", value);
			if (value != READ64(map, addr)) {
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

