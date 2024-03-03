#pragma once
#include "console.h"





/*
* Default logging level
*/

#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARN 1
#define LOG_LEVEL_NOTICE 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4

#define LOG_LEVEL LOG_LEVEL_DEBUG


namespace LOGGING {

const uint8_t LOG_QUEUE_DEPTH = 8;
const uint16_t MAX_LOG_SIZE = 80;
const uint8_t MAX_TAG_SIZE = 16;
const uint8_t MAX_LOG_LEVEL = 5;

enum {LOGGING_ERROR=0, LOGGING_WARN, LOGGING_NOTICE, LOGGING_INFO, LOGGING_DEBUG};


typedef struct logItem {
    uint8_t level;
    uint32_t timestamp;
    uint32_t line;
    char tag[MAX_TAG_SIZE];
    char log_message[MAX_LOG_SIZE];
} logItem;



class Logging {
    public:
    void log(const char *tag, uint8_t level, uint32_t line, const char *format, ...);
    void setup(void);
    void loop(void);



    protected:
    void _xmit_logitem(const char *tag, uint8_t level, uint32_t timestamp, const char *str, uint32_t line);

    osMessageQueueId_t _queue_logging_handle;
    bool _queue_overflow;
};

} /* End Namespace LOGGING */

extern LOGGING::Logging Logger;


#define LOG_ERROR(tag, format, ...) Logger.log(tag, LOGGING::LOGGING_ERROR, __LINE__, format __VA_OPT__(,) __VA_ARGS__)
#if LOG_LEVEL_WARN <= LOG_LEVEL
#define LOG_WARN(tag, format, ...) Logger.log(tag, LOGGING::LOGGING_WARN, __LINE__, format __VA_OPT__(,) __VA_ARGS__)
#else
#define LOG_WARN(tag, format, ...)
#endif
#if LOG_LEVEL_NOTICE <= LOG_LEVEL
#define LOG_NOTICE(tag, format, ...) Logger.log(tag, LOGGING::LOGGING_NOTICE, __LINE__, format __VA_OPT__(,) __VA_ARGS__)
#else
#define LOG_NOTICE(tag, format, ...)
#endif
#if LOG_LEVEL_INFO <= LOG_LEVEL
#define LOG_INFO(tag, format, ...) Logger.log(tag, LOGGING::LOGGING_INFO, __LINE__, format __VA_OPT__(,) __VA_ARGS__)
#else
#define LOG_INFO(tag, format, ...)
#endif
#if LOG_LEVEL_DEBUG <= LOG_LEVEL
#define LOG_DEBUG(tag, format, ...) Logger.log(tag, LOGGING::LOGGING_DEBUG, __LINE__, format __VA_OPT__(,) __VA_ARGS__)
#else
#define LOG_DEBUG(tag, format, ...)
#endif







