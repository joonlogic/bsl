#ifndef _BSL_REGISTER_H_
#define _BSL_REGISTER_H_

#ifdef _PROD_GT_
/*************************************************************************/
/************************* PROD_GT START *********************************/
/*************************************************************************/
// BSL Registers
typedef enum {
	// DMA Registers
	C0AR = 0x000,	// Start Address of DMA Channel 0
	C0SR = 0x001,	// Control Register of DMA Channel 0
	C1AR = 0x002,	// Start Address of DMA Channel 1
	C1SR = 0x003,	// Control Register of DMA Channel 1
	C3AR = 0x004,	// Start Address of DMA Channel 3
	C3SR = 0x005,	// Control Register of DMA Channel 3
	C4AR = 0x006,	// Start Address of DMA Channel 4
	C4SR = 0x007,	// Control Register of DMA Channel 4

	VERR = 0x040,	// Version Register
	CERR = 0x042,	// Statics Counter Reset Register
	DBGR = 0x044,	// Debug Register
	SWRR = 0x046,	// SW Reset Register
	TIMR = 0x048,	// Time Register
	LRCR = 0x04A,	// Local Rate Limit Control Register --> Stage Selection

	// PORT0 Traffic Registers
	NPCR0 = 0x080,	// Normal Rx Packet Counter Register
	//EPCR0,	// Error Rx Packet Counter Register
	//TPCR0,	// Total Rx Packet Counter Register
	NBCR0 = 0x082,	// Normal Rx Byte Counter Register
	//EBCR0,	// Error Rx Byte Counter Register
	//TBCR0,	// Total Rx Byte Counter Register
	PS0R0 = 0x084,	// 64B ~ 127B Size Packet Counter Register
	PS1R0 = 0x086,	// 128B ~ 255B Size Packet Counter Register
	PS2R0 = 0x088,	// 256B ~ 511B Size Packet Counter Register
	PS3R0 = 0x08A,	// 512B ~ 1023B Size Packet Counter Register
	PS4R0 = 0x08C,	// 1024B ~ 1518B Size Packet Counter Register

	// PORT1 Traffic Registers
	NPCR1 = 0x0C0,  // Normal Rx Packet Counter Register
	//EPCR1,        // Error Rx Packet Counter Register
	//TPCR1,        // Total Rx Packet Counter Register
	NBCR1 = 0x0C2,  // Normal Rx Byte Counter Register
	//EBCR1,        // Error Rx Byte Counter Register
	//TBCR1,        // Total Rx Byte Counter Register
	PS0R1 = 0x0C4,  // 64B ~ 127B Size Packet Counter Register
	PS1R1 = 0x0C6,  // 128B ~ 255B Size Packet Counter Register
	PS2R1 = 0x0C8,  // 256B ~ 511B Size Packet Counter Register
	PS3R1 = 0x0CA,  // 512B ~ 1023B Size Packet Counter Register
	PS4R1 = 0x0CC,  // 1024B ~ 1518B Size Packet Counter Register


	// Etc. Packet Counter
	FINR = 0x100,	// FIN Flag Packet Counter Register
	FINO = 0x102,	// only FIN Flag Packet Counter Register
	SYNR = 0x104,	// SYN Packet Counter Register
	SYNO = 0x106,	// only SYN Flag Packet Counter Register
	RSTR = 0x108,	// RST Packet Counter Register
	RSTO = 0x10A,	// only RST Flag Packet Counter Register
	PSHR = 0x10C,	// PSH Packet Counter Register
	PSHO = 0x10E,	// only PSH Flag Packet Counter Register
	ACKR = 0x110,	// ACK Packet Counter Register (except SYN-ACK Pakcet)
	ACKO = 0x112,	// only ACK Flag Packet Counter Register
	URGR = 0x114,	// URG Packet Counter Register
	URGO = 0x116,	// only URG Flag Packet Counter Register
	SYAR = 0x118,	// SYN-ACK Packet Counter Register
	SYAO = 0x11A,	// only SYN-ACK Packet Counter Register
	IP4R = 0x11C,	// IPv4 Packet Counter Register
	ARPR = 0x11E,	// ARP Packet Counter Register
	ETC3 = 0x120,	// Layer3 Etc Packet Counter Register (except IP, ARP)
	TCPR = 0x122,	// TCP Packet Counter Register
	UDPR = 0x124,	// UDP Packet Counter Register
	ICMP = 0x126,	// ICMP Packet Counter Register
	IGMP = 0x128,	// IGMP Packet Counter Register
	ETC4 = 0x12A,	// Layer4 Etc Packet Counter Register (except TCP, UDP, ICMP, IGMP)

	// ACL
	ERWR = 0x140,
	EADR = 0x142,
	EWDR0 = 0x144,
	EWDR1 = 0x146,
	EWDR2 = 0x148,
	EWDR3 = 0x14A,
	ERDR0 = 0x14C,
	ERDR1 = 0x14E,
	ERDR2 = 0x150,
	ERDR3 = 0x152,
	EGDR = 0x154,	// Engine Gray Rule Delete Request Register

	// RATE-LIMIT --> useless
	//RRWR = 0x140,
	//RADR,
	//RCWR,
	//RCRR,
	//RWDR0,
	//RWDR1,
	//RRDR0,
	//RRDR1,
	//RRDR2,
	//RRDR3,

	DPBR = 0x180,	// Protocol Filter Set Register
	RHTR = 0x182,	// Gray Rule Holding Time Set Register
	ARTR = 0x184,	// ACK Response Time Register (Unit : Sec)
	SCER = 0x186,	// Syn-Cookie Function (0: disable, 1:enable)
	APMR = 0x188,	// ACL Rule Source, Destination Port Mask Register

	PPCR = 0x18A,	// Pass Packet Counter
	BPCR = 0x18C,	// Block Packet Counter
	RLCR = 0x18E,	// Rate Limit Indicate Register
	//HFPC,		// Hash Fail Packet Pass Counter
	//HFBC,		// Hash Fail Packet Block Counter
	//HFCR = 0x18E,	// Hash Fail Packet Counter
	//NIPC = 0x190,	// Non-IP Packet Pass Counter
	//NIBC = 0x192,	// Non-IP Packet Block Counter
	//TOBC,		// Time Over Block Packet Counter
	SPCR = 0x194,	// SYN Packet Counter
	SAPC = 0x196,	// SYN-ACK Packet Counter
	//APCR = 0x198,	// ACK Packet Counter
	RPCR = 0x19A,	// RST Packet Counter Register
	SFCR = 0x19C,	// SYN-Cookie Fail Packet Counter Register
	MTHR = 0x19E,	// Magic Number OK Result Register
	PFER = 0x1A0,	// Frame FIFO Empty Register

	VSIR0 = 0x1A2,	// VPN Server0 IP or VPN Server0 proxy IP
	VSIR1 = 0x1A4,	// VPN Server0 IP or VPN Server0 proxy IP

	DLSR = 0x1A2,	// Default Local Rate Limit Set Register
	DPSR = 0x1A4,	// Default Protocol Control Register
	DHSR0 = 0x1A6,	// Default Hole Rate Limit Set Register (TCP, Except for TCP)
	DHSR1 = 0x1A8,	// Default Hole Rate Limit Set Register (UDP)

	BYPR = 0x1C0,	// Bypass Control (0 : normal path, 1: bypass path)

	// MDIO Registers
	MRWR = 0x200,
	MCMR = 0x202,
	MRDR = 0x204,	// MDIO Read Data Register

	TTPCR0 = 0x240,	// Port0 Total Tx Packet Counter Register
	TNPCR0 = 0x242,	// Port0 Normal Tx Packet Counter Register
	TCPCR0 = 0x244,	// Port0 Syn-Cookie Tx Packet Counter Register
	TTBCR0 = 0x246,	// Port0 Total Tx Byte Counter Register
	TNBCR0 = 0x248,	// Port0 Normal Tx Byte Counter Register
	TCBCR0 = 0x24A,	// Port0 Syn-Cookie Tx Byte Counter Register

	TTPCR1 = 0x280,	// Port1 Total Tx Packet Counter Register
	TTBCR1 = 0x282,	// Port1 Total Tx Byte Counter Register

	DSAR = 0x2C0,	// DMA Start Address Register
	DCAR = 0x2C2,	// Current Memory Write Address Register
	DISR = 0x2C4,	// DMA Interrupt Size Register

	APPR = 0x300,	// Abnormal Packet Processing Register

	AS0R = 0x302,	// Abnormal Rule '0' Detected Packet Counter Register
	AS1R = 0x304,	// Abnormal Rule '1' Detected Packet Counter Register
	AS2R = 0x306,	// Abnormal Rule '2' Detected Packet Counter Register
	AS3R = 0x308,	// Abnormal Rule '3' Detected Packet Counter Register
	AS4R = 0x30A,	// Abnormal Rule '4' Detected Packet Counter Register
	AS5R = 0x30C,	// Abnormal Rule '5' Detected Packet Counter Register
	AS6R = 0x30E,	// Abnormal Rule '6' Detected Packet Counter Register
	AS7R = 0x310,	// Abnormal Rule '7' Detected Packet Counter Register
	AS8R = 0x312,	// Abnormal Rule '8' Detected Packet Counter Register
	AS9R = 0x314,	// Abnormal Rule '9' Detected Packet Counter Register
	AS10R = 0x316,	// Abnormal Rule '10' Detected Packet Counter Register
	AS11R = 0x318,	// Abnormal Rule '11' Detected Packet Counter Register
	AS12R = 0x31A,	// Abnormal Rule '12' Detected Packet Counter Register
	AS13R = 0x31C,	// Abnormal Rule '13' Detected Packet Counter Register
	AS14R = 0x31E,	// Abnormal Rule '14' Detected Packet Counter Register
	AS15R = 0x320,	// Abnormal Rule '15' Detected Packet Counter Register
	AS16R = 0x322,	// Abnormal Rule '16' Detected Packet Counter Register
	AS17R = 0x324,	// Abnormal Rule '17' Detected Packet Counter Register
	AS18R = 0x326,	// Abnormal Rule '18' Detected Packet Counter Register
	AS19R = 0x328,	// Abnormal Rule '19' Detected Packet Counter Register

	MMSCR = 0x340,	// Mask Control Register
	MMSAR = 0x342,	// a_ or ab_memory address Register
	MMWDR = 0x344,	// a_ or ab_memory Write Data Register
	MMRDR = 0x346,	// a_ or ab_memory Read Data Register

	// RATE-LIMIT (new)
	RWCR = 0x380,	// Rd/Wr Control Register
	ADDR = 0x382,	// Address Register
	CMWR = 0x384,	// CAM Write Data Register
	CMRR = 0x386,	// CAM Read Data Register
	WDR0 = 0x388,	// DPRAM(Global) Write Data Register ([54:31] : PPS, [30:0] : BPS)
	WDR1 = 0x38A,	// DPRAM(Local) Write Data Register ([54:31] : PPS, [30:0] : BPS)
	WDR2 = 0x38C,	// Protocol Block Information
	WDR3 = 0x38E,	// Hole(BPS) Write Data Register ([59:32] : TCP, [27:0] : Except of TCP/UDP)
	WDR4 = 0x390,	// Hole(BPS) Write Data Register ([27:0] : UDP)
	RDR0 = 0x392,	// DPRAM(Global Set) Read Data Register ([54:31] : PPS, [30:0] : BPS)
	RDR1 = 0x394,	// DPRAM(Local Set) Read Data Register ([54:31] : PPS, [30:0] : BPS)
	RDR2 = 0x396,	// DPRAM(Local Current) Read Data Register ([54:31] : PPS, [30:0] : BPS)
	RDR3 = 0x398,	// Timer Read Register ([25:22] : Month, [21:17] : Day, [16:12] : Hour, [11:6] : Minute, [5:0] : Second)
	RDR4 = 0x39A,	// Hole (Except for TCP/UDP) Read Data Register ([47:24] : Current BPS, [23:0] : Set BPS)
	RDR5 = 0x39C,	// Hole (TCP) Read Data Register ([47:24] : Current BPS, [23:0] : Set BPS)
	RDR6 = 0x39E,	// Hole (UDP) Read Data Register ([47:24] : Current BPS, [23:0] : Set BPS)
	RDR7 = 0x3A0,	// Hole (Time) Read Data Register ([57:54] : TCP Month, [53:49] : TCP Day, [48:44] : TCP Hour, [43:38] : TCP Minute, [37:32] : TCP Second, [25:22] : Except for TCP/UDP Month, [21:17] : Except for TCP/UDP Day, [16:12] : Except for TCP/UDP Hour, [11:6] : Except for TCP/UDP Minute, [5:0] : Except for TCP/UDP Second)
	RDR8 = 0x3A2,	// Hole (Time) Read Data Register ([25:22] : TCP Month, [21:17] : TCP Day, [16:12] : TCP Hour, [11:5] : TCP Minute, [5:0] : TCP Second)

	RTPCR = 0x3C0,	// Rx TCP Packet Counter Register
	RUPCR = 0x3C2,	// Rx UDP Packet Counter Register
	RIPCR = 0x3C4,	// Rx ICMP Packet Counter Register
	REPCR = 0x3C6,	// Rx Etc Packet Counter Register
	TTPCR = 0x3C8,	// Tx TCP Packet Counter Register
	TUPCR = 0x3CA,	// Tx UDP Packet Counter Register
	TIPCR = 0x3CC,	// Tx ICMP Packet Counter Register
	TEPCR = 0x3CE,	// Tx Etc Packet Counter Register
} bsl_register_t;


