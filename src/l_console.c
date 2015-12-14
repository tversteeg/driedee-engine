#include "l_console.h"

#include <string.h>

#include "l_colors.h"

static void refresh(console_t *con)
{
	clearTexture(&con->tex, COLOR_MASK);

	if(con->len > 0){
		drawString(&con->tex, con->font, con->buf, 2, 2, COLOR_WHITE);
	}

	drawLine(&con->tex, (xy_t){0, con->tex.height - 1}, (xy_t){con->tex.width, con->tex.height - 1}, COLOR_WHITE);
	drawLine(&con->tex, (xy_t){0, con->tex.height - 1}, (xy_t){con->tex.width, con->tex.height - 1}, COLOR_WHITE);
}

static void setMaxLines(console_t *con)
{
	con->maxlines = con->tex.height / con->font->size;

	refresh(con);
}

void initConsole(console_t *con, unsigned int width, unsigned int height)
{
	initTexture(&con->tex, width, height);	
	
	con->font = loadDefaultFont();
	setMaxLines(con);

	con->active = true;

	con->size = 32;
	con->buf = (char*)calloc(con->size, sizeof(char));
	con->len = 0;
}

void resizeConsole(console_t *con, unsigned int width, unsigned int height)
{
	resizeTexture(&con->tex, width, height);
	setMaxLines(con);
}

void setConsoleFont(console_t *con, font_t *font)
{
	con->font = font;
	setMaxLines(con);
}

void renderConsole(console_t *con, texture_t *target)
{
	if(con->active){
		v_t scale = target->width / con->tex.width;
		drawTextureScaled(target, &con->tex, 0, 0, (xy_t){scale, scale});
	}
}

void printConsole(console_t *con, const char *text)
{
	size_t len = strlen(text);
	size_t total = con->len + len;

	while(total > con->size){
		con->size <<= 1;

		//TODO make the realloc safe
		con->buf = (char*)realloc(con->buf, con->size);
	}

	strcpy(con->buf + con->len, text);
	con->len += len;
	
	refresh(con);
}
