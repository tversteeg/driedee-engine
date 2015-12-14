#include "l_console.h"

#include <string.h>

#include "l_colors.h"

static void refresh(console_t *con)
{
	clearTexture(&con->tex, COLOR_MASK);

	if(con->len > 0){
		drawString(&con->tex, con->font, con->buf, 0, 0, COLOR_WHITE);
	}

	unsigned int cmdheight = con->tex.height - 2 - con->font->size;
	drawLetter(&con->tex, con->font, '>', 2, cmdheight, COLOR_WHITE);
	if(con->cmdlen > 0){
		drawString(&con->tex, con->font, con->cmd, con->font->size + 2, cmdheight, COLOR_WHITE);
	}

	drawLine(&con->tex, (xy_t){0, con->tex.height - 1}, (xy_t){con->tex.width, con->tex.height - 1}, COLOR_WHITE);
	drawLine(&con->tex, (xy_t){0, con->tex.height - 1}, (xy_t){con->tex.width, con->tex.height - 1}, COLOR_WHITE);
}

static void setMaxLines(console_t *con)
{
	con->maxlines = con->tex.height / con->font->size - 1;

	refresh(con);
}

static void performCommand(console_t *con)
{
	printConsole(con, con->cmd);
	printConsole(con, "\n");
}

void initConsole(console_t *con, unsigned int width, unsigned int height)
{
	initTexture(&con->tex, width, height);	
	
	con->font = loadDefaultFont();
	setMaxLines(con);

	con->active = true;

	con->size = 32;
	con->buf = (char*)calloc(con->size, sizeof(char));
	con->len = con->cmdlen = con->lines = 0;
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

void inputConsole(console_t *con, ccEvent event)
{
	if(event.keyCode == CC_KEY_BACKSPACE){
		if(con->cmdlen > 0){
			con->cmd[--con->cmdlen] = '\0';
			refresh(con);
			return;
		}
	}

	char key = ccEventKeyToChar(event.keyCode);
	if(key == '\0'){
		return;
	}else	if(key == '\n'){
		performCommand(con);
		con->cmdlen = 0;
		con->cmd[0] = '\0';
	}else if(con->cmdlen < MAX_CMD_LEN){
		con->cmd[con->cmdlen] = key;
		con->cmd[++con->cmdlen] = '\0';
	}

	refresh(con);
}

void printConsole(console_t *con, const char *text)
{
	size_t len = strlen(text);

	// Remove the first line when the new linebreak is bigger then maxlines
	unsigned int i;
	for(i = 0; i < len; i++){
		if(text[i] == '\n'){
			con->lines++;
		}
	}

	if(con->lines > con->maxlines){
		for(i = 0; i < con->len; i++){
			if(con->buf[i] == '\n'){
				con->lines--;
				if(con->lines == con->maxlines){
					i++;
					con->len -= i;
					memmove(con->buf, con->buf + i, con->len);
					break;
				}
			}
		}
	}

	size_t total = con->len + len;
	while(total > con->size){
		con->size *= 2;

		//TODO make the realloc safe
		con->buf = (char*)realloc(con->buf, con->size);
	}

	strcpy(con->buf + con->len, text);
	con->len = total;
	
	refresh(con);
}