/*************************************************************************/
/************************* PROD_GT FINISH ********************************/
/*************************************************************************/
#else //__PROD_BSL_ not _PROD_GT_
/*************************************************************************/
/************************* PROD_BSL START ********************************/
/*************************************************************************/
// BSL Registers
typedef enum {
	// DMA Registers
	C0AR = 0x000,	// Start Address of DMA Channel 0
	C0SR = 0x001,	// Control Register of DMA Channel 0
	C3AR = 0x006,	// Start Address of DMA Channel 3
	C3SR = 0x007,	// Control Register of DMA Channel 3

	VERR = 0x040,	// Version Register
	SWRR = 0x042,	// SW Reset Register
	SRCR = 0x044,	// Stats Reset Register

//	CERR = 0x042,	// Statics Counter Reset Register

	FSCR0 = 0x046,  // Port0 Frames Sent Count Register
	FSRR0 = 0x048,  // Port0 Frames Sent Rate Register
	BSCR0 = 0x04A,  // Port0 Bytes Sent Count Register
	BSRR0 = 0x04C,  // Port0 Bytes Sent Rate Register

	FSCR1 = 0x04E,  // Port1 Frames Sent Count Register
	FSRR1 = 0x050,  // Port1 Frames Sent Rate Register
	BSCR1 = 0x052,  // Port1 Bytes Sent Count Register
	BSRR1 = 0x054,  // Port1 Bytes Sent Rate Register

	LSR0  = 0x056,	// Port0 Link Status Register
	LSR1  = 0x058,	// Port1 Link Status Register

	WMCR  = 0x05A,	// Working Mode Control Register

	LBSCR = 0x05C,	// Working Mode Control Register

	DBGR  = 0x05E,	// Debugging Register

	// MDIO Registers
	MRWR  = 0x080,
	MCMR  = 0x082,
	MRDR  = 0x084,	// MDIO Read Data Register

	// Stream Controller 0
	CNTR  = 0x0C0,   // Control Register
	TIMR  = 0x0C2,   // Time Set Register
	PECR  = 0x0C4,   // Port Enable Control Register
	PMSR  = 0x0C6,   // Port Mode Set Register
	SECR  = 0x0C8,   // Port Stream Enable Control Register

	NMSR  = 0x0D0,   // Network Mode Set Register
	MASR  = 0x0D2,   // Mac Address Set Register
	IPSR  = 0x0D4,   // IP Address Set Register
          
	// Pkt_gen 0
	PGCR  = 0x100,   // Packet Gen Control Register
	SMCR  = 0x102,   // Stream Mode Control Register
	PACR  = 0x104,   // Protocol Assign Control Register
	BGCR  = 0x106,   // Bae Gap Counter Register
	IBGR  = 0x108,   // Inter Burst Gap Register
	ISGR  = 0x10A,   // Inter Stream Gap Register
	STDR  = 0x10C,   // Start Tx Delay Register

	PHIR0 = 0x10E,  // Packet Header Information Register
	PHIR1 = 0x110,  // Packet Header Information Register
	PHIR2 = 0x112,  // Packet Header Information Register
	PHIR3 = 0x114,  // Packet Header Information Register
	PHIR4 = 0x116,  // Packet Header Information Register
	PHIR5 = 0x118,  // Packet Header Information Register
	PHIR6 = 0x11A,  // Packet Header Information Register
	PHIR7 = 0x11C,  // Packet Header Information Register
	PHIR8 = 0x11E,  // Packet Header Information Register
	PHIR9 = 0x120,  // Packet Header Information Register
	PHIR10 = 0x122,  // Packet Header Information Register
	PHIR11 = 0x124,  // Packet Header Information Register
	PHIR12 = 0x126,  // Packet Header Information Register
	PHIR13 = 0x128,  // Packet Header Information Register
	PHIR14 = 0x12A,  // Packet Header Information Register
	PHIR15 = 0x12C,  // Packet Header Information Register
	PHIR16 = 0x12E,  // Packet Header Information Register
	PHIR17 = 0x130,  // Packet Header Information Register
	PHIR18 = 0x132,  // Packet Header Information Register
	PHIR19 = 0x134,  // Packet Header Information Register
	PHIR20 = 0x136,  // Packet Header Information Register
	PHIR21 = 0x138,  // Packet Header Information Register
	PHIR22 = 0x13A,  // Packet Header Information Register
	PHIR23 = 0x13C,  // Packet Header Information Register
	PHIR24 = 0x13E,  // Packet Header Information Register

	PPIR  = 0x140,    // Packet Payload Information Register

	PRSR0 = 0x142,   // Payload Random Size Register
	PRSR1 = 0x144,   // Payload Random Size Register
	PRSR2 = 0x146,   // Payload Random Size Register
	PRSR3 = 0x148,   // Payload Random Size Register
	PRSR4 = 0x14A,   // Payload Random Size Register
	PRSR5 = 0x14C,   // Payload Random Size Register
	PRSR6 = 0x14E,   // Payload Random Size Register
	PVDR0 = 0x150,   // Payload Random Value Difference Register
	PVDR1 = 0x152,   // Payload Random Value Difference Register
	PVDR2 = 0x154,   // Payload Random Value Difference Register
	PVDR3 = 0x156,   // Payload Random Value Difference Register
	PVDR4 = 0x158,   // Payload Random Value Difference Register
	PVDR5 = 0x15A,   // Payload Random Value Difference Register
	PVDR6 = 0x15C,   // Payload Random Value Difference Register
	PVDR7 = 0x15E,   // Payload Random Value Difference Register
	PVDR8 = 0x160,   // Payload Random Value Difference Register
	PVDR9 = 0x162,   // Payload Random Value Difference Register
	PVDR10 = 0x164,  // Payload Random Value Difference Register

	PIDR = 0x166,    // Payload Increase Size Difference Register
	PBCR = 0x168,    // Packets per Burst Count Set Register
	SCSR = 0x16A,    // Stream Control Set Register

	DMSR = 0x16C,    // DMAC Start Address Register
	DMLR = 0x16E,    // DMAC Last Address Register
	SMSR = 0x170,    // SMAC Start Address Register
	SMLR = 0x172,    // SMAC Last Address Register

	SI6SR0 = 0x174,  // SIPv6 Start Address Register
	SI6SR1 = 0x176,  // SIPv6 Start Address Register
	SI6SR2 = 0x178,  // SIPv6 Start Address Register
	SI6LR0 = 0x17A,  // SIPv6 Last Address Register
	SI6LR1 = 0x17C,  // SIPv6 Last Address Register
	SI6OR0 = 0x17E,  // SIPv6 Last Offset Register
	SI6OR1 = 0x180,  // SIPv6 Last Offset Register

	DI6SR0 = 0x182,  // DIPv6 Start Address Register
	DI6SR1 = 0x184,  // DIPv6 Start Address Register
	DI6SR2 = 0x186,  // DIPv6 Start Address Register
	DI6LR0 = 0x188,  // DIPv6 Last Address Register
	DI6LR1 = 0x18A,  // DIPv6 Last Address Register
	DI6OR0 = 0x18C,  // DIPv6 Last Offset Register
	DI6OR1 = 0x18E,  // DIPv6 Last Offset Register

	SI4SR = 0x190,   // SIPv4 Start Address Register
	SI4LR = 0x192,   // SIPv4 Last Address Register
	SI4OR = 0x194,   // SIPv4 Last Offset Register
	DI4SR = 0x196,   // DIPv4 Start Address Register
	DI4LR = 0x198,   // DIPv4 Last Address Register
	DI4OR = 0x19A,   // DIPv4 Last Offset Register

	SPVR = 0x19C,    // SP Value Register
	DPVR = 0x19E,    // DP Value Register

	PSAR = 0x1A0,    // Port Start Address Register
	PDWR = 0x1A2,    // Packet/Payload Download Register
	PDCR = 0x1A4,    // Packet/Payload Download Confirm Register
	CSVR = 0x1A6,    // L3/L4 Checksum Start Value Register

	DMACR = 0x1A8,    // L3/L4 Checksum Start Value Register
	DIPR  = 0x1AA,    // L3/L4 Checksum Start Value Register
	SNCSR = 0x1AC,    // L3/L4 Checksum Start Value Register
	IDVR = 0x1AE,    // L3/L4 Checksum Start Value Register								// 2017.2.27 by dgjung

	// Rx_0
	RFCR  = 0x1C0,    // Normal(not CRC and not Fragment) Frame Count Register
	RFRR  = 0x1C2,    // Normal(not CRC and not Fragment) Frame Rate Register
	CEFC  = 0x1C4,    // CRC Error Frame Count Register
	CEFR  = 0x1C6,    // CRC Error Frame Rate Register
	FEFC  = 0x1C8,    // Fragment Error Frame Count Register
	FEFR  = 0x1CA,    // Fragment Error Frame Rate Register
	RBCR  = 0x1CC,    // Normal(not CRC and not Fragment) Byte Count Register
	RBRR  = 0x1CE,    // Normal(not CRC and not Fragment) Byte Rate Register
	CEBC  = 0x1D0,    // CRC Error Byte Count Register
	CEBR  = 0x1D2,    // CRC Error Byte Rate Register
	FEBC  = 0x1D4,    // Fragment Error Byte Count Register
	FEBR  = 0x1D6,    // Fragment Error Byte Rate Register
	VFCR  = 0x1D8,     // VLAN Frame Count Register
	VBCR  = 0x1DA,     // VLAN Byte Count Register
	CCFR  = 0x1DC,     // Capture configuration Register
	MMLR  = 0x1DE,	  // Maximum / Minimum Latency Value Register
	TLVR  = 0x1E0,	  // Total Latency Value Register
	TSCR  = 0x1E2,	  // Total Packet Count Register
	SPCR  = 0x1E4,	  // Sequence Error Packet Count Register
	UFCR  = 0x1E6,	  // Undersize Frame Count Register
	OFCR  = 0x1E8,	  // Oversize Frame Count Register
	UEFCR = 0x1EA,	  // Oversize Frame Count Register
	PSMR  = 0x1EC,	  // Oversize Frame Count Register
	PSIR  = 0x1EE,	  // Oversize Frame Count Register
	SFCR  = 0x1F0,	  // Oversize Frame Count Register										// 2016.12.31 by dgjung
} bsl_register_t;

