#include "l_gui.h"

#include <stdlib.h>
#include <string.h>

#include <libconfig.h>

typedef struct {
	char *name;
	const font_t *font;
} boundfont_t;

static boundfont_t *boundfonts = NULL;
static unsigned int nboundfonts = 0;

static button_t *buttons = NULL;
static unsigned int nbuttons = 0;

void initializeSimpleButton(simplebutton_t *button, int x, int y, int width, int height, const font_t *font, const char *text)
{
	button->state = BUTTON_STATE_UP;
	button->x = x;
	button->y = y;
	button->width = width;
	button->height = height;
	button->font = font;
	button->hold = false;
	button->text = (char*)malloc(strlen(text));
	strcpy(button->text, text);
}

void renderSimpleButton(texture_t *tex, const simplebutton_t *button)
{
	pixel_t backgroundcolor;
	switch(button->state){
		case BUTTON_STATE_DOWN:
			backgroundcolor = DEFAULT_BACKGROUND_DOWN_COLOR;
			break;
		case BUTTON_STATE_UP:
			backgroundcolor = DEFAULT_BACKGROUND_UP_COLOR;
			break;
		case BUTTON_STATE_HOVER:
			backgroundcolor = DEFAULT_BACKGROUND_HOVER_COLOR;
			break;
	}

	drawRect(tex, (xy_t){(double)button->x, (double)button->y}, button->width, button->height, backgroundcolor);
	drawLine(tex, (xy_t){(double)button->x, (double)button->y}, (xy_t){(double)button->x, (double)(button->y + button->height - 1)}, COLOR_WHITE);
	drawLine(tex, (xy_t){(double)button->x, (double)button->y}, (xy_t){(double)(button->x + button->width), (double)button->y}, COLOR_WHITE);

	int x = (button->width - button->font->width * strlen(button->text)) / 2 + button->x;
	int y = (button->height - button->font->height) / 2 + button->y;

	drawString(tex, button->font, button->text, x, y, COLOR_BLACK);
}

void handleMouseSimpleButton(simplebutton_t *button, int x, int y, bool mousepressed)
{
	bool ismouseover = x >= button->x && x <= button->x + button->width &&
		y >= button->y && y <= button->y + button->height;

	if(!button->hold){
		button->state = BUTTON_STATE_UP;
	}
	if(ismouseover){
		if(button->state == BUTTON_STATE_UP){
			button->state = BUTTON_STATE_HOVER;
		}
		if(mousepressed){
			if(button->state == BUTTON_STATE_HOVER){
				button->state = BUTTON_STATE_DOWN;
			}else{
				button->state = BUTTON_STATE_HOVER;
			}
		}
	}else if(button->state == BUTTON_STATE_HOVER){
		button->state = BUTTON_STATE_UP;
	}
}

void initializeSimpleTextField(simpletextfield_t *field, int x, int y, const font_t *font, const char *text, pixel_t color)
{
	field->x = x;
	field->y = y;
	field->font = font;
	field->color = color;
	field->text = (char*)malloc(strlen(text));
	strcpy(field->text, text);
}

void renderSimpleTextField(texture_t *tex, const simpletextfield_t *field)
{
	drawString(tex, field->font, field->text, field->x, field->y, field->color);
}

bool loadGuiFromFile(const char *file)
{
	config_t config;
	config_init(&config);
	if(!config_read_file(&config, file)){
		config_destroy(&config);
		return false;
	}

	config_setting_t *setting = config_lookup(&config, "buttons");

	nbuttons = config_setting_length(setting);
	buttons = (button_t*)malloc(sizeof(button_t) * nbuttons);
	int i;
	for(i = 0; i < nbuttons; i++){
		button_t *button = buttons + i;
		config_setting_t *elem = config_setting_get_elem(setting, i);

		config_setting_lookup_int(elem, "x", &button->x);
		config_setting_lookup_int(elem, "y", &button->y);
		config_setting_lookup_int(elem, "width", &button->width);
		config_setting_lookup_int(elem, "height", &button->height);
		config_setting_lookup_string(elem, "text", (const char**)&button->text);

		const char *fontname;
		config_setting_lookup_string(elem, "font", &fontname);
		int j;
		for(j = 0; j < nboundfonts; j++){
			boundfont_t bound = boundfonts[j];
			if(strcmp(bound.name, fontname) == 0){
				button->font = bound.font;
				break;
			}
		}
	}

	config_destroy(&config);

	return true;
}

void renderButton(texture_t *tex, button_t *button)
{
	int x = (button->width - button->font->width * strlen(button->text)) / 2 + button->x;
	int y = (button->height - button->font->height) / 2 + button->y;

	drawString(tex, button->font, button->text, x, y, COLOR_BLACK);
}

void renderGui(texture_t *tex)
{
	unsigned int i;
	for(i = 0; i < nbuttons; i++){
		renderButton(tex, buttons + i);
	}
}

void bindFont(const font_t *font, const char *name)
{
	boundfonts = (boundfont_t*)realloc(boundfonts, ++nboundfonts * sizeof(boundfont_t));
	boundfont_t *bound = boundfonts + nboundfonts - 1;

	bound->font = font;
	bound->name = (char*)malloc(strlen(name));
	strcpy(bound->name, name);
}
