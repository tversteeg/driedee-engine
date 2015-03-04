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
	char letters, start, letterwidth, letterheight;
	unsigned int width, height;
	bool *pixels;
} font_t;

void initTexture(texture_t *tex, unsigned int width, unsigned int height);
void initFont(font_t *font, unsigned int width, unsigned int height);

inline bool getPixel(texture_t *tex, pixel_t *pixel, unsigned x, unsigned y);
inline void setPixel(texture_t *tex, unsigned x, unsigned y, pixel_t pixel);

void drawLine(texture_t *tex, xy_t p1, xy_t p2, char r, char g, char b, char a);
void drawCircle(xy_t p, unsigned int radius, char r, char g, char b, char a);

void drawLetter(texture_t *tex, const font_t *font, char letter, unsigned int x, unsigned int y, char r, char g, char b, char a);
void drawString(texture_t *tex, const font_t *font, const char *text, unsigned int x, unsigned int y, char r, char g, char b, char a);
