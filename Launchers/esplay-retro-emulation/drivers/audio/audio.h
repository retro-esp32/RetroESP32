#ifndef AUDIO_H
#define AUDIO_H

#include "settings.h"

#define I2S_NUM I2S_NUM_0
#define VOLUME_LEVEL_COUNT (5)


void audio_init(int sample_rate);
void audio_submit(short *stereoAudioBuffer, int frameCount);
void audio_terminate();
esplay_volume_level audio_volume_get();
void audio_volume_set(esplay_volume_level value);
void audio_volume_change();

#endif
