#include "tesr_config.h"
#include <getopt.h>
#include <libconfig.h>
#include <stdlib.h>
#include <string.h>
#include <tesr_common.h>
#include <utlist.h>

#define CONFIG_ERR_NONE 0
#define CONFIG_ERR_NON_NUMERIC_ARG 1
#define CONFIG_ERR_INVALID_CMD_LINE_OPTION 2
#define CONFIG_ERR_UNHANDLED_CMD_LINE_OPTION 3
#define CONFIG_ERR_UNPARSED_CMD_LINE_OPTION 4
#define CONFIG_ERR_HELP_REQUESTED 5

tesr_config_t *create_config() {
    tesr_config_t* thiz = NULL;
    thiz = (tesr_config_t*)malloc(sizeof(tesr_config_t));
    return thiz;
}

void init_config(tesr_config_t *thiz, int argc, char **argv) {
    //
    // DEFAULTS
    //
    int config_err = CONFIG_ERR_NONE;
    thiz->recv_port = DEFAULT_RECV_PORT;
    thiz->ip_rate_limit_max = DEFAULT_IP_RATE_LIMIT_MAX;
    thiz->ip_rate_limit_period = DEFAULT_IP_RATE_LIMIT_PERIOD;
    thiz->ip_rate_limit_prune_mark = DEFAULT_IP_RATE_LIMIT_PRUNE_MARK;
    thiz->num_workers = 0;
    thiz->filters = NULL;
    int recv_port_conf = 0;
    int recv_port_arg = 0;
    int num_workers_conf = 0;
    int num_workers_arg = 0;
    //
    // Configuration File
    //
    config_t cfg;
    char *local_config_file_name = "./tesr.conf";
    char *system_config_file_name = "/etc/tesr.conf";
    //Initialization
    config_init(&cfg);
    int config_loaded = 0;
    /* Read the file. If there is an error, report it and exit. */
    if(!config_loaded) {
        config_loaded = config_read_file(&cfg, local_config_file_name);
        if(config_loaded) {
        LOG_INFO("using config %s\n", local_config_file_name);
    } else {
        LOG_INFO("no local config found at %s\n", local_config_file_name);
    }
    }
    if(!config_loaded) {
        config_loaded = config_read_file(&cfg, system_config_file_name);
        if(config_loaded) {
        LOG_INFO("using config %s\n", system_config_file_name);
    } else {
        LOG_INFO("no system config found at %s\n", system_config_file_name);
    }
    }
    if(!config_loaded) {
        LOG_WARN("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
    }
    if(config_loaded) {
        // get the value of recv_port
        if(config_lookup_int(&cfg, "recv_port", &recv_port_conf)) {
            LOG_DEBUG("\nrecv_port: %d\n", recv_port_conf);
            thiz->recv_port = recv_port_conf;
        } else {
            LOG_DEBUG("\nNo 'recv_port' setting in configuration file.");
        }
        // get the value of num_workers
        if(config_lookup_int(&cfg, "num_workers", &num_workers_conf)) {
            LOG_DEBUG("\nnum_workers: %d\n", num_workers_conf);
            thiz->num_workers = num_workers_conf;
        } else {
            LOG_DEBUG("\nNo 'num_workers' setting in configuration file.");
        }
        // get the value of ip_rate_limit_max
        if(config_lookup_int(&cfg, "ip_rate_limit_max", &thiz->ip_rate_limit_max)) {
            LOG_INFO("\nip_rate_limit_max: %d\n", thiz->ip_rate_limit_max);
            thiz->ip_rate_limit_max = thiz->ip_rate_limit_max;
        } else {
            LOG_INFO("\nNo 'ip_rate_limit_max' setting in configuration file disabling.");
        }
        // get the value of ip_rate_limit_period
        if(config_lookup_int(&cfg, "ip_rate_limit_period", &thiz->ip_rate_limit_period)) {
            LOG_INFO("\nip_rate_limit_period: %d\n", thiz->ip_rate_limit_period);
            thiz->ip_rate_limit_period = thiz->ip_rate_limit_period;
        } else {
            LOG_INFO("\nNo 'ip_rate_limit_period' setting in configuration file.");
        }
        // get the value of ip_rate_limit_prune_mark
        if(config_lookup_int(&cfg, "ip_rate_limit_prune_mark", &thiz->ip_rate_limit_prune_mark)) {
            LOG_INFO("\nip_rate_limit_period: %d\n", thiz->ip_rate_limit_prune_mark);
            thiz->ip_rate_limit_prune_mark = thiz->ip_rate_limit_prune_mark;
        } else {
            LOG_INFO("\nNo 'ip_rate_limit_prune_mark' setting in configuration file.");
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
                    LL_PREPEND(thiz->filters, element);
                    LOG_DEBUG("filter=%s\n", sz_filter);
                }
                filter = config_setting_get_elem(filters, filterIdx++);
            }
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
            {"workers", required_argument, 0, 'w'},
            {"help",    no_argument, 0, 'h'},
            {0,         0,                 0,  0 }
        };
        c = getopt_long(argc, argv, "p:w:", long_options, &option_index);
        if (c == -1)
            break;
        switch (c) {
        case 'h':
            config_err = CONFIG_ERR_HELP_REQUESTED;
            break;
        case 'p':
            recv_port_arg = atoi(optarg);
            if(recv_port_arg == 0) {
                LOG_ERROR("[KO] non-numeric port '%s'", optarg);
                config_err = CONFIG_ERR_NON_NUMERIC_ARG;
            } else {
                LOG_DEBUG("command line port = %d\n", recv_port_arg);
                thiz->recv_port = recv_port_arg;
            }
            break;
        case 'w':
            num_workers_arg = atoi(optarg);
            LOG_DEBUG("command line workers = %d\n", num_workers_arg);
            thiz->num_workers = num_workers_arg;
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
        LOG_ERROR("\
\nUSAGE tesr [T]hreaded [E]cho [S]erve[R]\
\n-p --port (override the port)\
\n-w --workers (override the number of workers)\
\n");
        exit(config_err);
    }
}

void destroy_config(tesr_config_t *thiz) {
    if(thiz) {
        free(thiz);
        thiz = NULL;
    } else {
        LOG_ERROR("can not free tesr_config_t* as it is NULL");
    }
}

void log_config(tesr_config_t *thiz) {
    LOG_INFO("thiz->recv_port=%d\n", thiz->recv_port);
    LOG_INFO("thiz->num_workers=%d\n", thiz->num_workers);
    int print_comma = 0;
    tesr_filter_t *filter = NULL;
    LOG_INFO("filters=[");
    LL_FOREACH(thiz->filters, filter) {
        if(print_comma) {
            LOG_INFO(",");
        } else {
            print_comma = 1;
        }
        LOG_INFO("%s", filter->filter);
    }
    LOG_INFO("]\n");
}

