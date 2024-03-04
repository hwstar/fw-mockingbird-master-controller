/*
 * console.c
 *
 *  Created on: Feb 21, 2024
 *      Author: srodgers
 */


#include "console.h"
#include "logging.h"
#include "uart.h"


const char *TAG = "console";



/* Make printf work with UART6 */
extern "C" {
int __io_putchar(int ch)
{
    Uart.putc(ch);
	return ch;
}
}



namespace Console {

/*
 * Called by top.cpp to set up the console
 */

void Console::setup(void) {
	Logger.setup();

}

/*
 * Called by CMSIS V2 and top.cpp to process console data
 */

void Console::loop(void) {
	Logger.loop();
}


} /* End namespace console */
