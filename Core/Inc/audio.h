#pragma once
#include "top.h"


namespace Audio {

enum {AS_IDLE=0,
	AS_GEN_DIAL_TONE, AS_GEN_DIAL_TONE_WAIT,
	AS_GEN_BUSY_TONE, AS_GEN_CONGESTION_TONE, AS_BUSY_WAIT_TONE_END, AS_BUSY_WAIT_SILENCE_END,
	AS_GEN_RINGING_TONE, AS_RINGING_WAIT_TONE_END, AS_RINGING_WAIT_SILENCE_END,
	AS_SEND_MF, AS_SEND_MF_WAIT_TONE_END, AS_SEND_MF_WAIT_SILENCE_END,
	AS_SEND_DTMF, AS_SEND_DTMF_WAIT_TONE_END, AS_SEND_DTMF_WAIT_SILENCE_END
};

const float SAMPLE_FREQ_HZ = 8000; /* Actual sample freq is slightly higher at 8012 Hz, but setting this to 8000 Hz reduces jitter in the tone frequencies. */
const uint16_t SINE_TABLE_BIT_WIDTH = 10; /* 1KB of sine table */
const uint16_t PHASE_ACCUMULATOR_WIDTH = 16; /* 16 bits gives appx. 0.14 Hz of frequency resolution */
const uint16_t LR_AUDIO_BUFFER_SIZE = 320; /* Left and right audio buffer audio samples for a 20mS frame of both */
const int16_t TONE_SHUTOFF_THRESHOLD = 200; /* Adjustment to trade off clicking at the end of a tone, vs. the length of the tone */


const uint8_t DIGIT_STRING_MAX_LENGTH = 20;
const uint16_t NUM_MF_TONE_PAIRS = 15;
const uint16_t NUM_DTMF_TONE_PAIRS = 16;
const uint8_t MAX_TONES = 2;
const uint8_t NUM_AUDIO_CHANNELS = 2;
const uint16_t AUDIO_BUFFER_SIZE = (LR_AUDIO_BUFFER_SIZE/NUM_AUDIO_CHANNELS);
const uint16_t SINE_TABLE_LENGTH = (1 << SINE_TABLE_BIT_WIDTH);
const uint16_t PHASE_ACCUMULATOR_TRUNCATION = (PHASE_ACCUMULATOR_WIDTH - SINE_TABLE_BIT_WIDTH);
const uint32_t PHASE_ACCUM_MODULO_N = (1 << PHASE_ACCUMULATOR_WIDTH);
const uint32_t PHASE_ACCUMULATOR_MASK = (PHASE_ACCUM_MODULO_N - 1);
const uint32_t TIME_PER_SAMPLE_US = 1000000UL/SAMPLE_FREQ_HZ;



typedef struct ChannelInfo {
	uint8_t state;
	uint8_t return_state;
	uint8_t digit_string_length;
	uint8_t digit_string_index;
	uint8_t digit_string[DIGIT_STRING_MAX_LENGTH];
	float f1;
	float f2;
	float f1_level;
	float f2_level;
	uint32_t cadence_timing;
	uint32_t cadence_timer;
	uint32_t phase_accum[MAX_TONES];
	uint16_t tuning_word[MAX_TONES];
} ChannelInfo;

typedef struct Indications {
	struct Dial_Tone {
		float tone_pair[2];
		float level_pair[2];
	} dial_tone;
	struct Busy {
		float tone_pair[2];
		float level_pair[2];
		uint16_t busy_cadence_ms;
		uint16_t congestion_cadence_ms;
	} busy;
	struct Ringing {
		float tone_pair[2];
		float level_pair[2];
		uint16_t ring_on_cadence_ms;
		uint16_t ring_off_cadence_ms;
	}ringing;
}Indications;

typedef struct Dtmf {
	struct Levels {
		float high;
		float low;
	}levels;
	struct Tone_Pairs {
		float high;
		float low;
	}tone_pairs[NUM_DTMF_TONE_PAIRS];
	int16_t active_time_ms;
	int16_t inactive_time_ms;
}Dtmf;


typedef struct Mf {
	struct Levels {
		float high;
		float low;
	}levels;
	struct Tone_Pairs {
		float high;
		float low;
	}tone_pairs[NUM_MF_TONE_PAIRS];
	int16_t kp_active_time_ms;
	int16_t active_time_ms;
	int16_t inactive_time_ms;
	int16_t st_active_time_ms;
}Mf;

const Indications INDICATIONS = { /* Follows the precise tone plan: https://en.wikipedia.org/wiki/Precise_tone_plan */
									{
											{350.0, 440.0}, /* Dial Tone Frequencies */
											{-12, -12}, /* Levels in dB */
									},
									{
											{480.0, 620.0}, /* Busy/Congestion Frequencies */
											{-12, -12}, /* Levels in dB*/
											500, /* Busy cadence */
											250 /* Circuit busy cadence */
									},
									{
											{440.0, 480.0}, /* Ringing Frequencies */
											{-12, -12}, /* Levels in dB*/
											2000, /* On cadence cadence */
											4000 /* Off cadence */
									}
};


const Dtmf DTMF = {
					{-8.0,-6.0}, /* Levels */
					{
							{941.0,1336.0}, /* 0 */
							{697.0,1209.0}, /* 1 */
							{697.0,1336.0}, /* 2 */
							{697.0,1477.0}, /* 3 */
							{770.0, 1209.0}, /* 4 */
							{770.0, 1336.0}, /* 5 */
							{770.0, 1477.0}, /* 6 */
							{852.0, 1209.0}, /* 7 */
							{852.0, 1336.0}, /* 8 */
							{852.0, 1477.0}, /* 9 */
							{941.0, 1209.0}, /* * */
							{941.0, 1477.0}, /* # */
							{697.0, 1633.0}, /* A */
							{770.0, 1633.0}, /* B */
							{852.0, 1633.0}, /* C */
							{941.0, 1633.0}, /* D */
					},
					50, /* Active time */
					50, /* Inactive time */
};

const Mf MF = {
					{-7.0, -7.0}, /* Levels */
					{
							{1300.0, 1500.0}, /* 0 */
							{700.0, 900.0}, /* 1 */
							{700.0, 1100.0}, /* 2 */
							{900.0, 1100.0}, /* 3 */
							{700.0, 1300.0}, /* 4 */
							{900.0, 1300.0}, /* 5 */
							{1100.0, 1300.0}, /* 6 */
							{700.0, 1500.0}, /* 7 */
							{900.0, 1500.0}, /* 8 */
							{1100.0, 1500.0}, /* 9 */
							{1100.0, 1700.0}, /* KP */
							{1500.0, 1700.0}, /* ST */
							{900.0, 1700.0}, /* STP */
							{1300.0, 1700.0}, /* ST2P */
							{700.0, 1700.0} /* ST3P */
					},
					110, /* KP active time */
					70, /* Digits 0-9 active time */
					70, /* Digit inactive time */
					70 /* ST digits inactive time */
};

class Audio {
public:
	void setup(void);
	void request_block(uint8_t buffer_number);


protected:
	int32_t inline _convert_ms(uint16_t ms) { return ((((uint32_t) ms) * 1000)/TIME_PER_SAMPLE_US); }
	bool _convert_digit_string(ChannelInfo *ch_info, const char *digits, bool is_mf = false);
	uint32_t _get_mf_tone_duration(uint8_t mf_digit);
	void _process_left_right(void);
	void _dma_start(void);
	void _dma_stop(void);
	int16_t _next_tone_value(ChannelInfo *channel_info);
	void _generate_tone(ChannelInfo *channel_info, float freq, float level);
	void _generate_dual_tone(ChannelInfo *channel_info, float freq1, float freq2, float db_level1, float db_level2);
	ChannelInfo channel_info[NUM_AUDIO_CHANNELS];
	osMutexId_t _lock;
	int16_t lr_audio_output_buffer[LR_AUDIO_BUFFER_SIZE * 2]; /* 2 buffers in circular buffer for double buffering */
};


} /* End namespace audio */
