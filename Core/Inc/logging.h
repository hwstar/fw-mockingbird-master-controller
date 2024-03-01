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

const uint16_t MAX_LOG_SIZE = 128;
const uint8_t MAX_LOG_BUFFER_DEPTH = 8; /* Must be power of 2 */
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

typedef struct ringBuffer {
	osMutexId_t lock;
    uint8_t head;
    uint8_t tail;
    bool overflow_error;
    logItem messages[MAX_LOG_BUFFER_DEPTH];

} ringBuffer;


class Logging {
    public:
    void log(const char *tag, uint8_t level, uint32_t line, const char *format, ...);
    void setup(void);
    void loop(void);



    protected:
    ringBuffer _ring_buffer;

    bool _buffer_full(void) { return (((this->_ring_buffer.head + 1) & (MAX_LOG_BUFFER_DEPTH - 1)) == this->_ring_buffer.tail); }
    bool _buffer_empty(void) { return (this->_ring_buffer.tail == this->_ring_buffer.head); }
    uint8_t _buffer_next(uint8_t buffer) { return (buffer + 1) & (MAX_LOG_BUFFER_DEPTH - 1); }
    void _xmit_logitem(const char *tag, uint8_t level, uint32_t timestamp, const char *str, uint32_t line);
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







