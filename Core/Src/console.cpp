/*
 * console.c
 *
 *  Created on: Feb 21, 2024
 *      Author: srodgers
 */


#include "console.h"
#include "logging.h"


const char *TAG = "console";



/* Make printf work with UART6 */
extern "C" {
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart6, (uint8_t *)& ch, 1, HAL_MAX_DELAY);
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
