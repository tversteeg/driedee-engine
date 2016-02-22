#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>

#include <ccore/display.h>
#include <ccore/window.h>
#include <ccore/opengl.h>
#include <ccore/time.h>
#include <ccore/file.h>

#ifdef WINDOWS
#include <gl/GL.h>
#else
#include <GL/glew.h>
#endif

#include <driedee/colors.h>
#include <driedee/draw.h>
#include "g_commands.h"
#include "g_game.h"

GLuint gltex;
texture_t screentex;
cctTerm term;
ccfFont font;

unsigned int screenwidth, screenheight;

void loadFont(const char *file)
{
	unsigned flen = ccFileInfoGet(file).size;

	FILE *fp = fopen(file, "rb");
	if(!fp){
		fprintf(stderr, "Can not open file: %s\n", file);
		exit(1);
	}

	uint8_t *bin = (uint8_t*)malloc(flen);
	fread(bin, 1, flen, fp);

	fclose(fp);

	if(ccfBinToFont(&font, bin, flen) == -1){
		fprintf(stderr, "Binary font failed: invalid version\n");
		exit(1);
	}

	free(bin);
}

void renderTexture(texture_t tex)
{
	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, gltex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tex.width, tex.height, 0, GL_RGB, GL_UNSIGNED_BYTE, tex.pixels);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(-1.0f, 1.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(-1.0f, -1.0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(1.0f, -1.0f);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(1.0f, 1.0f);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);
}

int main(int argc, char **argv)
{
	//TODO set screen size with getopts
	screenwidth = 800;
	screenheight = 600;

	loadFont("font.ccf");

	initTexture(&screentex, screenwidth, screenheight);

	cctCreate(&term, screenwidth, screenheight);
	cctSetFont(&term, &font);

	cctPrintf(&term, "PRESS TAB TO VIEW AVAILABLE COMMANDS, AND F1 TO TOGGLE THE CONSOLE\n");

	mapConsoleCmds(&term);

	initGameWorld(&term);

	ccDisplayInitialize();

	ccWindowCreate((ccRect){0, 0, screenwidth, screenheight}, "Rogueliek", 0);
	ccWindowMouseSetCursor(CC_CURSOR_NONE);

	ccGLContextBind();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	glBindTexture(GL_TEXTURE_2D, 0);
	glGenTextures(1, &gltex);
	glBindTexture(GL_TEXTURE_2D, gltex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	bool termactive = false;
	bool loop = true;
	while(loop){
		while(ccWindowEventPoll()){
			ccEvent event = ccWindowEventGet();
			switch(event.type){
				case CC_EVENT_WINDOW_QUIT:
					loop = false;
					break;
				case CC_EVENT_WINDOW_RESIZE:
					screenwidth = ccWindowGetRect().width;
					screenheight = ccWindowGetRect().height;
					resizeTexture(&screentex, screenwidth, screenheight);
					cctResize(&term, screenwidth, screenheight >> 2);
					break;
				case CC_EVENT_KEY_DOWN:
					if(event.keyCode == CC_KEY_ESCAPE){
						loop = false;
					}else if(event.keyCode == CC_KEY_F1){
						termactive = !termactive;
					}
					if(!termactive){
						inputGameWorld(event);
					}
					break;
				case CC_EVENT_KEY_UP:
					if(!termactive){
						inputGameWorld(event);
					}
					break;
				default: break;
			}
			if(termactive){
				cctHandleEvent(&term, event);
			}
		}

		updateGameWorld();

		renderGameWorld(&screentex);
		renderTexture(screentex);
		clearTexture(&screentex, COLOR_BLACK);

		if(termactive){
			cctRender(&term, gltex);
		}

		ccGLBuffersSwap();

		ccTimeDelay(6);
	}

	ccFree();

	return 0;
}
