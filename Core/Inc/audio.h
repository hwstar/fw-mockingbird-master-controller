#pragma once
#include "top.h"


namespace Audio {

const float SAMPLE_FREQ_HZ = 8000;
const uint16_t SINE_TABLE_BIT_WIDTH = 10;
const uint16_t PHASE_ACCUMULATOR_WIDTH = 16;
const uint16_t LR_AUDIO_BUFFER_SIZE = 320;

const uint16_t AUDIO_BUFFER_SIZE = LR_AUDIO_BUFFER_SIZE/2;
const uint16_t LEFT_BUFFER = 0;
const uint16_t RIGHT_BUFFER = 1;
const uint16_t SINE_TABLE_LENGTH = (1 << SINE_TABLE_BIT_WIDTH);
const uint16_t PHASE_ACCUMULATOR_TRUNCATION = (PHASE_ACCUMULATOR_WIDTH - SINE_TABLE_BIT_WIDTH);
const uint32_t PHASE_ACCUM_MODULO_N = (1 << PHASE_ACCUMULATOR_WIDTH);
const uint32_t PHASE_ACCUMULATOR_MASK = (PHASE_ACCUM_MODULO_N - 1);



class Audio {
public:
	void setup(void);
	void process_left_right(void);
	void dma_start(void);
	void dma_stop(void);
	void request_block(uint8_t buffer_number);
	void generate_tone(float freq, float level);
	void generate_dual_tone(float freq1, float freq2, float db_level1, float db_level2);
	int16_t next_value(void);

protected:
	osMutexId_t _lock;
	float f1;
	float f2;
	float f1_level;
	float f2_level;
	uint32_t phase_accum[2];
	uint16_t tuning_word[2];
	int16_t lr_audio_output_buffer[LR_AUDIO_BUFFER_SIZE *2]; /* 2 buffers in circular buffer for double buffering */
	int16_t audio_output_buffers[2][AUDIO_BUFFER_SIZE]; /* Individual left and right channel buffers */
};


} /* End namespace audio */
