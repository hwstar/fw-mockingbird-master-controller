#pragma once
#include "logging.h"

namespace Mfd {



const uint8_t NUM_MF_FREQUENCIES = 6;
const uint8_t MF_MAX_DIGITS = 16;
const uint8_t MF_DECODE_TABLE_SIZE = 15;
const float MF_SAMPLE_RATE = 16000.0; // 16000 Hz simplifies the anti-aliasing low pass filter requirements.
const uint16_t MF_FRAME_SIZE = 320; // 20mS
const uint16_t MF_ADC_BUF_LEN = (2*MF_FRAME_SIZE);
const float MIN_ADC = -2048.0;
const float SILENCE_THRESHOLD = 2.0; // Digit detect noise floor 
const uint8_t MIN_KP_GATE_BLOCK_COUNT = 3;
const uint8_t MIN_DIGIT_BLOCK_COUNT = 2;
const uint16_t MF_INTERDIGIT_TIMEOUT = 50*5; // 5 Seconds


enum {MFE_OK=0, MFE_TIMEOUT};
enum {MFR_IDLE=0, MFR_WAIT_KP, MFR_KP_SILENCE, MFR_WAIT_DIGIT, MFR_WAIT_DIGIT_SILENCE, MFR_TIMEOUT, MFR_DONE, MFR_WAIT_RELEASE};

// Data for each goertzel tone decoder

typedef struct goertzelData {
	float q1;
	float q2;
	float coeff_k;
	float power;
} goertzelData;



typedef struct mfData {
	char tone_digit;
	uint8_t timer;
	uint8_t state;
	uint8_t error_code;
	uint8_t tone_block_count;
	uint8_t digit_count;
	uint32_t descriptor;
	void (*callback)(uint8_t error_code, uint8_t digit_count, char *data);
	char digits[MF_MAX_DIGITS];


} mfData;

// Functions

class MF_decoder {

public:


void setup(); /* Called once during initialization to set up the decoder */
uint32_t seize(void (*callback)(uint8_t error_code, uint8_t digit_count, char *data)); /* Called to seize the MF receiver */
bool release(uint32_t descriptor); /* Called to release the MF receiver */

void handle_buffer(uint8_t buffer_no); // Called by the DMA engine when half full and full.'


protected:
float _goertzel_block[MF_FRAME_SIZE];
goertzelData _goertzel_data[NUM_MF_FREQUENCIES];
osMutexId_t _lock;
mfData _mf_data;
uint16_t _mf_adc_buffer[MF_ADC_BUF_LEN];

};

} // END Namespace Mfd
