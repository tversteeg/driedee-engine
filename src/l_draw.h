#pragma once

#include "l_vector.h"
#include "l_utils.h"

typedef struct {
	unsigned char r, g, b, a;
} pixel_t;

typedef struct {
	int width, height;
	pixel_t *pixels;
} texture_t;

typedef struct {
	char start;
	unsigned char letters, size;
	int totalwidth;
	bool *pixels;
} font_t;

void initTexture(texture_t *tex, unsigned int width, unsigned int height);
void initFont(font_t *font, unsigned int width, unsigned int height);
void loadFont(font_t *font, char start, const bool *pixels);

void clearTexture(texture_t *tex, pixel_t pixel);

bool samePixel(pixel_t p1, pixel_t p2);
bool getPixel(const texture_t *tex, pixel_t *pixel, unsigned int x, unsigned int y);
// Force setting the pixel in a unsafe way
void setPixel(texture_t *tex, unsigned int x, unsigned int y, pixel_t pixel);
// Set the pixel with a boundary check
void drawPixel(texture_t *tex, int x, int y, pixel_t pixel);

void drawLine(texture_t *tex, xy_t p1, xy_t p2, pixel_t pixel);
void drawCircle(texture_t *tex, xy_t p, unsigned int radius, pixel_t pixel);
void drawRect(texture_t *tex, xy_t p, unsigned int width, unsigned int height, pixel_t pixel);

void drawLetter(texture_t *tex, const font_t *font, char letter, int x, int y, pixel_t pixel);
void drawString(texture_t *tex, const font_t *font, const char *text, int x, int y, pixel_t pixel);

void drawTexture(texture_t *target, const texture_t *source, int x, int y);
void drawTextureSlice(texture_t *target, const texture_t *source, int x, int y, int height, double uvx);
void drawTextureScaled(texture_t *target, const texture_t *source, int x, int y, xy_t scale);

void drawGrid(texture_t *tex, unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int gridwidth, unsigned int gridheight, pixel_t pixel);

pixel_t strtopixel(const char *hexstr);
