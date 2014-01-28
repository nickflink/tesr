#ifndef TESR_CONFIG_H
#define TESR_CONFIG_H
#define DEFAULT_RECV_PORT 7
#define DEFAULT_IP_RATE_LIMIT_MAX 0
#define DEFAULT_IP_RATE_LIMIT_PERIOD 0
#define DEFAULT_IP_RATE_LIMIT_PRUNE_MARK 0
#include <arpa/inet.h>
#include <tesr_common.h>
#include "tesr_types.h"
void log_config(tesr_config_t *tesr_config);
void init_config(tesr_config_t *tesr_config, int argc, char **argv);
#endif //TESR_CONFIG_H
