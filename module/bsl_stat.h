#ifndef _NSS_MODULE_STAT_H_
#define _NSS_MODULE_STAT_H_

#include "nss_stat_timer.h"

#define NSS_STAT_NAME		"nss_stat"
#define NSS_STAT_CMD_MAX_LEN	2

#define CMD_STAT_TRAFFIC_CLEAR		"00"
#define CMD_STAT_TRAFFIC_SHOW		"01"
#define CMD_STAT_ACL_CLEAR		"10"
#define CMD_STAT_ACL_SHOW		"11"
#define CMD_STAT_SYNCOOKIE_CLEAR	"20"
#define CMD_STAT_SYNCOOKIE_SHOW		"21"
#define CMD_STAT_RATELIMIT_GLOBAL_CLEAR	"30"
#define CMD_STAT_RATELIMIT_GLOBAL_SHOW	"31"
#define CMD_STAT_RATELIMIT_LOCAL_CLEAR	"40"

// for debug
#define CMD_STAT_DEBUG_QUEUE_DUMP		"90"

#define CMD_STAT_SW_RESET				"91"
#define CMD_STAT_PHY_INIT				"92"

int nss_stat_init(nss_stat_t *nss_statp);
void nss_stat_cleanup(nss_stat_t *nss_statp);
int nss_clear_port_traffic(void);
int nss_get_diag_traffic(nss_pci_t *nss_pci, nss_diag_stat_t *stat);
int nss_get_acl_traffic(nss_pci_t *nss_pci, nss_acl_stat_t *stat);
int nss_get_abnormal_packet_counter(nss_pci_t *nss_pci, nss_abnormal_packet_stat_t *stat);
int nss_get_refresh_packet_counter(nss_pci_t *nss_pci, nss_refresh_packet_counter_t *stat);

#endif
