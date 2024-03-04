#pragma once
#include <stdint.h>

namespace Uart_RB {


template <typename T, int size> class Uart_RB {
public:
	/* When placed in the .cpp file, the linker can't find a reference to put() so it is in-lined here */
	inline void put(char c) {if(this->_check_full()) {return;} else {uint32_t next = this->_next_position(this->_head); this->_buffer[this->_head] = c; this->_head = next;}};
	bool available(void);
	T get(void);
	T peek(void);
	void reset(void);


protected:
	inline uint32_t _next_position(uint32_t cur_pos) {return (((cur_pos + 1) >= size) ? 0 : cur_pos + 1 );};
	inline bool _check_full(void) {return(_next_position(_head) == _tail);};
	inline bool _check_empty(void){return(_tail == _head);};
	volatile uint32_t _head;
	volatile uint32_t _tail;
	T _buffer[size];

};

} /* End namespace UART_RB */