#define OFFSET_REGISTER_PORT( PORTID )      0x140*(PORTID)
#define OFFSET_REGISTER_PORT64( PORTID )    ((0x140*(PORTID))<<3)
#define BSL_HZ                              0x0000000009502F90L

#define PRSR(OFFSET)                        (PRSR0+(OFFSET)*2)
#define PVDR(OFFSET)                        (PVDR0+(OFFSET)*2)
#define PHIR(OFFSET)                        (PHIR0+(OFFSET)*2)

typedef enum {
	ValueCNTRDefault = 0,
	ValueCNTRStop = 1,
	ValueCNTRStart = 2,
	ValueCNTRPause = 3,
	ValueCNTRSingleStep = 4
} EnumValueCNTR;


//I_ means offset bit
//M_ means mask for valid bits
#define I_CnSR_ABORT                        35
#define I_CnSR_SGMODE                       36

#define I_SCSR_RETURN_TO_ID                 56
#define I_SCSR_LOOP_COUNT                   32
#define M_SCSR_LOOP_COUNT                   ((1<<24)-1)
#define M_SCSR_BURST_PER_STREAM             (((unsigned long long)1<<32)-1)

#define I_PPIR_FRAMESIZE_TYPE               60
#define I_PPIR_PATTERN_SIZE                 56
#define M_PPIR_PATTERN_SIZE                 ((1<<3)-1)
#define I_PPIR_PAYLOAD_PATTERN_TYPE         0
#define I_PPIR_PAYLOAD_PATTERN_VALUE        8

