#ifndef UES_CONFIG_H
#define UES_CONFIG_H

#define DEFAULT_PORT 7
typedef struct ues_config_t {
    int port;
} ues_config_t;
void log_config(ues_config_t *ues_config);
void init_config(ues_config_t *ues_config, int argc, char **argv);
#endif //UES_CONFIG_H
