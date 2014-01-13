// Source: http://www.mail-archive.com/libev@lists.schmorp.de/msg00987.html

#include "tesr_config.h"
#include <getopt.h>
#include <libconfig.h>
#include <stdlib.h>
#include <string.h>
#include <tesr_common.h>
#include <utlist.h>

#define CONFIG_ERR_NONE 0
#define CONFIG_ERR_NON_NUMERIC_PORT 1
#define CONFIG_ERR_INVALID_CMD_LINE_OPTION 2
#define CONFIG_ERR_UNHANDLED_CMD_LINE_OPTION 3
#define CONFIG_ERR_UNPARSED_CMD_LINE_OPTION 4

void log_config(tesr_config_t *tesr_config) {
    LOG_INFO("tesr_config->recv_port=%d\n", tesr_config->recv_port);
    LOG_INFO("tesr_config->num_worker_threads=%d\n", tesr_config->num_worker_threads);
    tesr_send_port_t *send_port = NULL;
    int print_comma = 0;
    LOG_INFO("send_ports=[");
    LL_FOREACH(tesr_config->send_ports, send_port) {
        if(print_comma) {
            LOG_INFO(",");
        } else {
            print_comma = 1;
        }
        LOG_INFO("%d", send_port->port);
    }
    LOG_INFO("]\n");
    print_comma = 0;
    tesr_filter_t *filter = NULL;
    LOG_INFO("filters=[");
    LL_FOREACH(tesr_config->filters, filter) {
        if(print_comma) {
            LOG_INFO(",");
        } else {
            print_comma = 1;
        }
        LOG_INFO("%s", filter->filter);
    }
    LOG_INFO("]\n");
}

