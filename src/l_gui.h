#pragma once

#include "l_draw.h"
#include "l_colors.h"
#include "l_utils.h"

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
	hash_t id;
	int x, y, width, height;
	char *text;
	const font_t *font;
	void (*onMouseDownEvent)(button_t *self);
	void (*onMouseUpEvent)(button_t *self);
	void (*onMouseOverEvent)(button_t *self);
	void (*onMouseOutEvent)(button_t *self);
	buttonstate_t state;
	pixel_t color, background;
};

typedef struct {
	hash_t id;
	int x, y;
	char *text;
	const font_t *font;
	pixel_t color;
} textfield_t;

bool loadGuiFromFile(const char *file);
void renderGui(texture_t *tex);
void updateGui(int mousex, int mousey, bool mousedown);

void bindFont(const font_t *font, const char *name);
void bindButtonEvent(const char *name, void (*event)(button_t*), guievent_t type);

button_t* getButtonByName(const char *name);
button_t* getButtonByHash(hash_t id);

textfield_t* getTextfieldByName(const char *name);
textfield_t* getTextfieldByHash(hash_t id);
