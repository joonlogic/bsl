/********************************************************************
 *  FILE   : bsl_proto.h
 *  Author : joon
 *
 *  Library : 
 *  Export Function :
 *
 ********************************************************************
 *                    Change Log
 *
 *    Date			Author			Note
 *  -----------   -----------  ----------------------------------   
 *  2013.11.13       joon        This File is written first.
 *								         
 ********************************************************************/
#ifndef BSL_PROTO_H
#define BSL_PROTO_H

#include "common.h"
#include "bsl_def.h"
#include "bsl_system.h"

#pragma pack(1)

#define SIZE_PREAMBLE                      8        
#define SIZE_ETHER_ADDR                    6        
#define SIZE_ETHER_TYPE                    2        
#define SIZE_ISL_ADDR                      5        
#define SIZE_ISL_SNAP                      3        
#define SIZE_ISL_HSA                       3        

#define VALUE_802_1Q_ETHER_TYPE            0x8100

#define LENGTH_HEADER_BASE_ETHERNET        14
#define LENGTH_HEADER_BASE_VLAN            //TODO
#define LENGTH_HEADER_BASE_ISL             //TODO
#define LENGTH_HEADER_BASE_MPLS             //TODO
#define LENGTH_HEADER_BASE_IP4             20
#define LENGTH_HEADER_BASE_IP6             32
#define LENGTH_HEADER_BASE_IP4OVERIP6      (32+20)
#define LENGTH_HEADER_BASE_IP6OVERIP4      (32+20)
#define LENGTH_HEADER_BASE_ARP             //TODO
#define LENGTH_HEADER_BASE_TCP             20
#define LENGTH_HEADER_BASE_UDP             //TODO
#define LENGTH_HEADER_BASE_ICMP            //TODO
#define LENGTH_HEADER_BASE_IGMPV2          //TODO
#define LENGTH_HEADER_BASE_UDF             //TODO

#define LENGTH_HEADER_RESERVE              (8*25) //Register PHIR SIZE
#define LENGTH_PAYLOAD_RESERVE             (10*1024) //Consider Jumbo frame

#define IP6_NO_NEXT_HEADER                 59
#define SUBNET_MASK(x)                     (((x)==0)?0:(0xFFFFFFFF<<(32-(x))))
#define SUBNET_MASK2(x)                    (((x)==0)?0:(0xFFFFFFFF<<((x))))

typedef struct {
	EnumProtocol          protocol;
	unsigned int          length;   //the length of this PDR
	unsigned int          offset;   //the offset this protocol starts
	void*                 pdr;
} T_ProtoUnit;

typedef struct { 
	unsigned int          streamid; 
	T_ProtoUnit           l2;       //Ethernet, custom
	T_ProtoUnit           l2tag;    //ISL, VLAN, MPLS, etc.
	T_ProtoUnit           l3;       //IP4, IP6, ARP, etc.
	T_ProtoUnit           l4;       //TCP, UDP, ICMP, IGMPv2, etc.
	unsigned int          headerLength;
	unsigned char         header[LENGTH_HEADER_RESERVE];
	T_Frame               fi;       //Frame & payload information.
} T_Protocol;

typedef struct {
	EnumEtherAddrMode     mode;
	char                  addr[SIZE_ETHER_ADDR];
	unsigned int          repeatCount;
	unsigned int          step;
} T_EtherAddrUnit;

typedef struct {
	T_EtherAddrUnit       dest;
	T_EtherAddrUnit       src;
} T_EtherAddr;

typedef struct {
	unsigned char         dest[SIZE_ISL_ADDR];
	unsigned char         type:4;
	unsigned char         user:4;
	unsigned char         src[SIZE_ETHER_ADDR];
	unsigned short        length;
	unsigned char         snap[SIZE_ISL_SNAP];
	unsigned char         hsa[SIZE_ISL_HSA];
	unsigned short        vlanid:15;
	unsigned short        bpduInd:1;
	unsigned short        index;
	unsigned short        reserved;
} T_Isl;

typedef struct {
	unsigned short        tpid; 
	unsigned short        priority:3;
	unsigned short        cfi:1;
	unsigned short        vlanid:12;
} T_802_1Q_VLANTag;

typedef struct {
	unsigned int          label:20; 
	unsigned int          exp:3;
	unsigned int          s:1;
	unsigned int          ttl:8;
} T_Mpls;

typedef struct {
	unsigned int          addr;
	unsigned int          mask;
	unsigned int          repeat;
	EnumIpAddrMode        mode;
} T_Ip4AddrUnit;

typedef struct {
	EnumChecksum          checksum;
	unsigned short        value; //has meaning if Override
} T_Checksum;

typedef struct {
	unsigned int          length;
	void*                 pval;
} T_Ip4Options;

