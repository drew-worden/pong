// Includes
#include "font.h"

// Constants
#define GLYPH_W 3
#define GLYPH_H 5
#define GLYPH_SPACING 1

// Each glyph is a 15-character bitmap, row-major from the top-left, where '1'
// marks a filled pixel. Kept as strings so the shapes are readable in source.
// Unknown characters return a blank glyph rather than failing.
static const char *GlyphBitmap(char c) {
    switch (c) {
        case '0': return "111101101101111";
        case '1': return "010110010010111";
        case '2': return "111001111100111";
        case '3': return "111001111001111";
        case '4': return "101101111001001";
        case '5': return "111100111001111";
        case '6': return "111100111101111";
        case '7': return "111001001001001";
        case '8': return "111101111101111";
        case '9': return "111101111001111";
        case 'A': return "111101111101101";
        case 'B': return "110101110101110";
        case 'C': return "111100100100111";
        case 'D': return "110101101101110";
        case 'E': return "111100111100111";
        case 'F': return "111100111100100";
        case 'G': return "111100101101111";
        case 'H': return "101101111101101";
        case 'I': return "111010010010111";
        case 'J': return "001001001101111";
        case 'K': return "101101110101101";
        case 'L': return "100100100100111";
        case 'M': return "101111101101101";
        case 'N': return "110101101101101";
        case 'O': return "111101101101111";
        case 'P': return "111101111100100";
        case 'Q': return "111101101111001";
        case 'R': return "111101110101101";
        case 'S': return "111100111001111";
        case 'T': return "111010010010010";
        case 'U': return "101101101101111";
        case 'V': return "101101101101010";
        case 'W': return "101101101111101";
        case 'X': return "101101010101101";
        case 'Y': return "101101010010010";
        case 'Z': return "111001010100111";
        case '-': return "000000111000000";
        case ':': return "000010000010000";
        case '.': return "000000000000010";
        case '!': return "010010010000010";
        case '/': return "001001010100100";
        default:  return "000000000000000";
    }
}

void DrawPixelText(const char *text, int x, int y, int pixelSize, Color color) {
    for (const char *c = text; *c; c++) {
        const char *bitmap = GlyphBitmap(*c);
        for (int row = 0; row < GLYPH_H; row++) {
            for (int col = 0; col < GLYPH_W; col++) {
                if (bitmap[row * GLYPH_W + col] == '1') {
                    DrawRectangle(x + col * pixelSize, y + row * pixelSize, pixelSize, pixelSize, color);
                }
            }
        }
        x += (GLYPH_W + GLYPH_SPACING) * pixelSize;
    }
}

int MeasurePixelText(const char *text, int pixelSize) {
    int length = 0;
    while (text[length]) length++;
    if (length == 0) return 0;
    return (length * (GLYPH_W + GLYPH_SPACING) - GLYPH_SPACING) * pixelSize;
}