#define I_PGCR_STREAM_NUMBER                8
#define M_PGCR_STREAM_NUMBER                ((1<<8)-1)
#define I_PGCR_GROUP_NUMBER                 16						// 2017.7.15 by dgjung
#define M_PGCR_GROUP_NUMBER                 ((1<<4)-1)					// 2017.7.15 by dgjung
#define I_PGCR_FRAMESIZE                    24
#define M_PGCR_FRAMESIZE                    ((1<<16)-1)

#define I_PRSR_FSIZE_RANDOM_0               0
#define I_PRSR_FSIZE_RANDOM_1               11
#define I_PRSR_FSIZE_RANDOM_2               22
#define I_PRSR_FSIZE_RANDOM_3               33
#define I_PRSR_FSIZE_RANDOM_4               44
#define M_PRSR_FSIZE_RANDOM                 ((1<<11)-1)

#define I_PIDR_LAST_SIZE                    48
#define I_PIDR_SIZE_DIFF                    32
#define M_PIDR_LAST_SIZE                    ((1<<15)-1)
#define M_PIDR_SIZE_DIFF                    ((1<<11)-1)

#define I_PVDR_DATA_RANDOM_0                34
#define I_PVDR_DATA_RANDOM_1                17
#define I_PVDR_DATA_RANDOM_2                0
#define M_PVDR_VALUE                        ((1<<17)-1)