typedef struct {
	unsigned char         version:4;
	unsigned char         hlen:4;
	unsigned char         tos;
	unsigned short        tlen;
	unsigned short        id;
	unsigned short        flags:3;
	unsigned short        fragoffset:13;
	unsigned char         ttl;
	unsigned char         proto;
	unsigned short        checksum;
	unsigned int          sip;
	unsigned int          dip;
} T_Ip4;

//TODO: IP6 definition
typedef struct {
	unsigned char         addr[16];
	unsigned int          repeat;
	unsigned int          step;
	EnumIpAddrMode        mode;
} T_Ip6AddrUnit;

typedef struct {
	unsigned int          version:4;
	unsigned int          tclass:8;
	unsigned int          flabel:20;
	unsigned short        plen;
	unsigned char         nextheader;
	unsigned char         hoplimit;
	unsigned char         sip[16];
	unsigned char         dip[16];
} T_Ip6;

//UDP
typedef struct {
	unsigned int          override;
	unsigned short        val;
} T_UdpLength;

typedef struct {
	unsigned short        sport;
	unsigned short        dport;
	unsigned short        length;
	unsigned short        checksum;
} T_Udp;

//TCP
typedef struct {
	unsigned short        sport;
	unsigned short        dport;
	unsigned int          seqnum;
	unsigned int          acknum;
	unsigned short        offset:4;
	unsigned short        flag_res:6;
	unsigned short        flag_urg:1;
	unsigned short        flag_ack:1;
	unsigned short        flag_psh:1;
	unsigned short        flag_rst:1;
	unsigned short        flag_syn:1;
	unsigned short        flag_fin:1;
	unsigned short        windows;
	unsigned short        checksum;
	unsigned short        urgent;
} T_Tcp;

typedef struct {
	unsigned int          length;
	void*                 pval;
} T_TcpOptions;

//IGMPv2
typedef struct {
	unsigned char         ver:4;
	unsigned char         type:4;
	unsigned char         maxRespTime;
	unsigned short        checksum;
	unsigned int          group;
} T_Igmpv2;

//ICMP for IPv4
typedef struct {
	unsigned char         type;
	unsigned char         code;
	unsigned short        checksum;
	unsigned short        data1;
	unsigned short        data2;
} T_Icmp;

//ARP
typedef struct {
	unsigned short        hwtype;
	unsigned short        protocoltype;
	unsigned char         halen;
	unsigned char         palen;
	unsigned short        operation;
	char                  senderMac[SIZE_ETHER_ADDR];
	unsigned int          senderIp;
	char                  targetMac[SIZE_ETHER_ADDR];
	unsigned int          targetIp;
} T_Arp;

//UDF
typedef struct {
	unsigned int          id; //udf index
	unsigned int          enable;
	unsigned int          offset;
	EnumUDFCounterType    type;
	unsigned int          isRandom;
	unsigned int          repeatCount; //0 if continuous counting
	EnumUDFCountingMode   mode;
	unsigned int          step;
	unsigned int          initval;
	EnumUDFInitValueType  initvalType;
	unsigned int          bitmask;
} T_UDF;


/*****************************************************************************/
/* For Message Interface */
/*****************************************************************************/

typedef struct {
	EnumEtherAddrMode     mode;
	unsigned char         addr[SIZE_ETHER_ADDR];
	unsigned char         padding[2];
	unsigned int          repeatCount;
	unsigned int          step;
} T_EtherAddrTuple;

typedef struct {
	unsigned int          label;
	unsigned int          exp;
	unsigned int          s;
	unsigned int          ttl;
} T_MplsTuple;

typedef struct {
	unsigned int          priority;
	unsigned int          cfi;
	unsigned int          vlanid;
	unsigned int          tagProtocolId;
	EnumVlanIdCountMode   vlanIdCountMode;
	unsigned int          count;
	unsigned int          bitMask;
} T_VlanTuple;

typedef struct {
	EnumChecksum          type;
	unsigned int          value; //has meaning if Override
} T_ChecksumTuple;

typedef struct {
	unsigned int          addr;
	unsigned int          mask;
	unsigned int          repeat;
	EnumIpAddrMode        mode;
} T_Ip4AddrTuple;

typedef struct {
	unsigned char         addr[16];
	unsigned int          repeat;
	unsigned int          step;
	unsigned int          mask;
	EnumIpAddrMode        mode;
} T_Ip6AddrTuple;

/* deprecated 
typedef struct {
	EnumCustomIntegerMode mode;
	unsigned int          stepOrValue;
	unsigned int          min;
	unsigned int          max;
} T_CustomIntegerTuple;
*/

typedef struct {
	EnumCustomIntegerMode mode;
	unsigned int          value;
	unsigned int          step;
	unsigned int          repeat;
} T_CustomIntegerTuple;
//
//protocol id 21
typedef struct {
	T_EtherAddrTuple      dest;
	T_EtherAddrTuple      src;
	unsigned int          type;
} T_PDR_Ethernet;