void init_config(tesr_config_t *tesr_config, int argc, char **argv) {
    //
    // DEFAULTS
    //
    int config_err = CONFIG_ERR_NONE;
    tesr_config->recv_port = DEFAULT_RECV_PORT;
    tesr_config->ip_rate_limit_max = DEFAULT_IP_RATE_LIMIT_MAX;
    tesr_config->ip_rate_limit_period = DEFAULT_IP_RATE_LIMIT_PERIOD;
    tesr_config->ip_rate_limit_prune_mark = DEFAULT_IP_RATE_LIMIT_PRUNE_MARK;
    tesr_config->num_worker_threads = 0;
    tesr_config->filters = NULL;
    tesr_config->send_ports = NULL;
    int configPort = 0;
    int cmdlinePort = 0;
    //
    // Configuration File
    //
    config_t cfg;
    char *local_config_file_name = "tesr.conf";
    char *system_config_file_name = "/etc/tesr.conf";
    //Initialization
    config_init(&cfg);
    int config_loaded = 0;
    /* Read the file. If there is an error, report it and exit. */
    if(!config_loaded) {
        config_loaded = config_read_file(&cfg, local_config_file_name);
        if(config_loaded) LOG_INFO("using config ./%s\n", local_config_file_name);
    }
    if(!config_loaded) {
        config_loaded = config_read_file(&cfg, system_config_file_name);
        if(config_loaded) LOG_INFO("using config %s\n", system_config_file_name);
    }
    if(!config_loaded) {
        LOG_WARN("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
    }
    // get the value of recv_port
    if(config_lookup_int(&cfg, "recv_port", &configPort)) {
        LOG_DEBUG("\nrecv_port: %d\n", configPort);
        tesr_config->recv_port = configPort;
    } else {
        LOG_DEBUG("\nNo 'recv_port' setting in configuration file.");
    }
    // get the value of ip_rate_limit_max
    if(config_lookup_int(&cfg, "ip_rate_limit_max", &configPort)) {
        LOG_INFO("\nip_rate_limit_max: %d\n", configPort);
        tesr_config->ip_rate_limit_max = configPort;
    } else {
        LOG_INFO("\nNo 'ip_rate_limit_max' setting in configuration file disabling.");
    }
    // get the value of ip_rate_limit_period
    if(config_lookup_int(&cfg, "ip_rate_limit_period", &configPort)) {
        LOG_INFO("\nip_rate_limit_period: %d\n", configPort);
        tesr_config->ip_rate_limit_period = configPort;
    } else {
        LOG_INFO("\nNo 'ip_rate_limit_period' setting in configuration file.");
    }
    // get the value of ip_rate_limit_prune_mark
    if(config_lookup_int(&cfg, "ip_rate_limit_prune_mark", &configPort)) {
        LOG_INFO("\nip_rate_limit_period: %d\n", configPort);
        tesr_config->ip_rate_limit_prune_mark = configPort;
    } else {
        LOG_INFO("\nNo 'ip_rate_limit_prune_mark' setting in configuration file.");
    }
    //get the value of send_ports 
    config_setting_t *send_ports = config_lookup(&cfg, "send_ports");
    int send_port_idx = 0;
    if(send_ports) {
        tesr_config->num_worker_threads = 0;
        config_setting_t *send_port = config_setting_get_elem(send_ports, send_port_idx++);
        while(send_port) {
            tesr_send_port_t *element = (tesr_send_port_t *)malloc(sizeof(tesr_send_port_t));
            element->port = config_setting_get_int(send_port);
            LL_PREPEND(tesr_config->send_ports, element);
            LOG_DEBUG("send_port=%d\n", element->port);
            send_port = config_setting_get_elem(send_ports, send_port_idx++);
            ++tesr_config->num_worker_threads;
        }
    }
    config_setting_t *filters = config_lookup(&cfg, "filters");
    int filterIdx = 0;
    if(filters) {
        LOG_DEBUG("we have filters\n");
        config_setting_t *filter = config_setting_get_elem(filters, filterIdx++);
        while(filter) {
            LOG_DEBUG("we have a filter\n");
            const char *sz_filter = config_setting_get_string(filter);
            if(sz_filter) {
                tesr_filter_t *element = (tesr_filter_t *)malloc(sizeof(tesr_filter_t));
                strncpy(element->filter, sz_filter, INET_ADDRSTRLEN);
                LL_PREPEND(tesr_config->filters, element);
                LOG_DEBUG("filter=%s\n", sz_filter);
            }
            filter = config_setting_get_elem(filters, filterIdx++);
        }
    }
    //
    // OptArg Overrides
    //
    int c;
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"port",    required_argument, 0, 'p'},
            {0,         0,                 0,  0 }
        };
        c = getopt_long(argc, argv, "p:", long_options, &option_index);
        if (c == -1)
            break;
        switch (c) {
        case 'p':
            cmdlinePort = atoi(optarg);
            if(cmdlinePort == 0) {
                LOG_ERROR("[KO] non-numeric port '%s'", optarg);
                config_err = CONFIG_ERR_NON_NUMERIC_PORT;
            } else {
                LOG_DEBUG("command line port = %d\n", cmdlinePort);
                tesr_config->recv_port = cmdlinePort;
            }
            break;
        case '?':
            config_err = CONFIG_ERR_INVALID_CMD_LINE_OPTION;
            LOG_ERROR("?? invalid option 0%o ??\n", c);
            break;
        default:
            LOG_ERROR("?? getopt returned character code 0%o ??\n", c);
            config_err = CONFIG_ERR_UNHANDLED_CMD_LINE_OPTION;
        }
    }
   if (optind < argc) {
        LOG_ERROR("non-option ARGV-elements: ");
        while (optind < argc)
            LOG_ERROR("%s ", argv[optind++]);
        LOG_ERROR("\n");
        config_err = CONFIG_ERR_UNPARSED_CMD_LINE_OPTION;
    }
    if(config_err) {
        //We can exit like this when checking the config as it is only checked in the beginning
        exit(config_err);
    }
}
