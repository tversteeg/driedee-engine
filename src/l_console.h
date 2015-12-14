#pragma once

#include <ccore/event.h>

#include "l_draw.h"

#define MAX_CMD_LEN 128

typedef struct {
	font_t *font;
	texture_t tex;
	char *buf, cmd[MAX_CMD_LEN];
	bool active;
	unsigned int size, len, lines, maxlines, cmdlen;
} console_t;

void initConsole(console_t *con, unsigned int width, unsigned int height);
void resizeConsole(console_t *con, unsigned int width, unsigned int height);
void setConsoleFont(console_t *con, font_t *font);

void renderConsole(console_t *con, texture_t *target);

void inputConsole(console_t *con, ccEvent event);

void printConsole(console_t *con, const char *text);
