#ifndef TESR_SUPERVISOR_H
#define TESR_SUPERVISOR_H

#include "tesr_types.h"

supervisor_thread_t *create_supervisor_instance();
int init_supervisor(supervisor_thread_t *thiz, tesr_config_t *config);
void destroy_supervisor(supervisor_thread_t *thiz);
void log_supervisor(supervisor_thread_t *thiz);

void supervisor_thread_run(supervisor_thread_t *thiz);
supervisor_thread_t *get_supervisor_thread();
#endif //TESR_SUPERVISOR_H
