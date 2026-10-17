#pragma once

// Includes
#include "raylib.h"

// Blocky 3x5 pixel font that mirrors the segment-drawn digits of the original
// 1972 arcade machine. Glyphs are rendered as filled squares so the text scales
// cleanly with the rest of the game. Supports A-Z, 0-9, space and - : . ! /.
// Unsupported characters render as blank space.

// Draws text with its top-left corner at (x, y). Each glyph pixel is drawn as a
// pixelSize x pixelSize square; one pixel column of spacing separates glyphs.
void DrawPixelText(const char *text, int x, int y, int pixelSize, Color color);

// Returns the rendered width of text in game units for the given pixel size.
int MeasurePixelText(const char *text, int pixelSize);
