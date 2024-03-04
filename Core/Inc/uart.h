#pragma once
#include "top.h"
#include "uart_ring_buffer.h"


namespace Uart_Rx {

const uint8_t RX_BUFFER_SIZE = 32;



class Uart_Rx {
public:
	void rx_int(char c) {_rx_rb.put(c); return; };
	void putc(char c) {HAL_UART_Transmit(&huart6, (uint8_t *)& c, 1, HAL_MAX_DELAY); return;}
	void flush(void) {return;}
	char getc(void) { return (char) _rx_rb.get(); };
	char peek(void) { return (char) _rx_rb.peek(); };
	char available(void) { return _rx_rb.available(); };
	void rx_flush() {_rx_rb.reset(); return; };
protected:
	Uart_RB::Uart_RB<volatile char, RX_BUFFER_SIZE> _rx_rb;

};

} // End namespace UART

extern Uart_Rx::Uart_Rx Uart;
