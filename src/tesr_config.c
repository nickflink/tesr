// Source: http://www.mail-archive.com/libev@lists.schmorp.de/msg00987.html

#include <tesr_config.h>
#include <getopt.h>
#include <libconfig.h>
#include <stdlib.h>
#include <string.h>
#include <utlist.h>


void log_config(tesr_config_t *tesr_config) {
    printf("tesr_config->port=%d\n", tesr_config->port);
}

void init_config(tesr_config_t *tesr_config, int argc, char **argv) {
    //
    // DEFAULTS
    //
    tesr_config->port = DEFAULT_PORT;
    tesr_config->filters = NULL;
    int configPort = 0;
    int cmdlinePort = 0;
    //
    // Configuration File
    //
    config_t cfg;
    char *local_config_file_name = "udp_net_check.conf";
    char *system_config_file_name = "/etc/udp_net_check.conf";
    /*Initialization */
    config_init(&cfg);
    int config_loaded = 0;
    /* Read the file. If there is an error, report it and exit. */
    if(!config_loaded) {
        config_loaded = config_read_file(&cfg, local_config_file_name);
        if(config_loaded) printf("using config ./%s\n", local_config_file_name);
    }
    if(!config_loaded) {
        config_loaded = config_read_file(&cfg, system_config_file_name);
        if(config_loaded) printf("using config %s\n", system_config_file_name);
    }
    if(!config_loaded) {
        printf("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
    }
    /* Get the configuration file name. */
    if(config_lookup_int(&cfg, "listen_port", &configPort)) {
        printf("\nlisten_port: %d\n", configPort);
        tesr_config->port = configPort;
    } else {
        printf("\nNo 'listen_port' setting in configuration file.");
    }
    config_setting_t *filters = config_lookup(&cfg, "filters");
    int filterIdx = 0;
    if(filters) {
        printf("we have filters\n");
        config_setting_t *filter = config_setting_get_elem(filters, filterIdx++);
        while(filter) {
            printf("we have a filter\n");
            const char *sz_filter = config_setting_get_string(filter);
            if(sz_filter) {
                tesr_filter_t *element = (tesr_filter_t *)malloc(sizeof(tesr_filter_t));
                strncpy(element->filter, sz_filter, INET_ADDRSTRLEN);
                LL_PREPEND(tesr_config->filters, element);
                printf("filter=%s\n", sz_filter);
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
                printf("non-numeric port '%s'", optarg);
            } else {
                printf("command line port =%d\n", cmdlinePort);
                tesr_config->port = cmdlinePort;
            }
            break;
        case '?':
            break;
        default:
            printf("?? getopt returned character code 0%o ??\n", c);
        }
    }
   if (optind < argc) {
        printf("non-option ARGV-elements: ");
        while (optind < argc)
            printf("%s ", argv[optind++]);
        printf("\n");
    }
}
