#include "logging.h"

namespace LOGGING {

static const char *TAG = "logger";

const char *log_level_strings[MAX_LOG_LEVEL] = {
    "ERROR",
    "WARN",
    "NOTICE",
    "INFO",
    "DEBUG"
};


void Logging::_xmit_logitem(const char *tag, uint8_t level, uint32_t timestamp, const char *str, uint32_t line) {
    if(level > MAX_LOG_LEVEL) {
        level = 0; /* Protect against bad log level being passed in. */
    }

    /* Convert time stamp to H:M:S.MS format */
    uint32_t hours = timestamp / 3600000;
    timestamp %= 36000000;
    uint8_t minutes = timestamp / 60000;
    timestamp %= 60000;
    uint8_t seconds = timestamp / 1000;
    timestamp %= 1000;

    printf("[%lu:%02u:%02u.%03lu] LOG_%s(%s.%ld):%s\r\n", hours, minutes, seconds, timestamp, log_level_strings[level], tag, line, str);
}


void Logging::log(const char *tag, uint8_t level, uint32_t timestamp, uint32_t line, const char *format, ...) {
   
	osMutexAcquire(this->_ring_buffer.lock, osWaitForever);
    if(this->_buffer_full()){
        this->_ring_buffer.overflow_error = true;
    }
    else {
        va_list alp;
        va_start(alp, format);

        logItem *li = &this->_ring_buffer.messages[_ring_buffer.head];
        vsnprintf(li->log_message, MAX_LOG_SIZE, format, alp);
        va_end(alp);
        li->log_message[MAX_LOG_SIZE - 1] = 0;
        strncpy(li->tag, tag, MAX_TAG_SIZE);
        li->tag[MAX_TAG_SIZE - 1] = 0;
        li->level = level;
        li->timestamp = timestamp;
        li->line = line;
        /* Advance to next position in ring buffer */
        _ring_buffer.head = this->_buffer_next(_ring_buffer.head);

    }
    osMutexRelease(this->_ring_buffer.lock);

}

void Logging::setup(void) {

	/* Create mutex to protect ring buffer data between tasks */
	static const osMutexAttr_t logging_mutex_attr = {
		"LoggingMutex",
		osMutexRecursive | osMutexPrioInherit,
		NULL,
		0U
	};

	this->_ring_buffer.lock = osMutexNew(&logging_mutex_attr);


}

void Logging::loop() {

	static logItem log_item;
	bool overflow_error = false;
	bool log_message = false;

	osMutexAcquire(this->_ring_buffer.lock, osWaitForever);
	if(this->_ring_buffer.overflow_error) {
		overflow_error = true;
		this->_ring_buffer.overflow_error = false;
	}
	else if (!this->_buffer_empty()) {
		logItem *li = &this->_ring_buffer.messages[_ring_buffer.tail];
		/* Indicate we have a log message to print */
		log_message = true;
		/* Copy log message out of buffer for printing */
		memcpy(&log_item, li, sizeof(logItem));
		/* Advance to next position in the ring buffer */
		_ring_buffer.tail = this->_buffer_next(_ring_buffer.tail);

	}
	osMutexRelease(this->_ring_buffer.lock);

	if(overflow_error) { /* Test for log buffer overflow */
		this->_xmit_logitem(TAG, LOGGING_ERROR, osKernelGetTickCount(), "*** Log buffer overflow ***", __LINE__);

	}
	else if(log_message == true) { /* Test for presence of log message */
		this->_xmit_logitem(log_item.tag, log_item.level, log_item.timestamp, log_item.log_message, log_item.line);
	}



}

} /* End Namespace LOGGING */

LOGGING::Logging Logger;


