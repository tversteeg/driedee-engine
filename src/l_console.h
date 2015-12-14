#pragma once

#include <ccore/event.h>

#include "l_draw.h"

#define MAX_CMD_LEN 128

typedef struct _console_t console_t;

typedef void (*cmdptr_t) (console_t *con, int argc, char **argv);

struct _console_t{
	font_t *font;
	texture_t tex;
	char *buf, cmdstr[MAX_CMD_LEN], **cmdnames;
	cmdptr_t *cmdfs;
	bool active;
	unsigned int bufmaxsize, buflen, buflines, bufmaxlines, cmdstrlen, cmds;
};

void initConsole(console_t *con, unsigned int width, unsigned int height);
void resizeConsole(console_t *con, unsigned int width, unsigned int height);
void setConsoleFont(console_t *con, font_t *font);

void renderConsole(console_t *con, texture_t *target);

void inputConsole(console_t *con, ccEvent event);

void printConsole(console_t *con, const char *text);

void mapCmdConsole(console_t *con, const char *cmd, cmdptr_t cmdfunction);
