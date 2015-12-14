#pragma once

#include "l_draw.h"

typedef struct {
	font_t *font;
	texture_t tex;
	char *buf;
	size_t size, len;
	bool active;
	unsigned int maxlines;
} console_t;

void initConsole(console_t *con, unsigned int width, unsigned int height);
void resizeConsole(console_t *con, unsigned int width, unsigned int height);
void setConsoleFont(console_t *con, font_t *font);

void renderConsole(console_t *con, texture_t *target);

void printConsole(console_t *con, const char *text);
