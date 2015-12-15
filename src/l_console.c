#include "l_console.h"

#include <string.h>

#include "l_colors.h"

static void refresh(console_t *con)
{
	clearTexture(&con->tex, COLOR_MASK);

	if(con->buflen > 0){
		drawString(&con->tex, con->font, con->buf, 0, 0, COLOR_WHITE);
	}

	unsigned int cmdheight = con->tex.height - 2 - con->font->size;
	drawLetter(&con->tex, con->font, '>', 2, cmdheight, COLOR_WHITE);
	if(con->cmdstrlen > 0){
		drawString(&con->tex, con->font, con->cmdstr, con->font->size + 2, cmdheight, COLOR_WHITE);
	}

	drawLine(&con->tex, (xy_t){0, con->tex.height - 1}, (xy_t){con->tex.width, con->tex.height - 1}, COLOR_WHITE);
	drawLine(&con->tex, (xy_t){0, con->tex.height - 1}, (xy_t){con->tex.width, con->tex.height - 1}, COLOR_WHITE);
}

static void setMaxLines(console_t *con)
{
	con->bufmaxlines = con->tex.height / con->font->size - 1;

	refresh(con);
}

static void performCommand(console_t *con)
{
	unsigned int i;
	for(i = 0; i < con->cmds; i++){
		if(strncmp(con->cmdnames[i], con->cmdstr, strlen(con->cmdnames)) == 0){
			con->cmdfs[i](con, 0, NULL);
			return;
		}
	}
	printConsole(con, "Command \"");
	printConsole(con, con->cmdstr);
	printConsole(con, "\" not found!\n");
}

void initConsole(console_t *con, unsigned int width, unsigned int height)
{
	initTexture(&con->tex, width, height);	
	
	con->font = loadDefaultFont();
	con->active = true;

	con->bufmaxsize = 32;
	con->buf = (char*)calloc(con->bufmaxsize, sizeof(char));
	con->cmdnames = NULL;
	con->cmdfs = NULL;
	con->buflen = con->cmdstrlen = con->buflines = con->cmds = 0;

	setMaxLines(con);
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
		if(con->cmdstrlen > 0){
			con->cmdstr[--con->cmdstrlen] = '\0';
			refresh(con);
			return;
		}
	}

	char key = ccEventKeyToChar(event.keyCode);
	if(key == '\0'){
		return;
	}else	if(key == '\n'){
		performCommand(con);
		con->cmdstrlen = 0;
		con->cmdstr[0] = '\0';
	}else if(key == '\t'){
		bool containsspace = false;
		unsigned int i;
		for(i = 0; i < con->cmdstrlen; i++){
			if(con->cmdstr[i] == ' '){
				containsspace = true;
				break;
			}
		}
		if(!containsspace){
			// Do autocompletion
			unsigned int count = 0;
			int cmd = -1;
			for(i = 0; i < con->cmds; i++){
				if(strncmp(con->cmdnames[i], con->cmdstr, con->cmdstrlen) == 0){
					count++;
					cmd = i;
				}
			}
			if(count > 1){
				unsigned int shortest = 0, distance = 1000;
				for(i = 0; i < con->cmds; i++){
					if(strncmp(con->cmdnames[i], con->cmdstr, con->cmdstrlen) == 0){
						printConsole(con, "\t");
						printConsole(con, con->cmdnames[i]);
						
						// Check which strings have the most in common
						if(i > 0 && con->cmdstrlen > 0){
							char *c1 = con->cmdnames[i];
							char *c2 = con->cmdnames[shortest];
							unsigned int d = 0;
							while(*(c1++) == *(c2++)){
								d++;
							}
							if(d < distance){
								distance = d;
								shortest = i;
							}
						}
					}
				}
				printConsole(con, "\n");

				if(distance > con->cmdstrlen && distance < 1000){
					con->cmdstrlen = distance;
					memcpy(con->cmdstr, con->cmdnames[cmd], con->cmdstrlen);
				}
			}else if(count == 1){
				con->cmdstrlen = strlen(con->cmdnames[cmd]) + 1;
				memcpy(con->cmdstr, con->cmdnames[cmd], con->cmdstrlen);
				con->cmdstr[con->cmdstrlen - 1] = ' ';
				con->cmdstr[con->cmdstrlen] = '\0';
			}
		}else{
			// Insert tab character
			con->cmdstr[con->cmdstrlen] = key;
			con->cmdstr[++con->cmdstrlen] = '\0';
		}
	}else if(con->cmdstrlen < MAX_CMD_LEN){
		con->cmdstr[con->cmdstrlen] = key;
		con->cmdstr[++con->cmdstrlen] = '\0';
	}

	refresh(con);
}

void printConsole(console_t *con, const char *text)
{
	size_t len = strlen(text);

	// Remove the first line when the new linebreak is bigger than maxlines
	//TODO add a newline when the total line width is bigger than the screen
	unsigned int i;
	for(i = 0; i < len; i++){
		if(text[i] == '\n'){
			con->buflines++;
		}
	}

	if(con->buflines > con->bufmaxlines){
		for(i = 0; i < con->buflen; i++){
			if(con->buf[i] == '\n'){
				con->buflines--;
				if(con->buflines == con->bufmaxlines){
					i++;
					con->buflen -= i;
					memmove(con->buf, con->buf + i, con->buflen);
					break;
				}
			}
		}
	}

	size_t total = con->buflen + len;
	while(total > con->bufmaxsize){
		con->bufmaxsize *= 2;

		//TODO make the realloc safe
		con->buf = (char*)realloc(con->buf, con->bufmaxsize);
	}

	strcpy(con->buf + con->buflen, text);
	con->buflen = total;
	
	refresh(con);
}

void mapCmdConsole(console_t *con, const char *cmd, cmdptr_t cmdfunction)
{
	if(con->cmds == 0){
		con->cmdnames = (char**)malloc(sizeof(char*));
		con->cmdfs = (cmdptr_t*)malloc(sizeof(cmdptr_t));
	}else{
		con->cmdnames = (char**)realloc(con->cmdnames, sizeof(char*) * (con->cmds + 1));
		con->cmdfs = (cmdptr_t*)realloc(con->cmdfs, sizeof(cmdptr_t) * (con->cmds + 1));
	}

	unsigned int len = strlen(cmd);
	con->cmdnames[con->cmds] = (char*)malloc(len + 1);
	memcpy(con->cmdnames[con->cmds], cmd, len);
	con->cmdnames[con->cmds][len] = '\0';

	con->cmdfs[con->cmds] = cmdfunction;

	con->cmds++;
}
