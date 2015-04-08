#include "l_gui.h"

#include <string.h>

void initializeSimpleButton(simplebutton_t *button, int x, int y, int width, int height, const font_t *font, const char *text)
{
	button->state = BUTTON_STATE_UP;
	button->x = x;
	button->y = y;
	button->width = width;
	button->height = height;
	button->font = font;
	button->text = (char*)malloc(strlen(text));
	button->hold = false;
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
	int y = (button->width - button->font->height) / 2 + button->y;

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
