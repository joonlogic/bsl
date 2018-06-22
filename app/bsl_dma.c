typedef struct dmadescr
{
	unsigned long* page;
	unsigned int pagesize;
	unsigned long* next;
} __attribute__((packed)) DmaDescr_t;

#define BSL_CHECK_IOCTL_GOTO(ret_ioctl, where) \
	if( ret_ioctl < 0 ) { \
		fprintf(stderr, "%s:%s:%d BSL_CHECK_IOCTL_GOTO ret_ioctl %d\n", \
					__FILE__, __func__, __LINE__, ret_ioctl ); \
		goto where; \
	}

#define BSL_CHECK_NULL_GOTO(p, where) \
	if( p == 0 ) { \
		fprintf(stderr, "%s:%s:%d BSL_CHECK_IOCTL_GOTO ret_ioctl %d\n", \
					__FILE__, __func__, __LINE__, p ); \
		goto where; \
	}

int main(void)
{
	int fd = 0;
	int cardid = 0;
	int ret = 0;
	int descrcount = 0;

	ioc_io_buffer_t pciBuffer = {0,};
	ioc_io_buffer_t descrBuffer = {0,};
	void* pciUserp = 0;
	void* descrUserp = 0;
	DmaDescr_t* descrp = 0;

	const unsigned int SIZEOF_DMA_BUFFER = 0x20000000; //512M
	const unsigned int SIZEOF_DMA_PAGE = 0x400000; //4M

	// 
	fd = OPEN_DEVICE( cardid );
	BSL_CHECK_DEVICE( fd );

	ret = dommap( fd, &map );
	BSL_CHECK_RESULT( ret, ret );

	//buffer allocation for PCI memory
	pciBuffer.Size = SIZEOF_DMA_BUFFER;
	ret = ioctl( fd, CMD_DMA_MEM_ALLOC, &pciBuffer );
	BSL_CHECK_IOCTL_GOTO( ret, close_fd );

	//buffer mapping to user space for PCI memory
	pciUserp = mmap(0, pciBuffer.Size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, pciBuffer.CpuPhysical);
	BSL_CHECK_NULL_GOTO( pciUserp, close_fd );
	memset( pciUserp, 0, pciBuffer.Size );

	printf("UAddr(%llx) PAddr(%llx) VAddr(%llx) Size(%d)\n",
		pciUserp,
		pciBuffer.PhysicalAddr,
		pciBuffer.CpuPhysical,
		pciBuffer.Size );


	printf("[[[[Debug]]]]] sizeof(DmaDescr_t) = %d\n", sizeof(DmaDescr_t) );
	descrcount = pciBuffer.Size / SIZEOF_DMA_PAGE;
	descrBuffer.Size = descrcount * sizeof( DmaDescr_t );

	//buffer allocation for descriptor memory
	ret = ioctl( fd, CMD_DMA_MEM_ALLOC, &descrBuffer );
	BSL_CHECK_IOCTL_GOTO( ret, close_fd );

	//buffer mapping to user space for PCI memory
	descrUserp = mmap(0, descrBuffer.Size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, descrBuffer.CpuPhysical);
	BSL_CHECK_NULL_GOTO( descrUserp, close_fd );
	memset( descrUserp, 0, descrBuffer.Size );

	printf("UAddr(%llx) PAddr(%llx) VAddr(%llx) Size(%d)\n",
		descrUserp,
		descrBuffer.PhysicalAddr,
		descrBuffer.CpuPhysical,
		descrBuffer.Size );

	descrp = (DmaDescr_t*)descrUserp;

	for( i=0; i<descrcount; i++, descrp++ )
	{
		descrp->page = pciBuffer.PhysicalAddr + SIZEOF_DMA_PAGE*i;
		descrp->pagesize = SIZEOF_DMA_PAGE;
		descrp->next = descrBuffer.PhysicalAddr + ( sizeof( DmaDescr_t ) * ( i + 1 ) );
	}
	descrp->next = descrBuffer.PhysicalAddr; // Last Descriptor // don't care

	/*
	 * Now Register Setting
	 */

	//Abort DMA
//	WRITE64( 0, C0SR, 1ULL<<I_CXSR_ABORT );

	WRITE64( map, C0AR, descrBuffer.PhysicalAddr );
	unsigned long long cxsr = 0ULL;
	cxsr = SIZEOF_DMA_BUFFER;
	cxsr |= ( 1ULL << I_CXSR_RUN ) | ( 1ULL<<I_CXSR_SGMODE );

	WRITE64( map, C0SR, SIZEOF_DMA_BUFFER );

	while(1) {
		int key = 0;
		printf("\nPRESS ANY Key ( For quit 'q' ) ------------------\n");
		fflush(stdout);

		key = Plx_getch();
		if( key == 'q' ) break;

		printf("C0SR : %llx\n", READ64( map, C0SR ) );
	}

	ioctl( fd, CMD_DMA_MEM_FREE, &descrBuffer );
	ioctl( fd, CMD_DMA_MEM_FREE, &pciBuffer );
	munmap( descrUserp, descrBuffer.Size );
	munmap( pciUserp, pciBuffer.Size );

close_fd:
	domunmap( fd, &map );
	close( fd );

}
