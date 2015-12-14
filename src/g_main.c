#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ccore/display.h>
#include <ccore/window.h>
#include <ccore/opengl.h>
#include <ccore/time.h>

#ifdef WINDOWS
#include <gl/GL.h>
#else
#include <GL/glew.h>
#endif

#include "l_draw.h"
#include "l_console.h"

GLuint gltex;
texture_t screentex;
console_t console;

unsigned int screenwidth, screenheight;

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

	initTexture(&screentex, screenwidth, screenheight);
	initConsole(&console, screenwidth >> 1, screenheight >> 2);

	printConsole(&console, "\\BWelcome to the \"Rogueliek\" console!\\d\n");
	printConsole(&console, "Press \\RTAB\\d to view the available commands\n");
	
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
					break;
				default: break;
			}
		}

		renderConsole(&console, &screentex);
		renderTexture(screentex);
		ccGLBuffersSwap();

		ccTimeDelay(6);
	}

	ccFree();

	return 0;
}
