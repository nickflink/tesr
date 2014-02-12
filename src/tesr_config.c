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
    TESR_LOG_ALLOC(thiz, tesr_config_t);
    return thiz;
}

static int load_config_file(config_t* cfg, const char *file_name) {
    int config_loaded = config_read_file(cfg, file_name);
    if(config_loaded) {
        LOG_INFO("using config %s\n", file_name);
    } else {
        LOG_INFO("no config found at %s\n", file_name);
    }
    return config_loaded;
}

static int int_from_config(config_t* cfg, int *dest, const char *name) {
    int val = 0;
    int success = config_lookup_int(cfg, name, &val);
    if(success) {
        *dest = val;
        LOG_DEBUG("%s: %d\n", name, val);
    } else {
        LOG_DEBUG("No '%s' setting in configuration file.\n", name);
    }
    return success;
}

static int filters_from_config(config_t* cfg, tesr_filter_t *list) {
    int success = 0;
    config_setting_t *filters = config_lookup(cfg, "filters");
    int filterIdx = 0;
    if(filters) {
        LOG_DEBUG("we have filters\n");
        config_setting_t *filter = config_setting_get_elem(filters, filterIdx++);
        while(filter) {
            LOG_DEBUG("we have a filter\n");
            const char *sz_filter = config_setting_get_string(filter);
            if(sz_filter) {
                tesr_filter_t *element = create_filter();
                init_filter(element, sz_filter);
                LL_PREPEND(list, element);
                LOG_DEBUG("filter=%s\n", sz_filter);
                success = 1;
            }
            filter = config_setting_get_elem(filters, filterIdx++);
        }
    }
    return success;
}

static void init_config_from_defaults(tesr_config_t *thiz) {
    //
    // DEFAULTS
    //
    thiz->daemonize = 0;
    thiz->recv_port = DEFAULT_RECV_PORT;
    thiz->ip_rate_limit_max = DEFAULT_IP_RATE_LIMIT_MAX;
    thiz->ip_rate_limit_period = DEFAULT_IP_RATE_LIMIT_PERIOD;
    thiz->ip_rate_limit_prune_mark = DEFAULT_IP_RATE_LIMIT_PRUNE_MARK;
    thiz->num_workers = 0;
    thiz->filters = NULL;
}

static int override_config_from_file(tesr_config_t *thiz) {
    //
    // Configuration File
    //
    //Initialization
    config_t cfg;
    config_init(&cfg);
    //Try local first
    int config_loaded = load_config_file(&cfg, "./tesr.conf");
    if(!config_loaded) {
        //Try sytem second
        config_loaded = load_config_file(&cfg, "/etc/tesr/tesr.conf");
    }
    if(config_loaded) {
        int_from_config(&cfg, &thiz->recv_port, "recv_port");
        int_from_config(&cfg, &thiz->num_workers, "num_workers");
        int_from_config(&cfg, &thiz->ip_rate_limit_max, "ip_rate_limit_max");
        int_from_config(&cfg, &thiz->ip_rate_limit_period, "ip_rate_limit_period");
        int_from_config(&cfg, &thiz->ip_rate_limit_prune_mark, "ip_rate_limit_prune_mark");
        filters_from_config(&cfg, thiz->filters);
    } else {
        LOG_WARN("running without a config file\n");
        LOG_DEBUG("File:%s Line:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
    }
    config_destroy(&cfg);
    return config_loaded;
}

static int override_config_from_args(tesr_config_t *thiz, int argc, char **argv) {
    //
    // OptArg Overrides
    //
    int config_err = CONFIG_ERR_NONE;
    int c;
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"port",    required_argument, 0, 'p'},
            {"workers", required_argument, 0, 'w'},
            {"help",    no_argument, 0, 'h'},
            {"daemon",    no_argument, 0, 'd'},
            {0,         0,                 0,  0 }
        };
        c = getopt_long(argc, argv, "p:w:dh", long_options, &option_index);
        if (c == -1)
            break;
        switch (c) {
        case 'h':
            config_err = CONFIG_ERR_HELP_REQUESTED;
            break;
        case 'd':
            thiz->daemonize = 1;
            break;
        case 'p':
            thiz->recv_port = atoi(optarg);
            if(thiz->recv_port == 0) {
                LOG_ERROR("[KO] non-numeric port '%s'", optarg);
                config_err = CONFIG_ERR_NON_NUMERIC_ARG;
            } else {
                LOG_DEBUG("command line port = %d\n", thiz->recv_port);
            }
            break;
        case 'w':
            thiz->num_workers = atoi(optarg);
            LOG_DEBUG("command line workers = %d\n", thiz->num_workers);
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
    return config_err;
}

void init_config(tesr_config_t *thiz, int argc, char **argv) {
    init_config_from_defaults(thiz);
    override_config_from_file(thiz);
    int config_err = override_config_from_args(thiz, argc, argv);
    if(config_err) {
        //We can exit like this when checking the config as it is only checked in the beginning
        LOG_ERROR("\
\nUSAGE tesr [T]hreaded [E]cho [S]erve[R]\
\n-d (daemonize)\
\n-p --port (override the port)\
\n-w --workers (override the number of workers)\
\n");
        exit(config_err);
    }
}

void destroy_config(tesr_config_t *thiz) {
    if(thiz) {
        destroy_filters_list(thiz->filters);
        TESR_LOG_FREE(thiz, tesr_config_t);
        free(thiz);
        thiz = NULL;
    } else {
        LOG_ERROR("can not free tesr_config_t* as it is NULL");
    }
}

void log_config(tesr_config_t *thiz) {
    LOG_INFO("recv_port=%d\n", thiz->recv_port);
    LOG_INFO("num_workers=%d\n", thiz->num_workers);
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

