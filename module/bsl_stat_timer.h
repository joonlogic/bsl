#ifndef _BSL_MODULE_STAT_TIMER_H_
#define _BSL_MODULE_STAT_TIMER_H_

#include "../api/stat.h"

#define TIME_STEP	(1*HZ)	// 1sec
#define MAX_STAT_QUEUE_SIZE	600

#define BSL_STAT_TRAFFIC_ONOFF			"bsl_stat_traffic_onoff"
#define BSL_STAT_TRAFFIC_PORT_LIST_NAME		"bsl_stat_traffic_port_list"
#define BSL_STAT_TRAFFIC_IP_LIST_NAME		"bsl_stat_traffic_ip_list"
#define BSL_STAT_TRAFFIC_OP_LIST_NAME		"bsl_stat_traffic_op_list"
#define BSL_STAT_TRAFFIC_IB_LIST_NAME		"bsl_stat_traffic_ib_list"
#define BSL_STAT_TRAFFIC_OB_LIST_NAME		"bsl_stat_traffic_ob_list"

int bsl_stat_timer_init(struct timer_list *bsl_timerp);
void bsl_stat_timer_cleanup(struct timer_list *bsl_timerp);

typedef struct {
	bsl_port_stat_t port0;
	bsl_port_stat_t port1;
} bsl_stat_queue_t;

typedef bsl_stat_queue_t element_t;

struct Node {
	element_t Data;
	struct Node *NextNode;
};

typedef enum {
	PORT_TRAFFIC,
	INBOUND_PACKET,
	OUTBOUND_PACKET,
	INBOUND_BYTE,
	OUTBOUND_BYTE
} traffic_list_type_t;

int bsl_stat_queue_reset(void);
void bsl_stat_queue_dump(void);

#endif
