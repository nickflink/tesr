// Source: http://www.mail-archive.com/libev@lists.schmorp.de/msg00987.html

#include "ues-config.h"
#include <getopt.h>
#include <libconfig.h>
#include <stdlib.h>

void log_config(ues_config_t *ues_config) {
    printf("ues_config->port=%d\n", ues_config->port);
}

void init_config(ues_config_t *ues_config, int argc, char **argv) {
    //
    // DEFAULTS
    //
    ues_config->port = DEFAULT_PORT;
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
        ues_config->port = configPort;
    } else {
        printf("\nNo 'listen_port' setting in configuration file.");
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
                ues_config->port = cmdlinePort;
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