#define NUM_FRAMESIZE_RANDOM                32
#define NUM_PAYLOAD_VALUE_RANDOM            32
#define NUM_PHIR                            25

#define I_PACR_HEADSIZE                     32
#define I_PACR_DMAC                         4
#define I_PACR_SMAC                         5
#define I_PACR_IP6_SIP                      6
#define I_PACR_IP6_DIP                      7
#define I_PACR_IP4_SIP                      8
#define I_PACR_IP4_DIP                      9
#define I_PACR_SPORT                        10
#define I_PACR_DPORT                        11
#define I_PACR_L3_CHKSUM                    12
#define I_PACR_L4_CHKSUM                    13
#define I_PACR_IS_IP4                       14
#define I_PACR_IS_TCP                       15
#define I_PACR_IS_UDP                       16
#define I_PACR_IS_IP4_IP6                   17
#define I_PACR_IS_IP6_IP4                   18
#define M_PACR_HEADSIZE                     ((1<<8)-1)

#define I_DMSR_OFFSET                       48
#define M_DMSR_OFFSET                       ((1<<16)-1)

#define I_DMLR_DISTANCE                     56
#define M_DMLR_DISTANCE                     ((1<<8)-1)
#define I_DMLR_MARK_RANDOM                  53
#define I_DMLR_MARK_CHANGE                  52
#define I_DMLR_MARK_CONTINUE                51
#define I_DMLR_MARK_DECREMENT               48
#define M_DMLR_CRC                          ((1<<2)-1)
#define I_DMLR_CRC                          49

