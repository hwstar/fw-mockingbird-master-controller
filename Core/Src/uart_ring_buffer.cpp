#include "top.h"
#include "uart_ring_buffer.h"

namespace Uart_RB {

/*
 * Get a character from the ring buffer
 */

template <typename T, int size>
T Uart_RB<T, size>::get() {
	if(this->_check_empty()) {
		return 0;
	}
	uint32_t next = this->_next_position(this->_tail);
	char res = this->_buffer[this->_tail];
	this->_tail = next;
	return res;
}

/*
 * See if there is something in the ring buffer
 */

template <typename T, int size>
bool Uart_RB<T, size>::available(void) {
	return !(this->_check_empty());
}

/*
 * Return the next character in the ring buffer.
 * The buffer must not be empty for this to work properly.
 */

template <typename T, int size>
T Uart_RB<T, size>::peek(void) {
	return this->_buffer[this->_tail];
}

/*
 * Reset the ring buffer
 */

template <typename T, int size>
void Uart_RB<T, size>::reset(void) {
	this->_head = this->_tail = 0;

}

} /* End Namespace UART_RB */


