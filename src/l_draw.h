#pragma once

#include "l_vector.h"

typedef struct {
	char r, g, b, a;
} pixel_t;

typedef struct {
	unsigned int width, height;
	pixel_t *pixels;
} texture_t;

typedef struct {
	char letters, start, width, height;
	unsigned int totalwidth;
	bool *pixels;
} font_t;

void initTexture(texture_t *tex, unsigned int width, unsigned int height);
void initFont(font_t *font, unsigned int width, unsigned int height);
void loadFont(font_t *font, char start, char letters, char width, const bool *pixels);

inline bool getPixel(texture_t *tex, pixel_t *pixel, unsigned int x, unsigned int y);
// Force setting the pixel in a unsafe way
inline void setPixel(texture_t *tex, unsigned int x, unsigned int y, pixel_t pixel);
// Set the pixel with a boundary check
inline void drawPixel(texture_t *tex, int x, int y, pixel_t pixel);

void drawLine(texture_t *tex, xy_t p1, xy_t p2, char r, char g, char b, char a);
void drawCircle(xy_t p, unsigned int radius, char r, char g, char b, char a);

void drawLetter(texture_t *tex, const font_t *font, char letter, unsigned int x, unsigned int y, char r, char g, char b, char a);
void drawString(texture_t *tex, const font_t *font, const char *text, unsigned int x, unsigned int y, char r, char g, char b, char a);
