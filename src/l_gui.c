#include "l_gui.h"

#include "l_utils.h"

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

static textfield_t *textfields = NULL;
static unsigned int ntextfields = 0;

static void renderTextfield(texture_t *tex, const textfield_t *field)
{
	drawString(tex, field->font, field->text, field->x, field->y, field->color);
}

static void renderButton(texture_t *tex, button_t *button)
{
	int x = (button->width - button->font->width * strlen(button->text)) / 2 + button->x;
	int y = (button->height - button->font->height) / 2 + button->y;

	drawString(tex, button->font, button->text, x, y, button->color);
}

bool loadGuiFromFile(const char *file)
{
	config_t config;
	config_init(&config);
	if(!config_read_file(&config, file)){
		config_destroy(&config);
		printf("Error: Could not load gui configuration file \"%s\"\n", file);
		return false;
	}

	config_setting_t *setting = config_lookup(&config, "buttons");

	nbuttons = config_setting_length(setting);
	buttons = (button_t*)calloc(nbuttons, sizeof(button_t));
	int i;
	for(i = 0; i < nbuttons; i++){
		button_t *button = buttons + i;
		config_setting_t *elem = config_setting_get_elem(setting, i);

		config_setting_lookup_int(elem, "x", &button->x);
		config_setting_lookup_int(elem, "y", &button->y);
		config_setting_lookup_int(elem, "width", &button->width);
		config_setting_lookup_int(elem, "height", &button->height);

		const char *text;
		config_setting_lookup_string(elem, "text", &text);
		button->text = (char*)malloc(strlen(text));
		strcpy(button->text, text);

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

		const char *name;
		config_setting_lookup_string(elem, "name", &name);
		button->id = hash(name);

		const char *color;
		config_setting_lookup_string(elem, "color", &color);
		button->color = strtopixel(color);
	}

	setting = config_lookup(&config, "textfields");
	ntextfields = config_setting_length(setting);
	textfields = (textfield_t*)calloc(ntextfields, sizeof(textfield_t));
	for(i = 0; i < ntextfields; i++){
		textfield_t *textfield = textfields + i;
		config_setting_t *elem = config_setting_get_elem(setting, i);

		config_setting_lookup_int(elem, "x", &textfield->x);
		config_setting_lookup_int(elem, "y", &textfield->y);

		const char *text;
		config_setting_lookup_string(elem, "text", &text);
		textfield->text = (char*)malloc(strlen(text));
		strcpy(textfield->text, text);

		const char *fontname;
		config_setting_lookup_string(elem, "font", &fontname);
		int j;
		for(j = 0; j < nboundfonts; j++){
			boundfont_t bound = boundfonts[j];
			if(strcmp(bound.name, fontname) == 0){
				textfield->font = bound.font;
				break;
			}
		}

		const char *name;
		config_setting_lookup_string(elem, "name", &name);
		textfield->id = hash(name);

		const char *color;
		config_setting_lookup_string(elem, "color", &color);
		textfield->color = strtopixel(color);
	}

	config_destroy(&config);

	return true;
}

void renderGui(texture_t *tex)
{
	unsigned int i;
	for(i = 0; i < nbuttons; i++){
		renderButton(tex, buttons + i);
	}
	for(i = 0; i < ntextfields; i++){
		renderTextfield(tex, textfields + i);
	}
}

void updateGui(int mousex, int mousey, bool mousedown)
{
	int i;
	for(i = 0; i < nbuttons; i++){
		button_t *button = buttons + i;
		bool ismouseover = mousex >= button->x && mousex <= button->x + button->width &&
			mousey >= button->y && mousey <= button->y + button->height;

		if(button->onMouseOverEvent != NULL && 
				button->state == BUTTON_STATE_OUT && ismouseover){
			button->onMouseOverEvent(button);
		}else if(button->onMouseOutEvent != NULL && 
				button->state == BUTTON_STATE_OVER && !ismouseover){
			button->onMouseOutEvent(button);
		}else	if(button->onMouseUpEvent != NULL && 
				button->state == BUTTON_STATE_DOWN && !mousedown){
			button->onMouseUpEvent(button);
		}else if(button->onMouseDownEvent != NULL && 
				button->state != BUTTON_STATE_DOWN && mousedown && ismouseover){
			button->onMouseDownEvent(button);
		}

		if(ismouseover){
			if(mousedown){
				button->state = BUTTON_STATE_DOWN;
			}else{
				button->state = BUTTON_STATE_OVER;
			}
		}else{
			button->state = BUTTON_STATE_OUT;
		}
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

void bindButtonEvent(const char *name, void (*event)(button_t*), guievent_t type)
{
	unsigned long id = hash(name);

	int i;
	for(i = 0; i < nbuttons; i++){
		button_t *button = buttons + i;
		if(button->id == id){
			switch(type){
				case EVENT_ON_MOUSE_DOWN:
					button->onMouseDownEvent = event;
					break;
				case EVENT_ON_MOUSE_UP:
					button->onMouseUpEvent = event;
					break;
				case EVENT_ON_MOUSE_OVER:
					button->onMouseOverEvent = event;
					break;
				case EVENT_ON_MOUSE_OUT:
					button->onMouseOutEvent = event;
					break;
			}
		}
	}
}
