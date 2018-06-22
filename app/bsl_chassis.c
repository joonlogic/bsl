#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>

#include "bsl_type.h"
#include "bsl_system.h"
#include "bsl_api.h"

const static char* this_version = "20150530";
extern char* bsl_getErrorStr( EnumResultCode );

void help(char *progname)
{
	printf("%s Chassis status check program.\n", progname );
	printf("      -h : Help\n");
	printf("      -v : Version\n");
}

int main(int argc, char *argv[])
{
	int opt, opt_ok = 0;
	int opt_version = 0;
	int isdaemon = 0;
	int fd, ret;
	char *map;
	int i;
	int dump, wr, rd, val;	// options
	unsigned int addr, value;
	int cardid=0;
	T_Chassis chassis={0,};
	int nCards=0;

	opt_ok = 1;
	dump = wr = rd = val = 0;

	while ((opt = getopt(argc, argv, "hv:")) != -1) {
		switch (opt) {
			case 'h':
				opt_ok = 0;
				break;
			case 'v':
				opt_ok = 1;
				opt_version = 1;
				break;
		}
	}

	if (opt_ok != 1) {
		help(argv[0]);
		exit(0);
	}

	if( opt_version ) {
		fprintf( stderr, "%s : Version %s\n", argv[0], this_version );
		return 0;
	}

	ret = bsl_getNumberOfCards( &nCards );
	if( ret != 0 ) {
		fprintf( stderr, "bsl_getNumberOfCards() ERROR, ret %d\n", ret );
		return 0;
	}

	ret = bsl_getChassis( &chassis );
	if( ret != 0 ) {
		fprintf( stderr, "bsl_getChassis() ERROR, ret %d\n", ret );
		return 0;
	}

	fprintf( stderr, "\n\n Total %d card%s installed \n", nCards, nCards>1?"(s)":"" );
	fprintf( stderr, "==============================================\n");
	if( chassis.status != ChassisStatusActive ) {
		fprintf( stderr, "Chassis Status is not active\n" );
		return 0;
	}
	
	for( i=0; i<nCards; i++ ) {
		if( chassis.card[i].status != CardStatusActive ) continue;

		fprintf( stderr, "Card %d information \n", i );
		int j=0;
		for( j=0; j<2; j++ ) {
			fprintf( stderr, "\tPort %d %s \n", j, 
				chassis.card[i].port[j].status == PortStatusActive ? "Active" : "Inactive");
			fprintf( stderr, "\tPort %d %s \n", j, 
				chassis.card[i].port[j].linkType == LinkType10GE ? "10GE" : "Unknown Link Type");
			fprintf( stderr, "\tPort %d %s \n", j, 
				chassis.card[i].port[j].link.operState == LinkUp ? "Up" : "Down");
			fprintf( stderr, "\n" );
		}
	}

	return 0;
}

