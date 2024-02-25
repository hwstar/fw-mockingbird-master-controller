
#include <math.h>
#include "logging.h"
#include "mf_decoder.h"


/* References:
*
* https://github.com/AI5GW/Goertzel
*
* Notes:

* Tested with a Sage 930A using tone lengths: KP:100ms, Other digits: 70ms
*/

namespace Mfd {
/* MF receiver states */
enum {MFR_DISABLED=0, MFR_WAIT_KP, MFR_KP_SILENCE, MFR_WAIT_DIGIT, MFR_WAIT_DIGIT_SILENCE, MFR_TIMEOUT, MFR_DONE};

const float PI = 3.141529;

/* MF tones */

const uint8_t MFT_700 = 0x20;
const uint8_t MFT_900 = 0x10;
const uint8_t MFT_1100 = 0x08;
const uint8_t MFT_1300 = 0x04;
const uint8_t MFT_1500 = 0x02;
const uint8_t MFT_1700 = 0x01;


/* MF Tone codes */

const uint8_t MFC_KP = (MFT_1100 | MFT_1700);
const uint8_t MFC_ST = (MFT_1500 | MFT_1700);
const uint8_t MFC_STP = (MFT_900 | MFT_1700);
const uint8_t MFC_ST2P = (MFT_1300 | MFT_1700);
const uint8_t MFC_ST3P = (MFT_700 | MFT_1700);
const uint8_t MFC_0 = (MFT_1300 | MFT_1500);
const uint8_t MFC_1 = (MFT_700 | MFT_900);
const uint8_t MFC_2 = (MFT_700 | MFT_1100);
const uint8_t MFC_3 = (MFT_900 | MFT_1100);
const uint8_t MFC_4 = (MFT_700 | MFT_1300);
const uint8_t MFC_5 = (MFT_900 | MFT_1300);
const uint8_t MFC_6 = (MFT_1100 | MFT_1300);
const uint8_t MFC_7 = (MFT_700 | MFT_1500);
const uint8_t MFC_8 = (MFT_900 | MFT_1500);
const uint8_t MFC_9 = (MFT_1100 | MFT_1500);


static const uint8_t mf_decode_table[MF_DECODE_TABLE_SIZE] = { MFC_0, MFC_1, MFC_2, MFC_3, MFC_4, MFC_5, MFC_6, MFC_7, MFC_8, MFC_9, MFC_KP, MFC_ST, MFC_STP, MFC_ST2P, MFC_ST3P};
static const char digit_map[MF_DECODE_TABLE_SIZE] =          { '0',   '1',   '2',   '3',   '4',   '5',   '6',   '7',   '8',   '9',   '*',    '#',    'A',     'B',      'C'  };


static const float frequencies[NUM_MF_FREQUENCIES] = {1700.0, 1500.0, 1300.0, 1100.0, 900.0, 700.0};



void MF_decoder::setup() {

	/* Create mutex to protect mf receiver data between tasks */
		static const osMutexAttr_t mfd_mutex_attr = {
			"MFDecoderMutex",
			osMutexRecursive | osMutexPrioInherit,
			NULL,
			0U
		};

		this->_lock = osMutexNew(&mfd_mutex_attr);

	/* Initialize the goertzel filter data */
	for (int i = 0; i < NUM_MF_FREQUENCIES; i++) {
		_goertzel_data[i].power = 0.0;
    	_goertzel_data[i].coeff_k = 2.0 * cos((2.0 * PI * frequencies[i]) / MF_SAMPLE_RATE);
	}
	

    /* Start the DMA for the MF receiver */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *) this->_mf_adc_buffer, MF_ADC_BUF_LEN);

	
}

/*
* Start listening for MF signalling
*/

void MF_decoder::listen_start() {
	osMutexAcquire(this->_lock, osWaitForever);
	this->_mf_data.error_code = 0;
	this->_mf_data.tone_digit = false;
	this->_mf_data.digit_count = 0;
	this->_mf_data.tone_block_count = 0;
	this->_mf_data.timer = 0;
	this->_mf_data.debug_info[0] = 0;
	this->_mf_data.state = MFR_WAIT_KP;

	/* Start sending conversion requests to the ADC */
    if (HAL_TIM_OC_Start(&htim3, TIM_CHANNEL_2) != HAL_OK) {
		Error_Handler();
	}

  	if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1 ) != HAL_OK) {
  		Error_Handler();
  	}
  	osMutexRelease(this->_lock);
}


