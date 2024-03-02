#include "top.h"
#include "audio.h"
#include "logging.h"
#include "util.h"
#include <math.h>

namespace Audio {

static const char *TAG = "audio";

#include "sine.h"





/*
 * Start the DMA to the I2S audio device
 */

void Audio::_dma_start(void) {
	/* Start up the DMA */
	HAL_I2S_Transmit_DMA(&hi2s2, (uint16_t *) this->lr_audio_output_buffer, LR_AUDIO_BUFFER_SIZE * 2);
}

/*
 * Stop the DMA to the I2S Audio device
 */

void Audio::_dma_stop(void) {
	HAL_I2S_DMAStop(&hi2s2);
}

/*
 * Set up the generation of a single tone
 */

void Audio::_generate_tone(ChannelInfo *channel_info, float freq, float level) {
	this->_generate_dual_tone(channel_info, freq, 0.0, level, 0.0);
}
/*
 * Set up the generation of a dual tone
 */

void Audio::_generate_dual_tone(ChannelInfo *channel_info, float freq1, float freq2, float db_level1, float db_level2) {

	channel_info->f1 = freq1;
	channel_info->f2 = freq2;
	// Treat as DbV here.
	channel_info->f1_level = pow(10,(db_level1/20));
	channel_info->f2_level = pow(10,(db_level2/20));

	channel_info->phase_accum[0] = 0;
	channel_info->phase_accum[1] = 0;
	/*
	 *  Formula:
	 *
	 *  (2^N * Fout)/Fs
	 *  Where:
	 *  Fs = sample frequency
	 *  Fout is the desired output frequency
	 *  2^N is maximum value of the phase accumulator + 1
	 *
	 */
	channel_info->tuning_word[0] = (uint16_t) (((PHASE_ACCUM_MODULO_N) * ((float) channel_info->f1)) / ((float) SAMPLE_FREQ_HZ));
	channel_info->tuning_word[1] = (uint16_t) (((PHASE_ACCUM_MODULO_N) * ((float) channel_info->f2)) / ((float) SAMPLE_FREQ_HZ));
}




/*
 * Call to return the next computed value
 */


int16_t Audio::_next_tone_value(ChannelInfo *channel_info) {


	int16_t rawval_f1 = 0;
	int16_t rawval_f2 = 0;

	if(channel_info->f1 != 0.0) {
		uint16_t sine_table_index = (uint16_t) (channel_info->phase_accum[0] >> PHASE_ACCUMULATOR_TRUNCATION); /* Get sine table index for F1 */
		rawval_f1 = lut[sine_table_index];
		/* For signed audio, positive and negative excursions need to be handled differently */
		if(rawval_f1 >= 0){
			rawval_f1 = rawval_f1 * channel_info->f1_level;
		}
		else {
			rawval_f1 = -(-rawval_f1 * channel_info->f1_level);
		}
	}

	if(channel_info->f2 != 0.0) {
		uint16_t sine_table_index = (uint16_t) (channel_info->phase_accum[1] >> PHASE_ACCUMULATOR_TRUNCATION); /* Get sine table index for F2 */
		rawval_f2 = lut[sine_table_index];
		/* For signed audio, positive and negative excursions need to be handled differently */
		if(rawval_f2 >= 0){
			rawval_f2 = rawval_f2 * channel_info->f2_level;
		}
		else {
			rawval_f2 = -(-rawval_f2 * channel_info->f2_level);
		}
	}

	/* Advance to next phase accumulator value */
	for(int i = 0; i < 2; i++) {
		channel_info->phase_accum [i] = (channel_info->phase_accum[i] + channel_info->tuning_word[i]) & (PHASE_ACCUMULATOR_MASK);
	}



	return rawval_f1 + rawval_f2;
}


uint32_t Audio::_get_mf_tone_duration(uint8_t mf_digit) {
	uint32_t duration_ms;
	switch(mf_digit) {
		case MF_KP:
			duration_ms = MF.kp_active_time_ms;
			break;

		case MF_ST:
		case MF_STP:
		case MF_ST2P:
		case MF_ST3P:
			duration_ms = MF.st_active_time_ms;
			break;

		default:
			duration_ms = MF.active_time_ms;
			break;
	}
	return this->_convert_ms(duration_ms);
}

/*
 * Convert ASCII tone string to binary representation
 */


bool Audio::_convert_digit_string(ChannelInfo *ch_info, const char *digits, bool is_mf) {
	uint8_t i, code;

	if(!ch_info || !digits) {
		LOG_ERROR(TAG, "Null argument(s) passed");
		return false;
	}

	ch_info->digit_string_index = 0;
	ch_info->digit_string_length = 0;


	for(i = 0; i < strlen(digits); i++) {
		if(i >= DIGIT_STRING_MAX_LENGTH) {
			LOG_ERROR(TAG, "Digit string exceeds maximum length");
			return false;
		}
		if(digits[i] == 0) { /* End of digit string */
			break;
		}
		switch(digits[i]) {
			case '*':
				code = 0x0A;
				break;

			case '#':
				code = 0x0B;
				break;

			case 'A':
				code = 0x0C;
				break;

			case 'B':
				code = 0x0D;
				break;

			case 'C':
				code = 0x0E;
				break;

			case 'D':
				if(!is_mf) {
					code = 0x0F;
				}
				else {
					LOG_ERROR(TAG,"Invalid mf tone digit");
					return false;
				}
				break;

			default:
				/* Digits 0 through 9 */
				if((digits[i] >= '0') && (digits[i] <= '9')) {
					code = digits[i] - 0x30;
				}
				else {
					LOG_ERROR(TAG, "Invalid tone digit");
					return false;
				}
				break;
		}
		/* Store the code digit in the dial string */
		ch_info->digit_string[ch_info->digit_string_length++] = code;
	}
	return true;

}

/*
 * Validate a descripter passed in by the caller
 */

bool Audio::_validate_channel(uint32_t channel_number) {
	if(channel_number > NUM_AUDIO_CHANNELS){
		return false;
	}
	return true;
}

/*
 * Called to set up the audio processing
 * before the RTOS starts running
 */

void Audio::setup(void) {

	/* Create mutex to protect audio data between tasks */
	static const osMutexAttr_t aud_mutex_attr = {
		"AudioMutex",
		osMutexRecursive | osMutexPrioInherit,
		NULL,
		0U
	};
	/* Create intertask lock */
	this->_lock = osMutexNew(&aud_mutex_attr);

	/* Start I2S DMA */

	this->_dma_start();


}

/*
 * Seize an audio channel and return a channel number.
 *
 * If no channel is available, return 0.
 */

uint32_t Audio::seize(void) {
	uint32_t channel_number;
	osMutexAcquire(this->_lock, osWaitForever); /* Get the lock */

	for(channel_number = 1; this->_validate_channel(channel_number); channel_number++) {
		ChannelInfo *ch_info = &this->channel_info[channel_number - 1];
		if(!ch_info->in_use) {
			ch_info->in_use = true;
			break;
		}
	}
	if(!this->_validate_channel(channel_number)) {
		channel_number = 0;
	}

	osMutexRelease(this->_lock); /* Release the lock */
	return channel_number;
}

/*
 * Release an audio channel
 *
 * Return true if successful
 */


bool Audio::release(uint32_t channel_number) {

	if(!this->_validate_channel(channel_number)){
		return false;
	}

	osMutexAcquire(this->_lock, osWaitForever); /* Get the lock */
	ChannelInfo *ch_info = &this->channel_info[channel_number - 1];
	/* Release channel if in use */
	if(ch_info->in_use) {
		ch_info->state = AS_IDLE;
		ch_info->in_use = false;
	}
	osMutexRelease(this->_lock); /* Release the lock */


	return true;

}

/*
 * Send call progress tones.
 * Will continue to call send progress tones until stop() is called.
 */
bool Audio::send_call_progress_tones(uint32_t channel_number, uint8_t type) {

	if(type >= CPT_MAX){
		return false;
	}
	if(!this->_validate_channel(channel_number)) {
		return false;
	}

	osMutexAcquire(this->_lock, osWaitForever); /* Get the lock */

	ChannelInfo *ch_info = &this->channel_info[channel_number - 1];

	/* Set the call progress tone type */
	switch(type) {
		case CPT_DIAL_TONE:
			ch_info->state = AS_GEN_DIAL_TONE;
			break;

		case CPT_BUSY:
			ch_info->state = AS_GEN_BUSY_TONE;
			break;

		case CPT_CONGESTION:
			ch_info->state = AS_GEN_CONGESTION_TONE;
			break;

		case CPT_RINGING:
			ch_info->state = AS_GEN_RINGING_TONE;
			break;
	}

	osMutexRelease(this->_lock); /* Release the lock */
	return true;

}
/*
 * Send a set of digits using MF tones.
 * Call the callback function when the all the digits are sent
 */
bool Audio::send_mf(uint32_t channel_number, const char *digit_string, void (*callback)(uint32_t channel_number)) {

	if(!this->_validate_channel(channel_number)) {
		return false;
	}

	if((!digit_string) || (!callback))
		return false;

	osMutexAcquire(this->_lock, osWaitForever); /* Get the lock */
	ChannelInfo *ch_info = &this->channel_info[channel_number - 1];

	int len = strlen(digit_string);
	ch_info->digit_string_length = 0;
	for (int i = 0; i < len; i++) {
		switch (digit_string[i]) {
			case 0:
				break;

			case '*':
				ch_info->digit_string[i] = 0x0a;
				ch_info->digit_string_length++;
				break;

			case '#':
				ch_info->digit_string[i] = 0x0b;
				ch_info->digit_string_length++;
				break;

			case 'A':
				ch_info->digit_string[i] = 0x0c;
				ch_info->digit_string_length++;
				break;

			case 'B':
				ch_info->digit_string[i] = 0x0d;
				ch_info->digit_string_length++;
				break;

			case 'C':
				ch_info->digit_string[i] = 0x0e;
				ch_info->digit_string_length++;
				break;

			default:
				if((digit_string[i] >= '0' || digit_string[i] <= '9')){
					ch_info->digit_string[i] = digit_string[i] - 0x30;
					ch_info->digit_string_length++;
				}
				break;
		}
		if(!digit_string[i])
			break;
	}



	ch_info->callback = callback;
	ch_info->state = AS_SEND_MF;

	osMutexRelease(this->_lock); /* Release the lock */
	return true;


}

/*
 * Send a set of digis using DTMF tones.
 * Call the callback function when the all the digits are sent
 */
bool Audio::send_dtmf(uint32_t channel_number, const char *digit_string, void (*callback)(uint32_t channel_number)) {
	if(!this->_validate_channel(channel_number)) {
		return false;
	}

	if((!digit_string) || (!callback))
		return false;


	osMutexAcquire(this->_lock, osWaitForever); /* Get the lock */
	ChannelInfo *ch_info = &this->channel_info[channel_number - 1];

	int len = strlen(digit_string);
	ch_info->digit_string_length = 0;
	for (int i = 0; i < len; i++) {
		switch (digit_string[i]) {
			case 0:
				break;

			case '*':
				ch_info->digit_string[i] = 0x0a;
				ch_info->digit_string_length++;
				break;

			case '#':
				ch_info->digit_string[i] = 0x0b;
				ch_info->digit_string_length++;
				break;

			case 'A':
				ch_info->digit_string[i] = 0x0c;
				ch_info->digit_string_length++;
				break;

			case 'B':
				ch_info->digit_string[i] = 0x0d;
				ch_info->digit_string_length++;
				break;

			case 'C':
				ch_info->digit_string[i] = 0x0e;
				ch_info->digit_string_length++;
				break;

			case 'D':
				ch_info->digit_string[i] = 0x0f;
				ch_info->digit_string_length++;
				break;

			default:
				if((digit_string[i] >= '0' || digit_string[i] <= '9')){
					ch_info->digit_string[i] = digit_string[i] - 0x30;
					ch_info->digit_string_length++;
				}
				break;
		}
		if(!digit_string[i])
			break;
	}
	ch_info->callback = callback;
	ch_info->state = AS_SEND_DTMF;

	osMutexRelease(this->_lock); /* Release the lock */
	return true;
}

/*
 * Send an audio sample
 * Call the callback function when the sample is completely sent
 */

bool Audio::send(uint32_t channel_number, const int16_t *samples, uint32_t length, void (*callback)(uint32_t channel_number)) {

	if (!this->_validate_channel(channel_number)) {
		return false;
	}

	if ((!samples) || (!callback)) {
		return false;
	}

	osMutexAcquire(this->_lock, osWaitForever); /* Get the lock */

	ChannelInfo *ch_info = &this->channel_info[channel_number - 1];

	ch_info->callback = callback;
	ch_info->audio_sample_size = length;
	ch_info->audio_sample = samples;
	ch_info->state = AS_SEND_AUDIO;

	osMutexRelease(this->_lock); /* Release the lock */
	return true;

}

/*
 * Send an audio loop.
 *
 * Will continue to send the audio loop until the stop function is called.
 */

bool Audio::send_loop(uint32_t channel_number, const int16_t *samples, uint32_t length) {

	if (!this->_validate_channel(channel_number)) {
		return false;
	}

	if (!samples) {
		return false;
	}


	osMutexAcquire(this->_lock, osWaitForever); /* Get the lock */
	ChannelInfo *ch_info = &this->channel_info[channel_number - 1];

	ch_info->audio_sample_size = length;
	ch_info->audio_sample = samples;
	ch_info->state = AS_SEND_AUDIO_LOOP;

	osMutexRelease(this->_lock); /* Release the lock */
	return true;

}

/*
 * Stop call progress tones and audio loops from playing
 */

bool Audio::stop(uint32_t channel_number) {
	if(!this->_validate_channel(channel_number)) {
		return false;
	}
	osMutexAcquire(this->_lock, osWaitForever); /* Get the lock */

	ChannelInfo *ch_info = &this->channel_info[channel_number - 1];
	ch_info->state = AS_IDLE;

	osMutexRelease(this->_lock); /* Release the lock */
	return true;

}

/*
 * This is called by the DMA half full and full interrupts to
 * request a new combined left and right audio block be created
 * at the requested buffer number (0 or 1). 0 indicates the lower
 * half of the buffer needs to be filled, and 1 indicates the upper
 * half of the buffer needs to be filled.
 *
 */

void Audio::request_block(uint8_t buffer_number) {

	HAL_GPIO_WritePin(LEDN_GPIO_Port, LEDN_Pin, GPIO_PIN_RESET);

	/* Calculate the buffer base address into the circular buffer */
	int16_t *buffer = (buffer_number) ? this->lr_audio_output_buffer + LR_AUDIO_BUFFER_SIZE : lr_audio_output_buffer;

	/*
	 * Left and right channels are interleaved.
	 * Left channel (0) is at even buffer addresses
	 * Right channel (1) is at odd buffer addresses
	 *
	 * We iterate through all buffer addresses here
	 * If there's active audio for a channel, it
	 * will be filled byte by byte here If nothing
	 * is active, a zero will be placed into the buffer.
	 */

	osMutexAcquire(this->_lock, osWaitForever); /* Get the lock */

	for (int i = 0; i < LR_AUDIO_BUFFER_SIZE; i++) {
		buffer[i] = 0;
		ChannelInfo *ch_info = &this->channel_info[(i & 1)];

		/*
		 * State machine
		 */


		switch(ch_info->state) {

		case AS_IDLE:
			break;

		case AS_GEN_DIAL_TONE:
			ch_info->is_stoppable = true;
			this->_generate_dual_tone(ch_info,
				INDICATIONS.dial_tone.tone_pair[0], /* F1 */
				INDICATIONS.dial_tone.tone_pair[1], /* F2 */
				INDICATIONS.dial_tone.level_pair[0],/* L1 */
				INDICATIONS.dial_tone.level_pair[1] /* L2 */
			);
			ch_info->state = AS_GEN_DIAL_TONE_WAIT;
			break;

		case AS_GEN_DIAL_TONE_WAIT:
			buffer[i] = this->_next_tone_value(ch_info);
			break;


		case AS_GEN_BUSY_TONE:
		case AS_GEN_CONGESTION_TONE:
			ch_info->is_stoppable = true;
			if (ch_info->state == AS_GEN_CONGESTION_TONE) {
				ch_info->cadence_timing = _convert_ms(INDICATIONS.busy.congestion_cadence_ms);
			}
			else {
				ch_info->cadence_timing =  _convert_ms(INDICATIONS.busy.busy_cadence_ms);
			}
			ch_info->cadence_timer = ch_info->cadence_timing;

			this->_generate_dual_tone(ch_info,
				INDICATIONS.busy.tone_pair[0], /* F1 */
				INDICATIONS.busy.tone_pair[1], /* F2 */
				INDICATIONS.busy.level_pair[0],/* L1 */
				INDICATIONS.busy.level_pair[1] /* L2 */
			);
			ch_info->state = AS_BUSY_WAIT_TONE_END;
			break;

		case AS_BUSY_WAIT_TONE_END:
			buffer[i] = this->_next_tone_value(ch_info);
			if(ch_info->cadence_timer == 0){
				if((buffer[i] > -TONE_SHUTOFF_THRESHOLD) && (buffer[i] < TONE_SHUTOFF_THRESHOLD)) { /* Shut off close to zero to reduce audio clicking. Changes the tone timing ever so slightly */
					ch_info->cadence_timer = ch_info->cadence_timing;
					ch_info->state = AS_BUSY_WAIT_SILENCE_END;
				}
			}
			else {
				ch_info->cadence_timer--;
			}

			break;

		case AS_BUSY_WAIT_SILENCE_END:
			if(ch_info->cadence_timer == 0){
				ch_info->cadence_timer = ch_info->cadence_timing;
				ch_info->state = AS_BUSY_WAIT_TONE_END;
			}
			else {
				ch_info->cadence_timer--;
			}

			break;


		case AS_GEN_RINGING_TONE:
			ch_info->is_stoppable = true;
			ch_info->cadence_timer =  _convert_ms(INDICATIONS.ringing.ring_on_cadence_ms);

			this->_generate_dual_tone(ch_info,
				INDICATIONS.ringing.tone_pair[0], /* F1 */
				INDICATIONS.ringing.tone_pair[1], /* F2 */
				INDICATIONS.ringing.level_pair[0],/* L1 */
				INDICATIONS.ringing.level_pair[1] /* L2 */
			);
			ch_info->state = AS_RINGING_WAIT_TONE_END;
			break;

		case AS_RINGING_WAIT_TONE_END:
			buffer[i] = this->_next_tone_value(ch_info);
			if(ch_info->cadence_timer == 0){
				if((buffer[i] > -TONE_SHUTOFF_THRESHOLD) && (buffer[i] < TONE_SHUTOFF_THRESHOLD)) { /* Shut off close to zero to reduce audio clicking. Changes the tone timing ever so slightly */
					ch_info->cadence_timer = _convert_ms(INDICATIONS.ringing.ring_off_cadence_ms);
					ch_info->state = AS_RINGING_WAIT_SILENCE_END;
				}
			}
			else {
				ch_info->cadence_timer--;
			}

			break;	ch_info->state = AS_IDLE;
			ch_info->state = AS_IDLE;

		case AS_RINGING_WAIT_SILENCE_END:
			if(ch_info->cadence_timer == 0){
				ch_info->cadence_timer = _convert_ms(INDICATIONS.ringing.ring_on_cadence_ms);
				ch_info->state = AS_RINGING_WAIT_TONE_END;
			}
			else {
				ch_info->cadence_timer--;
			}

			break;


		case AS_SEND_MF:
			ch_info->is_stoppable = false;
			if (!ch_info->digit_string_length) { /* Zero length aborts operation */
				ch_info->state = AS_IDLE;
			}
			else {
				/* First tone pair */
				ch_info->digit_string_index = 0;
				ch_info->cadence_timer = this->_get_mf_tone_duration(ch_info->digit_string[ch_info->digit_string_index]);
				this->_generate_dual_tone(ch_info,
								MF.tone_pairs[ch_info->digit_string[ch_info->digit_string_index]].low, /* F1 */
								MF.tone_pairs[ch_info->digit_string[ch_info->digit_string_index]].high, /* F2 */
								MF.levels.low,/* L1 */
								MF.levels.high /* L2 */
							);
				ch_info->digit_string_index++;
				ch_info->state = AS_SEND_MF_WAIT_TONE_END;
			}
			break;

		case AS_SEND_MF_WAIT_TONE_END:
			buffer[i] = this->_next_tone_value(ch_info);
			if (ch_info->cadence_timer == 0){
				if ((buffer[i] > -TONE_SHUTOFF_THRESHOLD) && (buffer[i] < TONE_SHUTOFF_THRESHOLD)) { /* Shut off close to zero to reduce audio clicking. Changes the tone timing ever so slightly */
					/* Test for end of tone sequence */
					if (ch_info->digit_string_index >= ch_info->digit_string_length) {
						/* Call the callback */
						ch_info->callback((i & 1) + 1);
						ch_info->state = AS_IDLE;
					}
					else {
						ch_info->cadence_timer = _convert_ms(MF.inactive_time_ms);
						ch_info->state = AS_SEND_MF_WAIT_SILENCE_END;
					}
				}
			}
			else {
				ch_info->cadence_timer--;
			}
			break;

		case AS_SEND_MF_WAIT_SILENCE_END:
			if (ch_info->cadence_timer == 0){
				/* Next tone pair */
				ch_info->cadence_timer = this->_get_mf_tone_duration(ch_info->digit_string[ch_info->digit_string_index]);
				this->_generate_dual_tone(ch_info,
								MF.tone_pairs[ch_info->digit_string[ch_info->digit_string_index]].low, /* F1 */
								MF.tone_pairs[ch_info->digit_string[ch_info->digit_string_index]].high, /* F2 */
								MF.levels.low,/* L1 */
								MF.levels.high /* L2 */
							);
				ch_info->digit_string_index++;
				ch_info->state = AS_SEND_MF_WAIT_TONE_END;
			}
			else {
				ch_info->cadence_timer--;
			}

		break;

		case AS_SEND_DTMF:
			ch_info->is_stoppable = false;
			if (!ch_info->digit_string_length) { /* Zero length aborts operation */
				ch_info->state = AS_IDLE;
			}
			else {
				/* First tone pair */
				ch_info->digit_string_index = 0;
				ch_info->cadence_timer = this->_convert_ms(DTMF.active_time_ms);
				this->_generate_dual_tone(ch_info,
								DTMF.tone_pairs[ch_info->digit_string[ch_info->digit_string_index]].low, /* F1 */
								DTMF.tone_pairs[ch_info->digit_string[ch_info->digit_string_index]].high, /* F2 */
								DTMF.levels.low,/* L1 */
								DTMF.levels.high /* L2 */
							);
				ch_info->digit_string_index++;
				ch_info->state = AS_SEND_DTMF_WAIT_TONE_END;
			}
			break;

		case AS_SEND_DTMF_WAIT_TONE_END:
			buffer[i] = this->_next_tone_value(ch_info);
			if (ch_info->cadence_timer == 0){
				if ((buffer[i] > -TONE_SHUTOFF_THRESHOLD) && (buffer[i] < TONE_SHUTOFF_THRESHOLD)) { /* Shut off close to zero to reduce audio clicking. Changes the tone timing ever so slightly */
					/* Test for end of tone sequence */
					if (ch_info->digit_string_index >= ch_info->digit_string_length) {
						/* Call the callback */
						ch_info->callback((i & 1) + 1);
						ch_info->state = AS_IDLE;
					}
					else {
						ch_info->cadence_timer = this->_convert_ms(DTMF.inactive_time_ms);
						ch_info->state = AS_SEND_DTMF_WAIT_SILENCE_END;
					}
				}
			}
			else {
				ch_info->cadence_timer--;
			}
			break;

		case AS_SEND_DTMF_WAIT_SILENCE_END:
			if (ch_info->cadence_timer == 0){
				/* Next tone pair */
				ch_info->cadence_timer = this->_convert_ms(DTMF.active_time_ms);
				this->_generate_dual_tone(ch_info,
								DTMF.tone_pairs[ch_info->digit_string[ch_info->digit_string_index]].low, /* F1 */
								DTMF.tone_pairs[ch_info->digit_string[ch_info->digit_string_index]].high, /* F2 */
								DTMF.levels.low,/* L1 */
								DTMF.levels.high /* L2 */
							);
				ch_info->digit_string_index++;
				ch_info->state = AS_SEND_DTMF_WAIT_TONE_END;
			}
			else {
				ch_info->cadence_timer--;
			}
			break;

		case AS_SEND_AUDIO_LOOP:
			ch_info->is_stoppable = true;
			ch_info->audio_sample_index = 0l;
			ch_info->state = AS_SEND_AUDIO_LOOP_WAIT;
			break;


		case AS_SEND_AUDIO_LOOP_WAIT:
			buffer[i] = ch_info->audio_sample[ch_info->audio_sample_index++];
			/* Keep sending the loop until we are stopped */
			if(ch_info->audio_sample_index >= ch_info->audio_sample_size) {
				ch_info->audio_sample_index = 0l;
			}
			break;

		case AS_SEND_AUDIO:
			ch_info->is_stoppable = false;
			ch_info->audio_sample_index = 0l;
			ch_info->state = AS_SEND_AUDIO_WAIT;
			break;


		case AS_SEND_AUDIO_WAIT:
			buffer[i] = ch_info->audio_sample[ch_info->audio_sample_index++];
			if(ch_info->audio_sample_index >= ch_info->audio_sample_size) {
				/* Call the callback */
				ch_info->callback((i & 1) + 1);
				ch_info->state = AS_IDLE;
			}
			break;





		default:
			channel_info->state = AS_IDLE;
			break;

		}

	}
	osMutexRelease(this->_lock); /* Release the lock */

	HAL_GPIO_WritePin(LEDN_GPIO_Port, LEDN_Pin, GPIO_PIN_SET);

}




} /* End namespace audio */
