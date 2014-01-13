#ifndef TESR_LOGGING_H
#define TESR_LOGGING_H

#define VERBOSE_LEVEL 5
#define DEBUG_LEVEL 4
#define INFO_LEVEL 3
#define WARN_LEVEL 2
#define ERROR_LEVEL 1
#define SILENT_LEVEL 0
//Default LogLevel if unset
#ifndef LOG_LEVEL
#error "please set LOG_LEVEL"
#define LOG_LEVEL 3
#endif

//VERBOSE_LEVEL
#if LOG_LEVEL >= VERBOSE_LEVEL
#define LOG_LOCATION(file, line, function) do{printf("LOC:%d::%s::%s\n",line,file,function);}while(0)//This should never be called directly use LOG_LOC instead
#define LOG_LOC LOG_LOCATION(__FILE__, __LINE__, __FUNCTION__)
#else //!LOG_LEVEL >= VERBOSE_LEVEL
#define LOG_LOC do{}while(0);
#endif //LOG_LEVEL >= VERBOSE_LEVEL

//DEBUG_LEVEL
#if LOG_LEVEL >= DEBUG_LEVEL
#define LOG_DEBUG(format,...) do{printf("DEBUG:");printf(format,##__VA_ARGS__);}while(0)
#else //!LOG_LEVEL >= DEBUG_LEVEL
#define LOG_DEBUG(...) do{}while(0)
#endif //LOG_LEVEL >= DEBUG_LEVEL

//INFO_LEVEL
#if LOG_LEVEL >= INFO_LEVEL
#define LOG_INFO(format,...) do{printf("INFO:"format,##__VA_ARGS__);}while(0)
#else //!LOG_LEVEL >= INFO_LEVEL
#define LOG_INFO(...) do{}while(0)
#endif //LOG_LEVEL >= INFO_LEVEL

//WARN_LEVEL
#if LOG_LEVEL >= WARN_LEVEL
#define LOG_WARN(format,...) do{printf("WARN:"format,##__VA_ARGS__);}while(0)
#else //!LOG_LEVEL >= WARN_LEVEL
#define LOG_WARN(...) do{}while(0)
#endif //LOG_LEVEL >= WARN_LEVEL

//ERROR_LEVEL
#if LOG_LEVEL >= ERROR_LEVEL
#define LOG_ERROR(format,...) do{fprintf(stderr,"ERROR:"format,##__VA_ARGS__);}while(0)
#else //!LOG_LEVEL >= ERROR_LEVEL
#define LOG_ERROR(...) do{}while(0)
#endif //LOG_LEVEL >= ERROR_LEVEL

#endif //TESR_LOGGING_H