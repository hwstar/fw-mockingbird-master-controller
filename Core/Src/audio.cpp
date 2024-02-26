#include "top.h"
#include "audio.h"
#include "logging.h"
#include <math.h>

namespace Audio {

static const char *TAG = "audio";


/** Generated using Dr LUT - Free Lookup Table Generator
  * https://github.com/ppelikan/drlut
  **/
// Formula: sin(2*pi*t/T)
static const uint16_t lut[SINE_TABLE_LENGTH] = {
0x0000,0x00C9,0x0192,0x025B,0x0324,0x03ED,0x04B6,
0x057F,0x0648,0x0711,0x07D9,0x08A2,0x096A,0x0A33,
0x0AFB,0x0BC4,0x0C8C,0x0D54,0x0E1C,0x0EE3,0x0FAB,
0x1072,0x113A,0x1201,0x12C8,0x138F,0x1455,0x151C,
0x15E2,0x16A8,0x176E,0x1833,0x18F9,0x19BE,0x1A82,
0x1B47,0x1C0B,0x1CCF,0x1D93,0x1E57,0x1F1A,0x1FDD,
0x209F,0x2161,0x2223,0x22E5,0x23A6,0x2467,0x2528,
0x25E8,0x26A8,0x2767,0x2826,0x28E5,0x29A3,0x2A61,
0x2B1F,0x2BDC,0x2C99,0x2D55,0x2E11,0x2ECC,0x2F87,
0x3041,0x30FB,0x31B5,0x326E,0x3326,0x33DF,0x3496,
0x354D,0x3604,0x36BA,0x376F,0x3824,0x38D9,0x398C,
0x3A40,0x3AF2,0x3BA5,0x3C56,0x3D07,0x3DB8,0x3E68,
0x3F17,0x3FC5,0x4073,0x4121,0x41CE,0x427A,0x4325,
0x43D0,0x447A,0x4524,0x45CD,0x4675,0x471C,0x47C3,
0x4869,0x490F,0x49B4,0x4A58,0x4AFB,0x4B9D,0x4C3F,
0x4CE0,0x4D81,0x4E20,0x4EBF,0x4F5D,0x4FFB,0x5097,
0x5133,0x51CE,0x5268,0x5302,0x539B,0x5432,0x54C9,
0x5560,0x55F5,0x568A,0x571D,0x57B0,0x5842,0x58D3,
0x5964,0x59F3,0x5A82,0x5B0F,0x5B9C,0x5C28,0x5CB3,
0x5D3E,0x5DC7,0x5E4F,0x5ED7,0x5F5D,0x5FE3,0x6068,
0x60EB,0x616E,0x61F0,0x6271,0x62F1,0x6370,0x63EE,
0x646C,0x64E8,0x6563,0x65DD,0x6656,0x66CF,0x6746,
0x67BC,0x6832,0x68A6,0x6919,0x698B,0x69FD,0x6A6D,
0x6ADC,0x6B4A,0x6BB7,0x6C23,0x6C8E,0x6CF8,0x6D61,
0x6DC9,0x6E30,0x6E96,0x6EFB,0x6F5E,0x6FC1,0x7022,
0x7083,0x70E2,0x7140,0x719D,0x71F9,0x7254,0x72AE,
0x7307,0x735E,0x73B5,0x740A,0x745F,0x74B2,0x7504,
0x7555,0x75A5,0x75F3,0x7641,0x768D,0x76D8,0x7722,
0x776B,0x77B3,0x77FA,0x783F,0x7884,0x78C7,0x7909,
0x794A,0x7989,0x79C8,0x7A05,0x7A41,0x7A7C,0x7AB6,
0x7AEE,0x7B26,0x7B5C,0x7B91,0x7BC5,0x7BF8,0x7C29,
0x7C59,0x7C88,0x7CB6,0x7CE3,0x7D0E,0x7D39,0x7D62,
0x7D89,0x7DB0,0x7DD5,0x7DFA,0x7E1D,0x7E3E,0x7E5F,
0x7E7E,0x7E9C,0x7EB9,0x7ED5,0x7EEF,0x7F09,0x7F21,
0x7F37,0x7F4D,0x7F61,0x7F74,0x7F86,0x7F97,0x7FA6,
0x7FB4,0x7FC1,0x7FCD,0x7FD8,0x7FE1,0x7FE9,0x7FF0,
0x7FF5,0x7FF9,0x7FFD,0x7FFE,0x7FFF,0x7FFE,0x7FFD,
0x7FF9,0x7FF5,0x7FF0,0x7FE9,0x7FE1,0x7FD8,0x7FCD,
0x7FC1,0x7FB4,0x7FA6,0x7F97,0x7F86,0x7F74,0x7F61,
0x7F4D,0x7F37,0x7F21,0x7F09,0x7EEF,0x7ED5,0x7EB9,
0x7E9C,0x7E7E,0x7E5F,0x7E3E,0x7E1D,0x7DFA,0x7DD5,
0x7DB0,0x7D89,0x7D62,0x7D39,0x7D0E,0x7CE3,0x7CB6,
0x7C88,0x7C59,0x7C29,0x7BF8,0x7BC5,0x7B91,0x7B5C,
0x7B26,0x7AEE,0x7AB6,0x7A7C,0x7A41,0x7A05,0x79C8,
0x7989,0x794A,0x7909,0x78C7,0x7884,0x783F,0x77FA,
0x77B3,0x776B,0x7722,0x76D8,0x768D,0x7641,0x75F3,
0x75A5,0x7555,0x7504,0x74B2,0x745F,0x740A,0x73B5,
0x735E,0x7307,0x72AE,0x7254,0x71F9,0x719D,0x7140,
0x70E2,0x7083,0x7022,0x6FC1,0x6F5E,0x6EFB,0x6E96,
0x6E30,0x6DC9,0x6D61,0x6CF8,0x6C8E,0x6C23,0x6BB7,
0x6B4A,0x6ADC,0x6A6D,0x69FD,0x698B,0x6919,0x68A6,
0x6832,0x67BC,0x6746,0x66CF,0x6656,0x65DD,0x6563,
0x64E8,0x646C,0x63EE,0x6370,0x62F1,0x6271,0x61F0,
0x616E,0x60EB,0x6068,0x5FE3,0x5F5D,0x5ED7,0x5E4F,
0x5DC7,0x5D3E,0x5CB3,0x5C28,0x5B9C,0x5B0F,0x5A82,
0x59F3,0x5964,0x58D3,0x5842,0x57B0,0x571D,0x568A,
0x55F5,0x5560,0x54C9,0x5432,0x539B,0x5302,0x5268,
0x51CE,0x5133,0x5097,0x4FFB,0x4F5D,0x4EBF,0x4E20,
0x4D81,0x4CE0,0x4C3F,0x4B9D,0x4AFB,0x4A58,0x49B4,
0x490F,0x4869,0x47C3,0x471C,0x4675,0x45CD,0x4524,
0x447A,0x43D0,0x4325,0x427A,0x41CE,0x4121,0x4073,
0x3FC5,0x3F17,0x3E68,0x3DB8,0x3D07,0x3C56,0x3BA5,
0x3AF2,0x3A40,0x398C,0x38D9,0x3824,0x376F,0x36BA,
0x3604,0x354D,0x3496,0x33DF,0x3326,0x326E,0x31B5,
0x30FB,0x3041,0x2F87,0x2ECC,0x2E11,0x2D55,0x2C99,
0x2BDC,0x2B1F,0x2A61,0x29A3,0x28E5,0x2826,0x2767,
0x26A8,0x25E8,0x2528,0x2467,0x23A6,0x22E5,0x2223,
0x2161,0x209F,0x1FDD,0x1F1A,0x1E57,0x1D93,0x1CCF,
0x1C0B,0x1B47,0x1A82,0x19BE,0x18F9,0x1833,0x176E,
0x16A8,0x15E2,0x151C,0x1455,0x138F,0x12C8,0x1201,
0x113A,0x1072,0x0FAB,0x0EE3,0x0E1C,0x0D54,0x0C8C,
0x0BC4,0x0AFB,0x0A33,0x096A,0x08A2,0x07D9,0x0711,
0x0648,0x057F,0x04B6,0x03ED,0x0324,0x025B,0x0192,
0x00C9,0x0000,0xFF37,0xFE6E,0xFDA5,0xFCDC,0xFC13,
0xFB4A,0xFA81,0xF9B8,0xF8EF,0xF827,0xF75E,0xF696,
0xF5CD,0xF505,0xF43C,0xF374,0xF2AC,0xF1E4,0xF11D,
0xF055,0xEF8E,0xEEC6,0xEDFF,0xED38,0xEC71,0xEBAB,
0xEAE4,0xEA1E,0xE958,0xE892,0xE7CD,0xE707,0xE642,
0xE57E,0xE4B9,0xE3F5,0xE331,0xE26D,0xE1A9,0xE0E6,
0xE023,0xDF61,0xDE9F,0xDDDD,0xDD1B,0xDC5A,0xDB99,
0xDAD8,0xDA18,0xD958,0xD899,0xD7DA,0xD71B,0xD65D,
0xD59F,0xD4E1,0xD424,0xD367,0xD2AB,0xD1EF,0xD134,
0xD079,0xCFBF,0xCF05,0xCE4B,0xCD92,0xCCDA,0xCC21,
0xCB6A,0xCAB3,0xC9FC,0xC946,0xC891,0xC7DC,0xC727,
0xC674,0xC5C0,0xC50E,0xC45B,0xC3AA,0xC2F9,0xC248,
0xC198,0xC0E9,0xC03B,0xBF8D,0xBEDF,0xBE32,0xBD86,
0xBCDB,0xBC30,0xBB86,0xBADC,0xBA33,0xB98B,0xB8E4,
0xB83D,0xB797,0xB6F1,0xB64C,0xB5A8,0xB505,0xB463,
0xB3C1,0xB320,0xB27F,0xB1E0,0xB141,0xB0A3,0xB005,
0xAF69,0xAECD,0xAE32,0xAD98,0xACFE,0xAC65,0xABCE,
0xAB37,0xAAA0,0xAA0B,0xA976,0xA8E3,0xA850,0xA7BE,
0xA72D,0xA69C,0xA60D,0xA57E,0xA4F1,0xA464,0xA3D8,
0xA34D,0xA2C2,0xA239,0xA1B1,0xA129,0xA0A3,0xA01D,
0x9F98,0x9F15,0x9E92,0x9E10,0x9D8F,0x9D0F,0x9C90,
0x9C12,0x9B94,0x9B18,0x9A9D,0x9A23,0x99AA,0x9931,
0x98BA,0x9844,0x97CE,0x975A,0x96E7,0x9675,0x9603,
0x9593,0x9524,0x94B6,0x9449,0x93DD,0x9372,0x9308,
0x929F,0x9237,0x91D0,0x916A,0x9105,0x90A2,0x903F,
0x8FDE,0x8F7D,0x8F1E,0x8EC0,0x8E63,0x8E07,0x8DAC,
0x8D52,0x8CF9,0x8CA2,0x8C4B,0x8BF6,0x8BA1,0x8B4E,
0x8AFC,0x8AAB,0x8A5B,0x8A0D,0x89BF,0x8973,0x8928,
0x88DE,0x8895,0x884D,0x8806,0x87C1,0x877C,0x8739,
0x86F7,0x86B6,0x8677,0x8638,0x85FB,0x85BF,0x8584,
0x854A,0x8512,0x84DA,0x84A4,0x846F,0x843B,0x8408,
0x83D7,0x83A7,0x8378,0x834A,0x831D,0x82F2,0x82C7,
0x829E,0x8277,0x8250,0x822B,0x8206,0x81E3,0x81C2,
0x81A1,0x8182,0x8164,0x8147,0x812B,0x8111,0x80F7,
0x80DF,0x80C9,0x80B3,0x809F,0x808C,0x807A,0x8069,
0x805A,0x804C,0x803F,0x8033,0x8028,0x801F,0x8017,
0x8010,0x800B,0x8007,0x8003,0x8002,0x8001,0x8002,
0x8003,0x8007,0x800B,0x8010,0x8017,0x801F,0x8028,
0x8033,0x803F,0x804C,0x805A,0x8069,0x807A,0x808C,
0x809F,0x80B3,0x80C9,0x80DF,0x80F7,0x8111,0x812B,
0x8147,0x8164,0x8182,0x81A1,0x81C2,0x81E3,0x8206,
0x822B,0x8250,0x8277,0x829E,0x82C7,0x82F2,0x831D,
0x834A,0x8378,0x83A7,0x83D7,0x8408,0x843B,0x846F,
0x84A4,0x84DA,0x8512,0x854A,0x8584,0x85BF,0x85FB,
0x8638,0x8677,0x86B6,0x86F7,0x8739,0x877C,0x87C1,
0x8806,0x884D,0x8895,0x88DE,0x8928,0x8973,0x89BF,
0x8A0D,0x8A5B,0x8AAB,0x8AFC,0x8B4E,0x8BA1,0x8BF6,
0x8C4B,0x8CA2,0x8CF9,0x8D52,0x8DAC,0x8E07,0x8E63,
0x8EC0,0x8F1E,0x8F7D,0x8FDE,0x903F,0x90A2,0x9105,
0x916A,0x91D0,0x9237,0x929F,0x9308,0x9372,0x93DD,
0x9449,0x94B6,0x9524,0x9593,0x9603,0x9675,0x96E7,
0x975A,0x97CE,0x9844,0x98BA,0x9931,0x99AA,0x9A23,
0x9A9D,0x9B18,0x9B94,0x9C12,0x9C90,0x9D0F,0x9D8F,
0x9E10,0x9E92,0x9F15,0x9F98,0xA01D,0xA0A3,0xA129,
0xA1B1,0xA239,0xA2C2,0xA34D,0xA3D8,0xA464,0xA4F1,
0xA57E,0xA60D,0xA69C,0xA72D,0xA7BE,0xA850,0xA8E3,
0xA976,0xAA0B,0xAAA0,0xAB37,0xABCE,0xAC65,0xACFE,
0xAD98,0xAE32,0xAECD,0xAF69,0xB005,0xB0A3,0xB141,
0xB1E0,0xB27F,0xB320,0xB3C1,0xB463,0xB505,0xB5A8,
0xB64C,0xB6F1,0xB797,0xB83D,0xB8E4,0xB98B,0xBA33,
0xBADC,0xBB86,0xBC30,0xBCDB,0xBD86,0xBE32,0xBEDF,
0xBF8D,0xC03B,0xC0E9,0xC198,0xC248,0xC2F9,0xC3AA,
0xC45B,0xC50E,0xC5C0,0xC674,0xC727,0xC7DC,0xC891,
0xC946,0xC9FC,0xCAB3,0xCB6A,0xCC21,0xCCDA,0xCD92,
0xCE4B,0xCF05,0xCFBF,0xD079,0xD134,0xD1EF,0xD2AB,
0xD367,0xD424,0xD4E1,0xD59F,0xD65D,0xD71B,0xD7DA,
0xD899,0xD958,0xDA18,0xDAD8,0xDB99,0xDC5A,0xDD1B,
0xDDDD,0xDE9F,0xDF61,0xE023,0xE0E6,0xE1A9,0xE26D,
0xE331,0xE3F5,0xE4B9,0xE57E,0xE642,0xE707,0xE7CD,
0xE892,0xE958,0xEA1E,0xEAE4,0xEBAB,0xEC71,0xED38,
0xEDFF,0xEEC6,0xEF8E,0xF055,0xF11D,0xF1E4,0xF2AC,
0xF374,0xF43C,0xF505,0xF5CD,0xF696,0xF75E,0xF827,
0xF8EF,0xF9B8,0xFA81,0xFB4A,0xFC13,0xFCDC,0xFDA5,
0xFE6E,0xFF37 };




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

	/* Initialize channel information */
	for (int i = 0; i < NUM_AUDIO_CHANNELS; i++) {
		if(i == 0) { // DEBUG
			_convert_digit_string(&this->channel_info[0], "*0123456789#", true);
			this->channel_info[i].state = AS_SEND_MF;
		}
		else {
			this->channel_info[i].state = AS_GEN_DIAL_TONE;
		}

		this->channel_info[i].return_state = AS_IDLE;


	}
	/* Start I2S DMA */

	this->_dma_start();


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

			break;

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

		default:
			channel_info->state = AS_IDLE;
			break;

		}
	}

	HAL_GPIO_WritePin(LEDN_GPIO_Port, LEDN_Pin, GPIO_PIN_SET);

}




} /* End namespace audio */
