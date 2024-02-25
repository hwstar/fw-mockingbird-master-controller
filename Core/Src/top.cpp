/*
 * top.cpp
 *
 * This file is used to bridge the C code in main.c with the c++ code of the application
 *
 *  Created on: Feb 20, 2024
 *      Author: srodgers
 */
#include "console.h"
#include "mf_decoder.h"
#include "audio.h"

static const char *TAG = "top";

/*
 * Class instantiations
 */
Console::Console Con;
Mfd::MF_decoder Mfr;
Audio::Audio Aud;


/*
 * Hook to initialize things before the RTOS is up and running
 */


void Top_init(void) {
	Con.setup();
	Mfr.setup();
	Aud.setup();

}

/*
 * Called when there is an MF frame to process
 */

void Top_process_MF_frame(uint8_t buffer_number) {
	Mfr.handle_buffer(buffer_number);

}

/*
 * Called when we have to send an I2S audio frame
 */

extern void Top_send_I2S_Audio_Frame(uint8_t buffer_number){
	Aud.request_block(buffer_number);
}

/*
 * Task to process switching functions
 */

void Top_switch_task(void) {
	static bool mfr_running = false;

	osDelay(1);


	/* Test code */
	if (!mfr_running) {
		Mfr.listen_start();
		mfr_running = true;
	}
	else {
		if (Mfr.check_done()) {
			uint8_t error_code;
			if ((error_code = Mfr.get_error_code())) {
				LOG_WARN(TAG, "MF receiver returned error code %d", error_code);
			}
			else {
				LOG_INFO(TAG, "MF receiver returned digits: %s", Mfr.get_received_digits());
			}
			Mfr.listen_stop();
			mfr_running = false;
		}

	}
}


/*
 * Task to handle the console
 */

void Top_console_task(void) {
	Con.loop();
}

