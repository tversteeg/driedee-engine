#include "l_console.h"

static void setMaxLines(console_t *con)
{
	con->maxlines = con->buffer->height / con->font->size;
}

void initConsole(console_t *con, unsigned int width, unsigned int height)
{
	initTexture(&con->buffer, width, height);	
	
	con->font = loadDefaultFont();
	setMaxLines(con);
}

void resizeConsole(console_t *con, unsigned int width, unsigned int height)
{
	resizeTexture(&con->buffer, width, height);
	setMaxLines(con);

void setConsoleFont(font_t *font)
{
	con->font = font;
	setMaxLines(con);
}
