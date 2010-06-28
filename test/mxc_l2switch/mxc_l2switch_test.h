/*
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All rights reserved.
 * Author: Shrek Wu <b16972@freescale.com>
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#define ESW_SET_LEARNING_CONF               0x9101
#define ESW_GET_LEARNING_CONF               0x9201
#define ESW_SET_BLOCKING_CONF               0x9102
#define ESW_GET_BLOCKING_CONF               0x9202
#define ESW_SET_MULTICAST_CONF              0x9103
#define ESW_GET_MULTICAST_CONF              0x9203
#define ESW_SET_BROADCAST_CONF              0x9104
#define ESW_GET_BROADCAST_CONF              0x9204
#define ESW_SET_PORTENABLE_CONF             0x9105
#define ESW_GET_PORTENABLE_CONF             0x9205
#define ESW_SET_IP_SNOOP_CONF               0x9106
#define ESW_GET_IP_SNOOP_CONF               0x9206
#define ESW_SET_PORT_SNOOP_CONF             0x9107
#define ESW_GET_PORT_SNOOP_CONF             0x9207
#define ESW_SET_PORT_MIRROR_CONF            0x9108
#define ESW_GET_PORT_MIRROR_CONF            0x9208
#define ESW_SET_PIRORITY_VLAN               0x9109
#define ESW_GET_PIRORITY_VLAN               0x9209
#define ESW_SET_PIRORITY_IP                 0x910A
#define ESW_GET_PIRORITY_IP                 0x920A
#define ESW_SET_PIRORITY_MAC                0x910B
#define ESW_GET_PIRORITY_MAC                0x920B
#define ESW_SET_PIRORITY_DEFAULT            0x910C
#define ESW_GET_PIRORITY_DEFAULT            0x920C
#define ESW_SET_P0_FORCED_FORWARD           0x910D
#define ESW_GET_P0_FORCED_FORWARD           0x920D
#define ESW_SET_SWITCH_MODE                 0x910E
#define ESW_GET_SWITCH_MODE                 0x920E
#define ESW_SET_BRIDGE_CONFIG               0x910F
#define ESW_GET_BRIDGE_CONFIG               0x920F
#define ESW_SET_VLAN_OUTPUT_PROCESS         0x9110
#define ESW_GET_VLAN_OUTPUT_PROCESS         0x9210
#define ESW_SET_VLAN_INPUT_PROCESS          0x9111
#define ESW_GET_VLAN_INPUT_PROCESS          0x9211
#define ESW_SET_VLAN_DOMAIN_VERIFICATION    0x9112
#define ESW_GET_VLAN_DOMAIN_VERIFICATION    0x9212
#define ESW_SET_VLAN_RESOLUTION_TABLE       0x9113
#define ESW_GET_VLAN_RESOLUTION_TABLE       0x9213


#define ESW_GET_STATISTICS_STATUS           0x9221
#define ESW_GET_PORT0_STATISTICS_STATUS     0x9222
#define ESW_GET_PORT1_STATISTICS_STATUS     0x9223
#define ESW_GET_PORT2_STATISTICS_STATUS     0x9224
#define ESW_SET_OUTPUT_QUEUE_MEMORY         0x9125
#define ESW_GET_OUTPUT_QUEUE_STATUS         0x9225
#define ESW_UPDATE_STATIC_MACTABLE          0x9226
#define ESW_CLEAR_ALL_MACTABLE              0x9227

typedef struct _eswIOCTL_PORT_CONF {
	int port;
	int enable;
} eswIoctlPortConfig;

typedef struct _eswIOCTL_PORT_EN_CONF {
	int port;
	int tx_enable;
	int rx_enable;
} eswIoctlPortEnableConfig;

typedef struct _eswIOCTL_IP_SNOOP_CONF {
	int num;
	int mode;
	unsigned char ip_header_protocol;
} eswIoctlIpsnoopConfig;

typedef struct _eswIOCTL_P0_FORCED_FORWARD_CONF {
	int port1;
	int port2;
	int enable;
} eswIoctlP0ForcedForwardConfig;

typedef struct _eswIOCTL_PORT_SNOOP_CONF {
	int num;
	int mode;
	unsigned short compare_port;
	int compare_num;
} eswIoctlPortsnoopConfig;

typedef struct _eswIOCTL_PORT_Mirror_CONF {
	int mirror_port;
	int port;
	int egress_en;
	int ingress_en;
	int egress_mac_src_en;
	int egress_mac_des_en;
	int ingress_mac_src_en;
	int ingress_mac_des_en;
	unsigned char *src_mac;
	unsigned char *des_mac;
	int mirror_enable;
} eswIoctlPortMirrorConfig;

typedef struct _eswIOCTL_PRIORITY_VLAN_CONF {
	int port;
	int func_enable;
	int vlan_pri_table_num;
	int vlan_pri_table_value;
} eswIoctlPriorityVlanConfig;

typedef struct _eswIOCTL_PRIORITY_IP_CONF {
	int port;
	int func_enable;
	int ipv4_en;
	int ip_priority_num;
	int ip_priority_value;
} eswIoctlPriorityIPConfig;

typedef struct _eswIOCTL_PRIORITY_MAC_CONF {
	int port;
} eswIoctlPriorityMacConfig;

typedef struct _eswIOCTL_PRIORITY_DEFAULT_CONF {
	int port;
	unsigned char priority_value;
} eswIoctlPriorityDefaultConfig;

typedef struct _eswIOCTL_IRQ_STATUS {
	unsigned long isr;
	unsigned long imr;
	unsigned long rx_buf_pointer;
	unsigned long tx_buf_pointer;
	unsigned long rx_max_size;
	unsigned long rx_buf_active;
	unsigned long tx_buf_active;
} eswIoctlIrqStatus;

typedef struct _eswIOCTL_PORT_Mirror_STATUS{
	unsigned long ESW_MCR;
	unsigned long ESW_EGMAP;
	unsigned long ESW_INGMAP;
	unsigned long ESW_INGSAL;
	unsigned long ESW_INGSAH;
	unsigned long ESW_INGDAL;
	unsigned long ESW_INGDAH;
	unsigned long ESW_ENGSAL;
	unsigned long ESW_ENGSAH;
	unsigned long ESW_ENGDAL;
	unsigned long ESW_ENGDAH;
	unsigned long ESW_MCVAL;
} eswIoctlPortMirrorStatus;

typedef struct _eswIOCTL_VLAN_OUTPUT_CONF {
	int port;
	int mode;
} eswIoctlVlanOutputConfig;

typedef struct _eswIOCTL_VLAN_INPUT_CONF {
	int port;
	int mode;
	unsigned short port_vlanid;
	int vlan_verify_en;
	int vlan_domain_num;
	int vlan_domain_port;
} eswIoctlVlanInputConfig;

typedef struct _eswIOCTL_VLAN_DOMAIN_VERIFY_CONF {
	int port;
	int vlan_domain_verify_en;
	int vlan_discard_unknown_en;
} eswIoctlVlanVerificationConfig;

typedef struct _eswIOCTL_VLAN_RESOULATION_TABLE {
	unsigned short port_vlanid;
	int vlan_domain_num;
	int vlan_domain_port;
} eswIoctlVlanResoultionTable;


typedef struct _eswIOCTL_VLAN_INPUT_STATUS {
	unsigned long ESW_VLANV;
	unsigned long ESW_PID[3];
	unsigned long ESW_VIMSEL;
	unsigned long ESW_VIMEN;
	unsigned long ESW_VRES[32];
} eswIoctlVlanInputStatus;

typedef struct _eswIOCTL_Static_MACTable {
	unsigned char *mac_addr;
	int port;
	int priority;
} eswIoctlUpdateStaticMACtable;


typedef struct l2switch_output_queue_status {
	unsigned long ESW_MMSR;
	unsigned long ESW_LMT;
	unsigned long ESW_LFC;
	unsigned long ESW_PCSR;
	unsigned long ESW_IOSR;
	unsigned long ESW_QWT;
	unsigned long esw_reserved;
	unsigned long ESW_P0BCT;
} esw_output_queue_status;

typedef struct l2switch_statistics_status {
	unsigned long ESW_DISCN;
	unsigned long ESW_DISCB;
	unsigned long ESW_NDISCN;
	unsigned long ESW_NDISCB;
} esw_statistics_status;

typedef struct l2switch_port_statistics_status {
	unsigned long MCF_ESW_POQC;
	unsigned long MCF_ESW_PMVID;
	unsigned long MCF_ESW_PMVTAG;
	unsigned long MCF_ESW_PBL;
} esw_port_statistics_status;

