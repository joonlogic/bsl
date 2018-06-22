#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>

#include "/root/trunk/module/bsl_register.h"

#define FILESIZE	0x10000
#define VERSION_REG_ADDR	(VERR * 8)

#define READ64(base, offset)	(*(volatile unsigned long long *)(base + offset))
#define WRITE64(base, offset, value)	(*(volatile unsigned long long *)(base + offset) = value)

void help(char *progname)
{
	printf("%s; pci dump/read/write\n", progname);
	printf("      -h : Help\n");
	printf("      -d : Dump Registers (0 ~ 0x10000)\n");
	printf("      -r <Addr(hex)> : Read Register\n");
	printf("      -w <Addr(hex)> -v <Data(hex)> : Write Register\n");
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
	int reg_size;									// register test number
	int test_addr[200];
	unsigned long long addr_save = 0;
	unsigned long long reg_map[][3]={
{0x0000,1,0x0000000000000000},    //C0AR
{0x0008,1,0x0000000000000000},    //C0SR
//{0x0200,1,"0x0000_0000_year,month_date,sequence"},	//VERR
{0x0220,1,0x0000000000000000},    //DBGR
{0x0230,2,0x0000000000000001},    //SWRR
{0x0240,2,0x0000000000000000},    //TIMR
{0x0400,1,0x0000000000000000},    //PCR0
{0x0410,1,0x0000000000000000},    //BCR0
{0x0420,1,0x0000000000000000},    //PS0R0
{0x0430,1,0x0000000000000000},    //PS1R0
{0x0440,1,0x0000000000000000},    //PS2R0
{0x0450,1,0x0000000000000000},    //PS3R0
{0x0460,1,0x0000000000000000},    //PS4R0
{0x0600,1,0x0000000000000000},    //PCR1
{0x0610,1,0x0000000000000000},    //PCR1
{0x0620,1,0x0000000000000000},    //PS0R1
{0x0630,1,0x0000000000000000},    //PS1R1
{0x0640,1,0x0000000000000000},    //PS2R1
{0x0650,1,0x0000000000000000},    //PS3R1
{0x0660,1,0x0000000000000000},    //PS4R1
{0x0800,1,0x0000000000000000},    //FINR
{0x0810,1,0x0000000000000000},    //FINO
{0x0820,1,0x0000000000000000},    //SYNR
{0x0830,1,0x0000000000000000},    //SYNO
{0x0840,1,0x0000000000000000},    //RSTR
{0x0850,1,0x0000000000000000},    //RSTO
{0x0860,1,0x0000000000000000},    //PSHR
{0x0870,1,0x0000000000000000},    //PSHO
{0x0880,1,0x0000000000000000},    //ACKR
{0x0890,1,0x0000000000000000},    //ACKO
{0x08A0,1,0x0000000000000000},    //URGR
{0x08B0,1,0x0000000000000000},    //URGO
{0x08C0,1,0x0000000000000000},    //SYAR
{0x08D0,1,0x0000000000000000},    //SYAO
{0x08E0,1,0x0000000000000000},    //IP4R
{0x08F0,1,0x0000000000000000},    //ARPR
{0x0900,1,0x0000000000000000},    //ETC3
{0x0910,1,0x0000000000000000},    //TCPR
{0x0920,1,0x0000000000000000},    //UDPR
{0x0930,1,0x0000000000000000},    //ICMP
{0x0940,1,0x0000000000000000},    //IGMP
{0x0950,1,0x0000000000000000},    //ETC4
{0x0A00,2,0x0000000000000000},    //ERWR
{0x0A10,2,0x0000000000000000},    //EADR
{0x0A20,2,0x0000000000000000},    //EWDR0
{0x0A30,2,0x0000000000000000},    //EWDR1
{0x0A40,2,0x0000000000000000},    //EWDR2
{0x0A50,2,0x0000000000000000},    //EWDR3
{0x0A60,1,0x0000000000000000},    //ERDR0
{0x0A70,1,0x0000000000000000},    //ERDR1
{0x0A80,1,0x0000000000000000},    //ERDR2
{0x0A90,1,0x0000000000000000},    //ERDR3
{0x0AA0,2,0x0000000000000000},    //EGDR
{0x0AB0,2,0x0000000000000050},    //ELCR
{0x0C00,2,0x0000000000000000},    //DPBR
{0x0C10,2,0x0000000000000000},    //RHTR
{0x0C20,2,0x000000000000000A},    //ARTR
{0x0C30,2,0x0000000000000001},    //SCER
{0x0C50,1,0x0000000000000000},    //PPCR
{0x0C60,1,0x0000000000000000},    //BPCR
{0x0C70,1,0x0000000000000000},    //RLCR
{0x0CA0,1,0x0000000000000000},    //SPCR
{0x0CB0,1,0x0000000000000000},    //SAPC
{0x0CD0,1,0x0000000000000000},    //RPCR
{0x0CE0,1,0x0000000000000000},    //SFCR
{0x0CF0,1,0x0000000000000000},    //MTHR
{0x0D00,1,0x0000000000000000},    //PFER
{0x0D10,2,0x00FFFFFF7FFFFFFF},    //DLSR
{0x0D30,2,0x0010000000100000},    //DHSR0
{0x0D40,2,0x0000000000100000},    //DHSR1
{0x0E00,2,0x0000000000000000},    //BYPR
{0x1000,2,0x0000000000000000},    //MRWR
{0x1010,2,0x0000000000000000},    //MCMR
{0x1020,1,0x0000000000000000},    //MRDR
{0x1200,1,0x0000000000000000},    //TTPCR0
{0x1210,1,0x0000000000000000},    //TNPCR0
{0x1220,1,0x0000000000000000},    //TCPCR0
{0x1230,1,0x0000000000000000},    //TTBCR0
{0x1240,1,0x0000000000000000},    //TNBCR0
{0x1250,1,0x0000000000000000},    //TCBCR0
{0x1400,1,0x0000000000000000},    //TTPCR1
{0x1410,1,0x0000000000000000},    //TTBCR1
{0x1600,2,0x0000000000000000},    //DSAR
{0x1610,1,0x0000000000000000},    //DCAR
{0x1620,2,0x0000000000001000},    //DISR
{0x1800,2,0x00000000000FFFFF},    //APPR
{0x1810,1,0x0000000000000000},    //AS0R
{0x1820,1,0x0000000000000000},    //AS1R
{0x1830,1,0x0000000000000000},    //AS2R
{0x1840,1,0x0000000000000000},    //AS3R
{0x1850,1,0x0000000000000000},    //AS4R
{0x1860,1,0x0000000000000000},    //AS5R
{0x1870,1,0x0000000000000000},    //AS6R
{0x1880,1,0x0000000000000000},    //AS7R
{0x1890,1,0x0000000000000000},    //AS8R
{0x18A0,1,0x0000000000000000},    //AS9R
{0x18B0,1,0x0000000000000000},    //AS10R
{0x18C0,1,0x0000000000000000},    //AS11R
{0x18D0,1,0x0000000000000000},    //AS12R
{0x18E0,1,0x0000000000000000},    //AS13R
{0x18F0,1,0x0000000000000000},    //AS14R
{0x1900,1,0x0000000000000000},    //AS15R
{0x1910,1,0x0000000000000000},    //AS16R
{0x1920,1,0x0000000000000000},    //AS17R
{0x1930,1,0x0000000000000000},    //AS18R
{0x1940,1,0x0000000000000000},    //AS19R
{0x1A00,2,0x0000000000000000},    //MSCR
{0x1A10,2,0x0000000000000000},    //MSAR
{0x1A20,2,0x0000000000000000},    //MWDR
{0x1A30,1,0x0000000000000000},    //MRDR
{0x1C00,2,0x0000000000000000},    //RWCR
{0x1C10,2,0x0000000000000000},    //ADDR
{0x1C20,2,0x0000000000000000},    //CMWR
{0x1C30,1,0x0000000000000000},    //CMRR
{0x1C40,2,0x0000000000000000},    //WDR0
{0x1C50,2,0x0000000000000000},    //WDR1
{0x1C60,2,0x0000000000000000},    //WDR2
{0x1C70,2,0x0000000000000000},    //WDR3
{0x1C80,2,0x0000000000000000},    //WDR4
{0x1C90,1,0x0000000000000000},    //RDR0
{0x1CA0,1,0x0000000000000000},    //RDR1
{0x1CB0,1,0x0000000000000000},    //RDR2
{0x1CC0,1,0x0000000000000000},    //RDR3
{0x1CD0,1,0x0000000000000000},    //RDR4
{0x1CE0,1,0x0000000000000000},    //RDR5
{0x1CF0,1,0x0000000000000000},    //RDR6
{0x1D00,1,0x0000000000000000},    //RDR7
{0x1D10,1,0x0000000000000000},    //RDR8
{0x1E00,1,0x0000000000000000},    //RDR0
{0x1E10,1,0x0000000000000000},    //RDR1
{0x1E20,1,0x0000000000000000},    //RDR2
{0x1E30,1,0x0000000000000000},    //RDR3
{0x1E40,1,0x0000000000000000},    //RDR4
{0x1E50,1,0x0000000000000000},    //RDR5
{0x1E60,1,0x0000000000000000},    //RDR6
{0x1E70,1,0x0000000000000000}};    //RDR7

	unsigned long long value;

	reg_size = sizeof(reg_map)/sizeof(reg_map[0]);

	opt_ok = 0;
	dump = wr = rd = val = simple = test = 0;

	while ((opt = getopt(argc, argv, "hdr:w:v:st")) != -1) {
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

	fd = open("/dev/bsl_ctl", O_RDWR, (mode_t)0600);
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
			printf("write ok\n");
	}

	if (rd) {
		if (addr == VERSION_REG_ADDR) {
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
				printf("%016llx\n", READ64(map, addr));
			} else {
				printf("%04x -> %016llx\n", addr, READ64(map, addr));
			}
		}
	}

	if (test) {
		int t_one = 0, t_two = 0, t_three = 0;
		unsigned long long last_one = 0, last_two = 0, last_three = 0;
		printf("Now Testing...\n");
		for(i=0;i<reg_size;i++)
		{
			if(reg_map[i][1] == 2)
			{
				reg_map[i][2] = READ64(map, reg_map[i][0]);
			}
		}
//	addr = 0xa20;	// any address for only test
//	value = 0x0;
//	=================================================
//	READ TEST
//	=================================================
		printf("Test 1-1\n");
		for(i=0;i<reg_size;i++)
		{
			if(i != 0)
			{
				if(reg_map[i][0] == 0)
				{
					continue;
				}
			}
			if(reg_map[i][0] == 0x0000)	
			{
				printf("Skip 0x0000 = debuging\n");
				continue;
			}
			if(reg_map[i][0] == 0x0008)
			{
				printf("Skip 0x0008 = debuging\n");
		 		continue;
			}
			if(reg_map[i][0] == 0x0CF0) 
			{
				printf("Skip 0x0CF0 = debuging\n");
				continue;
			}
			if(reg_map[i][0] == 0x0D00)	
			{
				printf("Skip 0x0D00 = debuging\n");
				continue;
			}
//			if(reg_map[i][1] == 2)
//			{
//				continue;
//			}
			addr_save = READ64(map, reg_map[i][0]);
			if(reg_map[i][2] != addr_save)
			{
				printf("addr=0x%04llX\t//type=%llX\ndefault value=0x%016llX\t===> now value=0x%016llX\nNot matched!!!!\n", 
							reg_map[i][0], reg_map[i][1], reg_map[i][2], addr_save);
			}
			test_addr[t_one] = reg_map[i][0];
			last_one = reg_map[i][0];
			t_one++;
		}
//	=================================================
//	WRITE AND READ TEST
//	=================================================
		printf("\nTest 1-2\n");
		for(i=0;i<reg_size;i++)
		{
			if(reg_map[i][1] == 2)
			{
				if(reg_map[i][0] == 0x0A00)
				{
					printf("\nSkip 0x0A00 = 0x55~55\n");

					continue;
				}

        if(reg_map[i][0] == 0x1000)
        {
          printf("\nSkip 0x1000 = 0x55~55\n");

					continue;
        }

        if(reg_map[i][0] == 0x1A00)
        {
          printf("\nSkip 0x1A00 = 0x55~55\n");

					continue; 
        }

        if(reg_map[i][0] == 0x1C00)
        {
          printf("\nSkip 0x1C00 = 0x55~55\n");

					continue;
        }

				value = 0x5555555555555555;
				WRITE64(map, reg_map[i][0], value);
				addr_save = READ64(map, reg_map[i][0]);
//				printf("\r0x%016llX",value);
				if(value != addr_save)
				{
					printf("\naddr==0x%04llX//write value=0x%016llx\tnow value==0x%016llX\n--> Not matched!!!!", 
						reg_map[i][0], value, addr_save);
				}
				last_two = reg_map[i][0];
				test_addr[t_two] = reg_map[i][0];
				t_two++;
			}
		}
//		printf("ADDR =====================\n");
//		for(i=0;i<t_two;i++)
//		{
//			printf("0x%04x,\t",test_addr[i]);
//		}
//		printf("\n=====================end\n");
//		printf("\n\nTest number = %d // LAST ADDR = 0x%04llx\n",
//			t_two, last_two);
		printf("\nTest 1-3\n");
		for(i=0;i<reg_size;i++)
		{
			if(i != 0)
			{
				if(reg_map[i][0] == 0)
				{
					continue;
				}
			}
			if(reg_map[i][1] == 2)
			{
				value = 0xAAAAAAAAAAAAAAAA;
				WRITE64(map, reg_map[i][0], value);
				addr_save = READ64(map, reg_map[i][0]);
//				printf("\r0x%016llX",value);
				if(value != addr_save)
				{
					printf("\naddr==0x%04llX//write value==0x%016llX\tnow value==0x%016llX\n--> Not matched!!!!",
						reg_map[i][0], value, addr_save);
				}
				last_three = reg_map[i][0];
				t_three++;
			}
		}
//    printf("ADDR =====================\n");
//		for(i=0;i<t_three;i++)
//    {
//	    printf("0x%04x,\t",test_addr[i]);
//    }
//    printf("\n=====================end\n");
//		printf("\n\nTest number = %d//LAST ADDR = 0x%04llx\n",t_three,last_three);
//	===========================================================
//	register map value initialization
//	===========================================================
		for(i=0;i<reg_size;i++)
		{
			if(reg_map[i][1] == 2)
			{
				WRITE64(map, reg_map[i][0], reg_map[i][2]);
			}
		}
//		while (1) {
//			WRITE64(map, addr, value);
//			printf("\r%016llx", value);
//			if (value != READ64(map, addr)) {
//				printf("\n--> Not matched!!!\n");
//			}
//			value++;
//		}
		printf("\n");
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

