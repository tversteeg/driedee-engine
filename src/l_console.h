#pragma once

#include "l_draw.h"

typedef struct {
	font_t *font;
	texture_t buffer;
	unsigned int maxlines;
} console_t;

void initConsole(console_t *con, unsigned int width, unsigned int height);
void resizeConsole(console_t *con, unsigned int width, unsigned int height);

void setConsoleFont(font_t *font);