//protocol id 22
typedef struct {
	EnumVlanMode          mode;
	T_VlanTuple           vlan;
	T_VlanTuple           vlanDup[0];
} T_PDR_802_1Q_VLAN;

//protocol id 23
typedef struct {
	char                  dest[SIZE_ISL_ADDR];
	char                  padding0[3]; //for byte align - SIZE_ISL_ADDR
	unsigned int          type;
	unsigned int          user;
	char                  src[SIZE_ETHER_ADDR];
	char                  padding1[2]; //for byte align - SIZE_ETHER_ADDR
	unsigned int          length;
	unsigned int          vlanid;
	unsigned int          bpduInd;
	unsigned int          index;
	unsigned int          reserved;
} T_PDR_ISL;

//protocol id 24
typedef struct {
	EnumMplsType          type;
	T_MplsTuple           mpls;
	T_MplsTuple           mplsDup[0]; 
} T_PDR_MPLS;

//protocol id 31
typedef struct {
	unsigned int          version;
	unsigned int          hlen;
	unsigned int          tos;
	unsigned int          tlen;
//	unsigned int          id;
	T_CustomIntegerTuple  id;								// 2017.2.27 by dgjung
	unsigned int          flags;
	unsigned int          fragoffset;
	unsigned int          ttl;
	unsigned int          proto;
	T_ChecksumTuple       checksum;
	T_Ip4AddrTuple        sip;
	T_Ip4AddrTuple        dip;
} T_PDR_Ip4;

typedef struct {
	unsigned int          length;
	unsigned int          pval[0];
} T_PDR_Ip4Options;

//protocol id 32
typedef struct {
	unsigned int          version;
	unsigned int          tclass;
	unsigned int          flabel;
	unsigned int          plen;
	unsigned int          nextheader;
	unsigned int          hoplimit;
	T_Ip6AddrTuple        sip;
	T_Ip6AddrTuple        dip;
} T_PDR_Ip6;

typedef struct {
	unsigned int          length;
	unsigned int          pval[0];
} T_PDR_Ip6NextHeader;

//protocol id 33
typedef struct {
	T_PDR_Ip6             ip6;
	T_PDR_Ip4             ip4;
} T_PDR_Ip4OverIp6;

//protocol id 34
typedef struct {
	T_PDR_Ip4             ip4;
	T_PDR_Ip6             ip6;
} T_PDR_Ip6OverIp4;

//protocol id 35
typedef struct {
	unsigned int          type;
	unsigned int          hwtype;
	unsigned int          protocoltype;
	unsigned int          halength;
	unsigned int          palength;
	unsigned int          operation;
	T_EtherAddrTuple      senderMac;
	T_Ip4AddrTuple        senderIp;
	T_EtherAddrTuple      targetMac;
	T_Ip4AddrTuple        targetIp;
} T_PDR_ARP;

//protocol id 41
typedef struct {
	T_CustomIntegerTuple  sport;
	T_CustomIntegerTuple  dport;
	unsigned int          seqnum;
	unsigned int          acknum;
	unsigned int          offset;
	unsigned int          flag_res;
	unsigned int          flag_urg;
	unsigned int          flag_ack;
	unsigned int          flag_psh;
	unsigned int          flag_rst;
	unsigned int          flag_syn;
	unsigned int          flag_fin;
	unsigned int          windows;
	T_ChecksumTuple       checksum;
	unsigned int          urgent;
	unsigned int          optionLength;
	unsigned char         option[0];
} T_PDR_TCP;

typedef struct {
	unsigned int          length;
	unsigned int          pval[0];
} T_PDR_TcpOptions;

//protocol id 42
typedef struct {
	T_CustomIntegerTuple  sport;
	T_CustomIntegerTuple  dport;
	unsigned int          override;
	unsigned int          val;
	T_ChecksumTuple       checksum;
} T_PDR_UDP;

//protocol id 43
//ICMP for IPv4
typedef struct {
	unsigned int          type;
	unsigned int          code;
	T_ChecksumTuple       checksum;
	unsigned int          data1;
	unsigned int          data2;
} T_PDR_ICMP;

//protocol id 44
//IGMPv2
typedef struct {
	unsigned int          ver;
	unsigned int          type;
	unsigned int          maxRespTime;
	T_ChecksumTuple       checksum;
	T_Ip4AddrTuple        group;
} T_PDR_IGMPv2;

//protocol id 51
//UDF
typedef struct {
	unsigned int          id; //udf index
	unsigned int          enable;
	EnumUDFCounterType    type;
	unsigned int          isRandom;
	unsigned int          offset;
	unsigned int          repeatCount; //0 if continuous counting
	EnumUDFCountingMode   mode;
	unsigned int          step;
	unsigned int          initval;
	EnumUDFInitValueType  initvalType;
	unsigned int          bitmask;
} T_PDR_UDF;

#pragma pack()

#endif //BSL_PROTO_H