/*
* Stop listening for MF signalling
*/

void MF_decoder::listen_stop() {
	osMutexAcquire(this->_lock, osWaitForever);
    /* Stop sending conversion requests to the ADC */
  	if (HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1 ) != HAL_OK) {
  		Error_Handler();
  	}

	if (HAL_TIM_OC_Stop(&htim3, TIM_CHANNEL_2) != HAL_OK) {
		Error_Handler();
	}
	osMutexRelease(this->_lock);
}


bool MF_decoder::check_done() {
	bool res = false;
	osMutexAcquire(this->_lock, osWaitForever);
	if(this->_mf_data.state == MFR_DONE) {
		res = true;
	}
	osMutexRelease(this->_lock);
	return res;
}

int MF_decoder::get_error_code() {
	int error_code;
	error_code = this->_mf_data.error_code;
	return error_code;

}


char * MF_decoder::get_received_digits() {
	this->_mf_data.digits[MF_MAX_DIGITS - 1] = 0; /* Ensure string is terminated */
	return (char *)this->_mf_data.digits;
}

void MF_decoder::handle_buffer(uint8_t buffer_no) {
	float max = 1.0;
	float min = -1.0;
	uint16_t *buffer = (buffer_no) ? this->_mf_adc_buffer + MF_FRAME_SIZE : this->_mf_adc_buffer;

	/* HAL_GPIO_WritePin(LEDN_GPIO_Port, LEDN_Pin, GPIO_PIN_RESET); */

	/* PASS 1: Convert to bipolar format, and record min and max values */
	for (int sample_index = 0; sample_index < MF_FRAME_SIZE; sample_index++) {
		/* center around 0 */
		float val = ((float) buffer[sample_index]) + MIN_ADC;

		/* Scale to range -1 to 1 */
		val /= -MIN_ADC;

		if (val > max) {
			max = val;
		}
		if ( val < min ) {
			min = val;
		}
		this->_goertzel_block[sample_index] = val;
	}

	/* PASS 2: Use the min and max values to remove any DC offset */
	float dc_offset = (((1.0 - max)) - (1.0 - fabs(min)));

	for (int sample_index = 0; sample_index < MF_FRAME_SIZE; sample_index++) {
		this->_goertzel_block[sample_index] -= dc_offset;
	}

	/* PASS 3: Decode MF tones */

	/* Clear previous values for all goertzel tone decoders */
	for (int mf_freq_index = 0; mf_freq_index < NUM_MF_FREQUENCIES; mf_freq_index++) {
		this->_goertzel_data[mf_freq_index].q1 = this->_goertzel_data[mf_freq_index].q2 = 0.0;
	}

    /* Go through the frame and calculate the goertzel data values */
	for (int sample_index = 0; sample_index < MF_FRAME_SIZE; sample_index++) { 
		register float s = this->_goertzel_block[sample_index];
		for (int mf_freq_index = 0; mf_freq_index < NUM_MF_FREQUENCIES; mf_freq_index++) {
			float q0 = this->_goertzel_data[mf_freq_index].coeff_k * this->_goertzel_data[mf_freq_index].q1 -
					this->_goertzel_data[mf_freq_index].q2 + s;
			this->_goertzel_data[mf_freq_index].q2 = this->_goertzel_data[mf_freq_index].q1;
			this->_goertzel_data[mf_freq_index].q1 = q0;




		}
	}
	
	/* Calculate power from the goertzel data values */
	for (int mf_freq_index = 0; mf_freq_index < NUM_MF_FREQUENCIES; mf_freq_index++) {
		this->_goertzel_data[mf_freq_index].power = sqrt(this->_goertzel_data[mf_freq_index].q1 * this->_goertzel_data[mf_freq_index].q1 +
				this->_goertzel_data[mf_freq_index].q2 * this->_goertzel_data[mf_freq_index].q2 -
				this->_goertzel_data[mf_freq_index].coeff_k * this->_goertzel_data[mf_freq_index].q1 * this->_goertzel_data[mf_freq_index].q2);


	}

	/* Check for tones */
	bool silence = false;
	bool valid_code = false;

	uint8_t mf_code = 0;


    /* Test to see if MF tone component is above the threshold */
	for(uint8_t tone_index = 0; tone_index < NUM_MF_FREQUENCIES; tone_index++) {

		if (this->_goertzel_data[tone_index].power > SILENCE_THRESHOLD) {
			mf_code |= (1 << tone_index);

		}
	}
	/* Count the number of tones present */
	uint8_t tones_present = 0;
	for (uint8_t tone_index = 0; tone_index < NUM_MF_FREQUENCIES; tone_index++) {
		if (mf_code & (1 << tone_index)) {
			tones_present++;
		}
	}

	/* Check for silence */
	if (tones_present == 0) {
		silence = true;
	} 
	/* Check for exactly 2 tones */
	else if (tones_present == 2){
		valid_code = true;
	}

	/* Decoder state machine */

	osMutexAcquire(this->_lock, osWaitForever); /* Get the lock */

	switch(this->_mf_data.state) {
		case MFR_WAIT_KP:
			if (!silence && valid_code == true && mf_code == MFC_KP) {
				if (this->_mf_data.tone_block_count >= MIN_KP_GATE_BLOCK_COUNT){
					this->_mf_data.digit_count = 0;
					this->_mf_data.timer = 0;
					this->_mf_data.state = MFR_KP_SILENCE;
				}
				else {
					this->_mf_data.tone_block_count++;
				}
			}
			else {
				this->_mf_data.debug_info[0] = 0;
			}
			break;

		case MFR_KP_SILENCE:
			if (silence) {
				this->_mf_data.state = MFR_WAIT_DIGIT;
				this->_mf_data.tone_block_count = 0;
				this->_mf_data.timer = 0;
			}
			else {
				this->_mf_data.timer++;
				if (this->_mf_data.timer >= MF_INTERDIGIT_TIMEOUT) {
					this->_mf_data.state = MFR_TIMEOUT;
				}
			}
			break;

		case MFR_WAIT_DIGIT:
			if (!silence && valid_code == true) {
				if (this->_mf_data.tone_block_count >= MIN_DIGIT_BLOCK_COUNT - 1) {
					uint8_t tone_number;
					for (tone_number = 0; tone_number < MF_DECODE_TABLE_SIZE; tone_number++) {
						if (mf_code == mf_decode_table[tone_number]) {
							break;
						}
					}
					if (tone_number < MF_DECODE_TABLE_SIZE) {
						this->_mf_data.tone_digit = digit_map[tone_number];
						this->_mf_data.state = MFR_WAIT_DIGIT_SILENCE;
						this->_mf_data.timer = 0;
					}
				}
				else {
					this->_mf_data.tone_block_count++;
				}
			}
			else {
				this->_mf_data.timer++;
				if (this->_mf_data.timer >= MF_INTERDIGIT_TIMEOUT) {
					this->_mf_data.state = MFR_TIMEOUT;
				}
			}
			break;

		case MFR_WAIT_DIGIT_SILENCE:
			if (silence) {
				if (this->_mf_data.tone_digit != '#') {
					this->_mf_data.state = MFR_WAIT_DIGIT;
					this->_mf_data.tone_block_count = 0;
					this->_mf_data.timer = 0;
					if (this->_mf_data.digit_count < MF_MAX_DIGITS) {
						this->_mf_data.digits[this->_mf_data.digit_count] = this->_mf_data.tone_digit;
					}
					this->_mf_data.digit_count++;
				}
				else {
					this->_mf_data.state = MFR_DONE;
					this->_mf_data.digits[this->_mf_data.digit_count] = 0;
				}
			}
			else {
				this->_mf_data.timer++;
				if (this->_mf_data.timer >= MF_INTERDIGIT_TIMEOUT) {
					this->_mf_data.state = MFR_TIMEOUT;
				}
			}
			break;

		case MFR_TIMEOUT:
			this->_mf_data.error_code = MFE_TIMEOUT;
			this->_mf_data.state = MFR_DONE;
			break;

		case MFR_DONE:
			break;


		default:
			this->_mf_data.state = MFR_DONE;
			break;



	}
	osMutexRelease(this->_lock); /* Release the lock */

	/* HAL_GPIO_WritePin(LEDN_GPIO_Port, LEDN_Pin, GPIO_PIN_SET); */
}

} // End Namespace MFR



