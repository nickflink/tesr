#ifndef TESR_MACROS
#define TESR_MACROS
#include "tesr_logging.h"

#define TESR_CREATE_FUNC(__TYPE__) \
__TYPE__* create##__TYPE__() { \
__TYPE__ *thiz = NULL; \
thiz = (__TYPE__*)malloc(sizeof(__TYPE__)); \
return thiz; \
LOG_DEBUG("malloc of "##__TYPE__); \
}

#endif //TESR_MACROS
