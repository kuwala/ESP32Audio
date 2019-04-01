/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef effect_delay_h_
#define effect_delay_h_

#include "AudioStream.h"
#include "freertos/FreeRTOS.h"

#define DELAY_QUEUE_SIZE  (20480  / AUDIO_BLOCK_SAMPLES)


class AudioEffectDelay : public AudioStream
{
public:
	AudioEffectDelay() : AudioStream(1, inputQueueArray, "AudioEffectDelay") {
		activemask = 0;
		headindex = 0;
		tailindex = 0;
		maxblocks = 0;
		memset(queue, 0, sizeof(queue));
        initialised = true;
	}
	void delay(uint8_t channel, float milliseconds) {
		if (channel >= 8) return;
		if (milliseconds < 0.0) milliseconds = 0.0;
		uint32_t n = (milliseconds*(AUDIO_SAMPLE_RATE_EXACT/1000.0))+0.5;
		uint32_t nmax = AUDIO_BLOCK_SAMPLES * (DELAY_QUEUE_SIZE-1);
		if (n > nmax) n = nmax;
		uint32_t blks = (n + (AUDIO_BLOCK_SAMPLES-1)) / AUDIO_BLOCK_SAMPLES + 1;
		if (!(activemask & (1<<channel))) {
			// enabling a previously disabled channel
			position[channel] = n;
			if (blks > maxblocks) maxblocks = blks;
			activemask |= (1<<channel);
		} else {
			if (n > position[channel]) {
				// new delay is greater than previous setting
				if (blks > maxblocks) maxblocks = blks;
				position[channel] = n;
			} else {
				// new delay is less than previous setting
				position[channel] = n;
				recompute_maxblocks();
			}
		}
	}
	void disable(uint8_t channel) {
		if (channel >= 8) return;
		// diable this channel
		activemask &= ~(1<<channel);
		// recompute maxblocks for remaining enabled channels
		recompute_maxblocks();
	}
	virtual void update(void);
private:
	void recompute_maxblocks(void) {
		uint32_t max=0;
		uint32_t channel = 0;
		do {
			if (activemask & (1<<channel)) {
				uint32_t n = position[channel];
				n = (n + (AUDIO_BLOCK_SAMPLES-1)) / AUDIO_BLOCK_SAMPLES + 1;
				if (n > max) max = n;
			}
		} while(++channel < 8);
		maxblocks = max;
	}
	uint8_t activemask;   // which output channels are active
	uint16_t headindex;    // head index (incoming) data in quueu
	uint16_t tailindex;    // tail index (outgoing) data from queue
	uint16_t maxblocks;    // number of blocks needed in queue
#if DELAY_QUEUE_SIZE * AUDIO_BLOCK_SAMPLES < 65535
	uint16_t position[8]; // # of sample delay for each channel
#else
	uint32_t position[8]; // # of sample delay for each channel
#endif
	audio_block_t *queue[DELAY_QUEUE_SIZE];
	audio_block_t *inputQueueArray[1];
};

#endif