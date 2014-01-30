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
#define USE_LOG_PREFIX 0
#if USE_LOG_PREFIX
#define DEBUG_PREFIX fprintf(stdout,"DEBUG:")
#define INFO_PREFIX fprintf(stdout,"INFO:")
#define WARN_PREFIX fprintf(stdout,"WARN:")
#define ERROR_PREFIX fprintf(stderr,"ERROR:")
#else //!USE_LOG_PREFIX
#define DEBUG_PREFIX
#define INFO_PREFIX
#define WARN_PREFIX
#define ERROR_PREFIX
#endif //USE_LOG_PREFIX

//VERBOSE_LEVEL
#if LOG_LEVEL >= VERBOSE_LEVEL
#define TESR_LOG_ALLOC(mem, type) do{fprintf(stdout,"ALLOC:0x%zx:%s\n",(size_t)mem, #type);fflush(stdout);}while(0)
#define TESR_LOG_FREE(mem, type) do{fprintf(stdout,"FREE:0x%zx:%s\n",(size_t)mem, #type);fflush(stdout);}while(0)
#define LOG_LOCATION(file, line, function) do{fprintf(stdout,"LOC:%d::%s::%s\n",line,file,function);fflush(stdout);}while(0)//This should never be called directly use LOG_LOC instead
#define LOG_LOC LOG_LOCATION(__FILE__, __LINE__, __FUNCTION__)
#else //!LOG_LEVEL >= VERBOSE_LEVEL
#define LOG_ALLOC(...) do{}while(0)
#define LOG_FREE(...) do{}while(0)
#define LOG_LOC do{}while(0)
#endif //LOG_LEVEL >= VERBOSE_LEVEL

//DEBUG_LEVEL
#if LOG_LEVEL >= DEBUG_LEVEL
#define LOG_DEBUG(format,...) do{DEBUG_PREFIX;fprintf(stdout,format,##__VA_ARGS__);fflush(stdout);}while(0)
#else //!LOG_LEVEL >= DEBUG_LEVEL
#define LOG_DEBUG(...) do{}while(0)
#endif //LOG_LEVEL >= DEBUG_LEVEL

//INFO_LEVEL
#if LOG_LEVEL >= INFO_LEVEL
#define LOG_INFO(format,...) do{INFO_PREFIX;fprintf(stdout,format,##__VA_ARGS__);fflush(stdout);}while(0)
#else //!LOG_LEVEL >= INFO_LEVEL
#define LOG_INFO(...) do{}while(0)
#endif //LOG_LEVEL >= INFO_LEVEL

//WARN_LEVEL
#if LOG_LEVEL >= WARN_LEVEL
#define LOG_WARN(format,...) do{WARN_PREFIX;fprintf(stdout,format,##__VA_ARGS__);fflush(stdout);}while(0)
#else //!LOG_LEVEL >= WARN_LEVEL
#define LOG_WARN(...) do{}while(0)
#endif //LOG_LEVEL >= WARN_LEVEL

//ERROR_LEVEL
#if LOG_LEVEL >= ERROR_LEVEL
#define LOG_ERROR(format,...) do{ERROR_PREFIX;fprintf(stderr,format,##__VA_ARGS__);fflush(stderr);}while(0)
#else //!LOG_LEVEL >= ERROR_LEVEL
#define LOG_ERROR(...) do{}while(0)
#endif //LOG_LEVEL >= ERROR_LEVEL

#endif //TESR_LOGGING_H