#define I_SMSR_OFFSET                       I_DMSR_OFFSET                       
#define M_SMSR_OFFSET                       M_DMSR_OFFSET                       

#define I_SMLR_DISTANCE                     I_DMLR_DISTANCE                     
#define M_SMLR_DISTANCE                     M_DMLR_DISTANCE                     
#define I_SMLR_MARK_RANDOM                  I_DMLR_MARK_RANDOM                  
#define I_SMLR_MARK_CHANGE                  I_DMLR_MARK_CHANGE
#define I_SMLR_MARK_CONTINUE                I_DMLR_MARK_CONTINUE                
#define I_SMLR_MARK_DECREMENT               I_DMLR_MARK_DECREMENT               

#define M_SI6SR_DISTANCE                    ((1<<8)-1)
#define I_SI6SR_DISTANCE                    48
#define M_SI6SR_MASK                        ((1<<7)-1)
#define I_SI6SR_MASK                        32
#define I_SI6SR_MARK_CHANGE                 25
#define I_SI6SR_MARK_NETWORK                24
#define I_SI6SR_MARK_SIGN                   16
#define M_SI6SR_OFFSET                      ((1<<16)-1)
#define I_SI6SR_OFFSET                      0

#define M_DI6SR_PAYLOADLEN                  ((1<<8)-1)
#define I_DI6SR_PAYLOADLEN                  40
#define M_DI6SR_MASK                        M_SI6SR_MASK
#define I_DI6SR_MASK                        I_SI6SR_MASK
#define I_DI6SR_MARK_CHANGE                 I_SI6SR_MARK_CHANGE
#define I_DI6SR_MARK_NETWORK                I_SI6SR_MARK_NETWORK
#define I_DI6SR_MARK_SIGN                   I_SI6SR_MARK_SIGN
#define M_DI6SR_OFFSET                      M_SI6SR_OFFSET
#define I_DI6SR_OFFSET                      I_SI6SR_OFFSET

