// Includes
#include "audio.h"
#include "raylib.h"
#include <stdlib.h>

// Constants
#define SAMPLE_RATE 44100
#define AMPLITUDE 6000
#define FADE_FRAMES 200

// Tones used by the original 1972 arcade machine
#define PADDLE_FREQ 459.0f
#define WALL_FREQ   226.0f
#define SCORE_FREQ  490.0f

#define HIT_DURATION   0.06f
#define SCORE_DURATION 0.25f

static Sound paddleSound;
static Sound wallSound;
static Sound scoreSound;

// Synthesizes a mono 16-bit square wave. The final FADE_FRAMES samples are
// ramped down linearly to avoid an audible click when the tone cuts off.
// raylib copies the sample data into its own buffer, so the wave is freed here.
static Sound MakeTone(float frequency, float duration) {
    int frameCount = (int)(SAMPLE_RATE * duration);
    short *samples = malloc(frameCount * sizeof(short));
    float halfPeriod = SAMPLE_RATE / frequency / 2.0f;

    for (int i = 0; i < frameCount; i++) {
        short value = ((int)(i / halfPeriod) % 2 == 0) ? AMPLITUDE : -AMPLITUDE;
        int remaining = frameCount - i;
        if (remaining < FADE_FRAMES) value = (short)(value * remaining / FADE_FRAMES);
        samples[i] = value;
    }

    Wave wave = {
        .frameCount = frameCount,
        .sampleRate = SAMPLE_RATE,
        .sampleSize = 16,
        .channels = 1,
        .data = samples,
    };
    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return sound;
}

void AudioInit(void) {
    paddleSound = MakeTone(PADDLE_FREQ, HIT_DURATION);
    wallSound = MakeTone(WALL_FREQ, HIT_DURATION);
    scoreSound = MakeTone(SCORE_FREQ, SCORE_DURATION);
}

void AudioClose(void) {
    UnloadSound(paddleSound);
    UnloadSound(wallSound);
    UnloadSound(scoreSound);
}

void AudioPlayPaddle(void) { PlaySound(paddleSound); }
void AudioPlayWall(void) { PlaySound(wallSound); }
void AudioPlayScore(void) { PlaySound(scoreSound); }
