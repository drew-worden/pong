#pragma once

// Procedurally generated square-wave beeps tuned to the three tones of the
// original Pong cabinet (paddle hit, wall hit, point scored). No audio assets
// are required; the samples are synthesized once at startup.

// Generates the sound effects. InitAudioDevice() must already have been called.
void AudioInit(void);

// Releases the sound effects. Call before CloseAudioDevice().
void AudioClose(void);

// Plays the tone for the ball striking a paddle.
void AudioPlayPaddle(void);

// Plays the tone for the ball bouncing off the top or bottom wall.
void AudioPlayWall(void);

// Plays the longer tone for a point being scored.
void AudioPlayScore(void);
