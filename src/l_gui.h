#pragma once

#include "l_draw.h"
#include "l_colors.h"

#define DEFAULT_BACKGROUND_UP_COLOR COLOR_GRAY
#define DEFAULT_BACKGROUND_DOWN_COLOR COLOR_DARKGRAY
#define DEFAULT_BACKGROUND_HOVER_COLOR COLOR_LIGHTGRAY

typedef enum {BUTTON_STATE_DOWN, BUTTON_STATE_UP, BUTTON_STATE_HOVER} buttonstate_t;

typedef struct {
	int x, y, width, height;
	buttonstate_t state;
	const font_t *font;
	char *text;
} simplebutton_t;

void initializeSimpleButton(simplebutton_t *button, int x, int y, int width, int height, const font_t *font, const char *text);
void renderSimpleButton(texture_t *tex, const simplebutton_t *button);
