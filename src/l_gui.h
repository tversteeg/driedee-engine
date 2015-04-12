#pragma once

#include "l_draw.h"
#include "l_colors.h"

#define DEFAULT_BACKGROUND_UP_COLOR COLOR_GRAY
#define DEFAULT_BACKGROUND_DOWN_COLOR COLOR_DARKGRAY
#define DEFAULT_BACKGROUND_HOVER_COLOR COLOR_LIGHTGRAY

typedef enum {BUTTON_STATE_DOWN, BUTTON_STATE_UP, BUTTON_STATE_HOVER} buttonstate_t;

typedef enum {
	EVENT_ON_MOUSE_DOWN,
	EVENT_ON_MOUSE_UP,
	EVENT_ON_MOUSE_OVER,
	EVENT_ON_MOUSE_OUT
} guievent_t;

typedef struct {
	int x, y, width, height;
	buttonstate_t state;
	const font_t *font;
	char *text;
	bool hold;
} simplebutton_t;

typedef struct {
	unsigned long id;
	int x, y, width, height;
	char *text;
	const font_t *font;
	void (*onMouseDownEvent)();
	void (*onMouseUpEvent)();
	void (*onMouseOverEvent)();
	void (*onMouseOutEvent)();
} button_t;

typedef struct {
	int x, y;
	const font_t *font;
	char *text;
	pixel_t color;
} simpletextfield_t;

void initializeSimpleButton(simplebutton_t *button, int x, int y, int width, int height, const font_t *font, const char *text);
void renderSimpleButton(texture_t *tex, const simplebutton_t *button);
void handleMouseSimpleButton(simplebutton_t *button, int x, int y, bool mousepressed);

void initializeSimpleTextField(simpletextfield_t *field, int x, int y, const font_t *font, const char *text, pixel_t color);
void renderSimpleTextField(texture_t *tex, const simpletextfield_t *field);

bool loadGuiFromFile(const char *file);
void renderGui(texture_t *tex);
void bindFont(const font_t *font, const char *name);
void bindEvent(const char *name, void (*event)(), guievent_t type);
