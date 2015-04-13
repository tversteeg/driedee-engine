#pragma once

#include "l_draw.h"
#include "l_colors.h"

#define DEFAULT_BACKGROUND_UP_COLOR COLOR_GRAY
#define DEFAULT_BACKGROUND_DOWN_COLOR COLOR_DARKGRAY
#define DEFAULT_BACKGROUND_HOVER_COLOR COLOR_LIGHTGRAY

typedef enum {
	BUTTON_STATE_DOWN, 
	BUTTON_STATE_OVER, 
	BUTTON_STATE_OUT
} buttonstate_t;

typedef enum {
	EVENT_ON_MOUSE_DOWN,
	EVENT_ON_MOUSE_UP,
	EVENT_ON_MOUSE_OVER,
	EVENT_ON_MOUSE_OUT
} guievent_t;

typedef struct _button_t button_t;

struct _button_t {
	unsigned long id;
	int x, y, width, height;
	char *text;
	const font_t *font;
	void (*onMouseDownEvent)(button_t *self);
	void (*onMouseUpEvent)(button_t *self);
	void (*onMouseOverEvent)(button_t *self);
	void (*onMouseOutEvent)(button_t *self);
	buttonstate_t state;
};

typedef struct {
	int x, y;
	const font_t *font;
	char *text;
	pixel_t color;
} simpletextfield_t;

void initializeSimpleTextField(simpletextfield_t *field, int x, int y, const font_t *font, const char *text, pixel_t color);
void renderSimpleTextField(texture_t *tex, const simpletextfield_t *field);

bool loadGuiFromFile(const char *file);
void renderGui(texture_t *tex);
void updateGui(int mousex, int mousey, bool mousedown);

void bindFont(const font_t *font, const char *name);
void bindEvent(const char *name, void (*event)(button_t*), guievent_t type);