#define M_SI4SR_CHKSUM_DISTANCE             ((1<<8)-1)
#define I_SI4SR_CHKSUM_DISTANCE             56
#define I_SI4SR_CLASS_SEL                   52
#define I_SI4SR_MARK_RANDOM                 51
#define I_SI4SR_MARK_CHANGE                 50
#define I_SI4SR_MARK_CLASS                  49
#define I_SI4SR_MARK_OFFSET_SIGN            48
#define I_SI4SR_MARK_CONTINUE               40
#define I_SI4SR_DISTANCE                    32
#define M_SI4SR_DISTANCE                    ((1<<8)-1)

#define M_SI4LR_IP_DISTANCE                 ((1<<8)-1)
#define I_SI4LR_IP_DISTANCE                 32
#define I_SI4LR_IP_MASK                     32

//#define M_SI4OR_TLEN_DISTANCE               ((1<<16)-1)
//#define I_SI4OR_TLEN_DISTANCE               48

// 2017.4.3 by dgjung//
//#define M_SI4OR_TLEN_CURRENT                ((1<<16)-1)
//#define I_SI4OR_TLEN_CURRENT                48
#define M_SI4OR_ID_START	                ((1<<16)-1)
#define I_SI4OR_ID_START	                48

#define M_SI4OR_TLEN_START                  ((1<<16)-1)                 
#define I_SI4OR_TLEN_START                  32
#define M_SI4OR_LAST_OFFSET                 ((1<<32)-1)                 
#define I_SI4OR_LAST_OFFSET                 0

#define M_DI4OR_TLEN_DISTANCE               M_SI4OR_TLEN_DISTANCE
#define I_DI4OR_TLEN_DISTANCE               I_SI4OR_TLEN_DISTANCE
#define M_DI4OR_TLEN_START                  M_SI4OR_TLEN_START
#define I_DI4OR_TLEN_START                  I_SI4OR_TLEN_START
#define M_DI4OR_LAST_OFFSET                 M_SI4OR_LAST_OFFSET
#define I_DI4OR_LAST_OFFSET                 I_SI4OR_LAST_OFFSET
#define M_DI4OR_UDP_TLEN                    ((1<<16)-1)
#define I_DI4OR_UDP_TLEN                    32

#define M_DI4SR_CHKSUM_DISTANCE             M_SI4SR_CHKSUM_DISTANCE             
#define I_DI4SR_CHKSUM_DISTANCE             I_SI4SR_CHKSUM_DISTANCE             
#define I_DI4SR_CLASS_SEL                   I_SI4SR_CLASS_SEL                   
#define I_DI4SR_MARK_RANDOM                 I_SI4SR_MARK_RANDOM                 
#define I_DI4SR_MARK_CHANGE                 I_SI4SR_MARK_CHANGE                 
#define I_DI4SR_MARK_CONTINUE               I_SI4SR_MARK_CONTINUE               
#define I_DI4SR_MARK_CLASS                  I_SI4SR_MARK_CLASS                  
#define I_DI4SR_MARK_OFFSET_SIGN            I_SI4SR_MARK_OFFSET_SIGN            
#define I_DI4SR_OFFSET_VALUE                I_SI4SR_OFFSET_VALUE                
#define M_DI4SR_OFFSET_VALUE                M_SI4SR_OFFSET_VALUE                

#define M_CSVR_L3_START_VALUE               ((1<<16)-1)
#define I_CSVR_L3_START_VALUE               0
#define M_CSVR_L4_START_VALUE               ((1<<16)-1)
#define I_CSVR_L4_START_VALUE               32

#define M_SPVR_PORT_DISTANCE                ((1<<8)-1)						// 2017.4.3 by dgjung
#define I_SPVR_PORT_DISTANCE                56
#define I_SPVR_MARK_CHANGE                  54
#define I_SPVR_MARK_RANDOM                  53
#define I_SPVR_MARK_CONTINUE                52
#define I_SPVR_MARK_SIGN                    48
#define M_SPVR_OFFSET_VALUE                 ((1<<16)-1)
#define I_SPVR_OFFSET_VALUE                 32
#define M_SPVR_LAST_OFFSET                  ((1<<16)-1)
#define I_SPVR_LAST_OFFSET                  16
#define M_SPVR_LAST_VALUE                   ((1<<16)-1)
#define I_SPVR_LAST_VALUE                   0

#define M_DPVR_CHKSUM_DISTANCE              ((1<<8)-1)
#define I_DPVR_CHKSUM_DISTANCE              56
#define I_DPVR_MARK_CHANGE                  I_SPVR_MARK_CHANGE
#define I_DPVR_MARK_RANDOM                  I_SPVR_MARK_RANDOM                  
#define I_DPVR_MARK_CONTINUE                I_SPVR_MARK_CONTINUE                
#define I_DPVR_MARK_SIGN                    I_SPVR_MARK_SIGN                    
#define M_DPVR_OFFSET_VALUE                 M_SPVR_OFFSET_VALUE            
#define I_DPVR_OFFSET_VALUE                 I_SPVR_OFFSET_VALUE            
#define M_DPVR_LAST_OFFSET                  M_SPVR_LAST_OFFSET                  
#define I_DPVR_LAST_OFFSET                  I_SPVR_LAST_OFFSET                  
#define M_DPVR_LAST_VALUE                   M_SPVR_LAST_VALUE              
#define I_DPVR_LAST_VALUE                   I_SPVR_LAST_VALUE              

