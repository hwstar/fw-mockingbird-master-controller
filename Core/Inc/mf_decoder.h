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
const uint16_t MF_MAX_DEBUG_INFO = 256;
const uint8_t MFE_OK = 0;
const uint8_t MFE_TIMEOUT = 1;
const uint8_t MFE_MORE_THAN_TWO_TONES = 2;


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
	uint8_t digits[MF_MAX_DIGITS];
	uint8_t debug_info[MF_MAX_DEBUG_INFO];
} mfData;

// Functions

class MF_decoder {

public:


void setup(); // Called once during initialization to set up the decoder
void listen_start(); // Called to enable the MF receiver
void listen_stop(); // Called to disable the MF receiver
bool check_done(); // Called to see if const float PI = 3.141529;the MF receiver is done
int get_error_code(); // Called to retrieve error code
char *get_received_digits(); // Return digits received

void handle_buffer(uint8_t buffer_no); // Called by the DMA engine when half full and full.'


protected:
float _goertzel_block[MF_FRAME_SIZE];
goertzelData _goertzel_data[NUM_MF_FREQUENCIES];
osMutexId_t _lock;
mfData _mf_data;
uint16_t _mf_adc_buffer[MF_ADC_BUF_LEN];

};

} // END Namespace Mfd
