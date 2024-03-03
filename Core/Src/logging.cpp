#include "top.h"
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


void Logging::log(const char *tag, uint8_t level, uint32_t line, const char *format, ...) {

	va_list alp;
	va_start(alp, format);

	logItem li = {0};
	vsnprintf(li.log_message, MAX_LOG_SIZE, format, alp);
	va_end(alp);
	li.log_message[MAX_LOG_SIZE - 1] = 0;
	strncpy(li.tag, tag, MAX_TAG_SIZE);
	li.tag[MAX_TAG_SIZE - 1] = 0;
	li.level = level;
	li.timestamp = osKernelGetTickCount();
	li.line = line;
	osStatus_t res = osMessageQueuePut(this->_queue_logging_handle, &li, 0U, 0U);
	if(res == osErrorResource) {
		this->_queue_overflow = true;
	}
	else if (res == osErrorParameter) {
		/* Program bug */
		Error_Handler();
	}
}

void Logging::setup(void) {

	/* Definitions for Logging Queue */

	const osMessageQueueAttr_t Queue_Logging_Attributes = {
	  .name = "Queue_Logging"
	};

	/* Create logging queue */

	this->_queue_logging_handle = osMessageQueueNew (LOG_QUEUE_DEPTH, sizeof(logItem), &Queue_Logging_Attributes);

	this->_queue_overflow = false;

}

void Logging::loop() {

	logItem li;

	osStatus_t res = osMessageQueueGet(this->_queue_logging_handle, &li, NULL, 0U);

	if (res == osOK) {
		this->_xmit_logitem(li.tag, li.level, li.timestamp, li.log_message, li.line);
	}
	else if (res == osErrorParameter) {
		/* Program Bug if this happens */
		Error_Handler();
	}



	if (this->_queue_overflow) { /* Test for log buffer overflow */
		this->_queue_overflow = false;
		this->_xmit_logitem(TAG, LOGGING_ERROR, osKernelGetTickCount(), "*** Log buffer overflow ***", __LINE__);

	}




}

} /* End Namespace LOGGING */

LOGGING::Logging Logger;