#define M_PSAR_DPORT_START_VALUE            ((1<<16)-1)
#define I_PSAR_DPORT_START_VALUE            32
#define M_PSAR_SPORT_START_VALUE            ((1<<16)-1)
#define I_PSAR_SPORT_START_VALUE            0
// 2017.2.27 by dgjung
#define M_IDVR_DISTANCE						M_SPVR_PORT_DISTANCE
#define I_IDVR_DISTANCE						I_SPVR_PORT_DISTANCE
#define I_IDVR_MARK_CHANGE                  I_SPVR_MARK_CHANGE
#define I_IDVR_MARK_RANDOM                  I_SPVR_MARK_RANDOM  
#define I_IDVR_MARK_CONTINUE                I_SPVR_MARK_CONTINUE
#define I_IDVR_MARK_SIGN                    I_SPVR_MARK_SIGN    
#define M_IDVR_OFFSET_VALUE                 M_SPVR_OFFSET_VALUE 
#define I_IDVR_OFFSET_VALUE                 I_SPVR_OFFSET_VALUE 
#define M_IDVR_LAST_OFFSET                  M_SPVR_LAST_OFFSET  
#define I_IDVR_LAST_OFFSET                  I_SPVR_LAST_OFFSET  
#define M_IDVR_LAST_VALUE                   M_SPVR_LAST_VALUE   
#define I_IDVR_LAST_VALUE                   I_SPVR_LAST_VALUE   
/*************************************************************************/
/************************* PROD_BSL FINISH *******************************/
/*************************************************************************/
#endif  //!_PROD_GT_

// ACL Control
#define ERWR_SODIMM_RESET_FLAG	(1 << 2)
#define ERWR_READ_FLAG			(0 << 1)
#define ERWR_WRITE_FLAG			(1 << 1)
#define ERWR_SW_REQUEST_FLAG	(1 << 0)

// ACL mask
#define MMSCR_SW_REQUEST          (0x01)
#define MMSCR_READ                ((0x00) << 1)
#define MMSCR_WRITE               ((0x01) << 1) 
#define MMSCR_A_MEMORY_RESET      ((0x02) << 1)
#define MMSCR_AB_MEMORY_RESET     ((0x03) << 1)

// Rate-limit Control
#define RWCR_CAM_READ_FLAG						(0x0 << 1)
#define RWCR_CAM_WRITE_FLAG						(0x1 << 1)
#define RWCR_HOLE_CAM_READ_FLAG					(0x2 << 1)
#define RWCR_HOLE_CAM_WRITE_FLAG				(0x3 << 1)
#define RWCR_DPRAM_READ_FLAG					(0x4 << 1)
#define RWCR_DPRAM_WRITE_FLAG					(0x5 << 1)
#define RWCR_DPRAM_HOLE_READ_FLAG				(0x6 << 1)
#define RWCR_DPRAM_HOLE_WRITE_FLAG				(0x7 << 1)
#define RWCR_DPRAM_LOCAL_RESET_FLAG				(0x8 << 1)
#define RWCR_DPRAM_GLOBAL_RESET_FLAG			(0x9 << 1)
#define RWCR_DPRAM_PROTOCOL_RESET_FLAG			(0xA << 1)
#define RWCR_DPRAM_CAM_RESET_FLAG				(0xB << 1)
#define RWCR_HOLE_EXCEPT_OF_TCPUDP_RESET_FLAG	(0xC << 1)
#define RWCR_HOLE_TCP_RESET_FLAG				(0xD << 1)
#define RWCR_HOLE_UDP_RESET_FLAG				(0xE << 1)
#define RWCR_HOLE_CAM_RESET_FLAG				(0xF << 1)
#define RWCR_DPRAM_GLOBAL_WRITE_FLAG			(0x10 << 1)
#define RWCR_DPRAM_LOCAL_WRITE_FLAG				(0x11 << 1)
#define RWCR_DPRAM_PROTOCOL_WRITE_FLAG			(0x12 << 1)
#define RWCR_HOLE_EXCEPT_OF_TCPUDP_WRITE_FLAG	(0x13 << 1)
#define RWCR_HOLE_TCP_WRITE_FLAG				(0x14 << 1)
#define RWCR_HOLE_UDP_WRITE_FLAG				(0x15 << 1)
#define RWCR_SW_REQUEST_FLAG					(1 << 0)

// MDIO Control
#define MRWR_WRITE_FLAG						(1 << 1)
#define MRWR_READ_FLAG						(0 << 1)
#define MRWR_SW_REQUEST_FLAG				(1 << 0)

#endif //_BSL_REGISTER_H_
