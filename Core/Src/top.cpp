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
#include "i2c_engine.h"
#include "util.h"
#include "uart.h"
#include "city_ring.h"


static const char *TAG = "top";
static const uint8_t UART_RX_BUFFER_SIZE = 32;
static volatile char rx_buffer_uart6[UART_RX_BUFFER_SIZE];

/*
 * Class instantiations
 */
Console::Console Con;
Mfd::MF_decoder Mfr;
Audio::Audio Aud;
I2C_Engine::I2C_Engine I2c;
Util::Util Utility;
Uart_Rx::Uart_Rx Uart;



/*
 * Hook to initialize things before the RTOS tasks are up and running
 */


void Top_init(void) {
	Con.setup();
	Mfr.setup();
	Aud.setup();
	I2c.setup();

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

void Top_send_I2S_Audio_Frame(uint8_t buffer_number){
	Aud.request_block(buffer_number);
}

/*
 * Custom UART6 interrupt handler - See stm32f4xx_it.c, and stm32f4xx_hal_msp.c for user modifications
 */

void Top_Int_Handler_Uart6(void) {
	/* Grab the character from the UART register */
	/* TODO: handle reception errors */
	char c = (char)(huart6.Instance->DR & (uint8_t)0x00FF);
	Uart.rx_int(c);


}


/*
 * Task to process switching functions
 */
typedef struct mf_test_obj {
	bool done;
	bool running;
	uint8_t error_code;
	uint8_t digit_count;
	uint32_t descriptor;
	char digits[Mfd::MF_MAX_DIGITS];

}mf_test_obj;

mf_test_obj mf_test;


void mf_receiver_callback(uint8_t error_code, uint8_t digit_count, char *data) {
	mf_test.error_code = error_code;
	mf_test.digit_count = digit_count;
	strncpy(mf_test.digits, data, Mfd::MF_MAX_DIGITS);
	mf_test.done = true;
}

void audio_callback(uint32_t channel_num) {
	LOG_INFO(TAG, "Audio callback called, channel num: %d", channel_num);
}

static bool audio_seized = false;
static uint32_t ch[2];

void Top_switch_task(void) {

	osDelay(50);

	if(!audio_seized) {
		audio_seized = true;
		ch[0] = Aud.seize();
		ch[1] = Aud.seize();
		Aud.send_loop(ch[0], city_ring, sizeof(city_ring) >>1);
	    Aud.send_call_progress_tones(ch[1], Audio::CPT_DIAL_TONE);
	}



	/* Test code */
	if (!mf_test.running) {
		mf_test.done = false;
		mf_test.descriptor = Mfr.seize(mf_receiver_callback);
		if(!mf_test.descriptor) {
			LOG_ERROR(TAG, "MF Receiver seizure failed");
		}
		else {
			mf_test.running = true;
		}
	}
	else {
		if(mf_test.done) {
			mf_test.running = false;
			if(mf_test.error_code == Mfd::MFE_OK) {
				LOG_INFO(TAG, "Digit count: %d, MF data received: %s", mf_test.digit_count, mf_test.digits);
			}
			else {
				LOG_ERROR(TAG, "MF receiver digit timeout");
			}
			Mfr.release(mf_test.descriptor);

		}

	}
}


/*
 * Task to handle the console
 */

void Top_console_task(void) {
	Con.loop();
}


/*
 * Task to handle the I2C bus
 */

void Top_i2c_task(void) {
	I2c.loop();
}

