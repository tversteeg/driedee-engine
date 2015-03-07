#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <ccore/display.h>
#include <ccore/window.h>
#include <ccore/opengl.h>
#include <ccore/time.h>

#ifdef WINDOWS
#include <gl/GL.h>
#else
#include <GL/glew.h>
#endif

#include "PixelFont1.h"
#include "l_draw.h"
#include "l_vector.h"
#include "l_sector.h"

#define WIDTH 800
#define HEIGHT 600

#define MENU_HEIGHT 64

typedef enum {MOVEMENT_TOOL, REMOVAL_TOOL, VERTEX_TOOL, EDGE_ADD_TOOL, EDGE_CHANGE_TOOL, SECTOR_TOOL} tool_t;

GLuint texture;
texture_t tex;
font_t font;

sector_t *sectors = NULL;
unsigned int nsectors = 0;
xy_t *vertices = NULL;
unsigned int nvertices = 0;
edge_t *edges = NULL;
unsigned int nedges = 0;

bool snaptogrid = false;
tool_t toolselected = VERTEX_TOOL;
edgetype_t edgetypeselected = WALL;
int vertselected = -1;
int sectorselected = -1;
int gridsize = 10;
int snapsize = 10;

int xmouse, ymouse;

double distanceToSegment(xy_t p, xy_t p1, xy_t p2)
{
	xy_t seg = {p2.x - p1.x, p2.y - p1.y};
	xy_t cir = {p.x - p1.x, p.y - p1.y};
	double proj = vectorProjectScalar(cir, seg);

	xy_t closest;
	if(proj < 0){
		closest = p1;
	}else if(proj > sqrt(seg.x * seg.x + seg.y * seg.y)){
		closest = p2;
	}else{
		xy_t projv = vectorProject(cir, seg);
		closest = (xy_t){p1.x + projv.x, p1.y + projv.y};
	}

	double dx = p.x - closest.x;
	double dy = p.y - closest.y;

	return sqrt(dx * dx + dy * dy);
}
 
void deleteEdge(unsigned int index)
{
	if(index >= nedges){
		return;
	}

	memmove(edges + index, edges + index + 1, (nedges - index - 1) * sizeof(*edges));
	edges = (edge_t*)realloc(edges, --nedges * sizeof(*edges));
}

void deleteVertex(unsigned int index)
{
	if(index >= nvertices){
		return;
	}

	memmove(vertices + index, vertices + index + 1, (nvertices - index - 1) * sizeof(vertices[index]));
	vertices = (xy_t*)realloc(vertices, --nvertices * sizeof(*vertices));

	int i;
	for(i = 0; i < nedges; i++){
		if(edges[i].vertex1 == index || edges[i].vertex2 == index){
			deleteEdge(i);
			i--;
			continue;
		}
		if(edges[i].vertex1 > index){
			edges[i].vertex1--;
		}
		if(edges[i].vertex2 > index){
			edges[i].vertex2--;
		}
	}
}

void renderBackground()
{
	drawGrid(&tex, 0, 0, WIDTH, HEIGHT - MENU_HEIGHT, gridsize, gridsize, (pixel_t){32, 32, 32, 255});
}

void renderMenu()
{
	drawLine(&tex, (xy_t){0, HEIGHT - MENU_HEIGHT}, (xy_t){WIDTH, HEIGHT - MENU_HEIGHT}, (pixel_t){255, 255, 0, 255});

	int i;
	for(i = HEIGHT - MENU_HEIGHT + 1; i < HEIGHT; i++){
		drawLine(&tex, (xy_t){0, (double)i}, (xy_t){WIDTH, (double)i}, (pixel_t){16, 16, 16, 255});
	}

	char buffer[64];
	int pos = sprintf(buffer, "(9&0) GRID SIZE: (%dx%d)", gridsize, gridsize);
	buffer[pos] = '\0';
	drawString(&tex, &font, buffer, 8, HEIGHT - MENU_HEIGHT + 18, (pixel_t){0, 128, 128, 255});

	pos = sprintf(buffer, "(S) SNAP TO GRID: %s", snaptogrid ? "ON" : "OFF");
	buffer[pos] = '\0';
	drawString(&tex, &font, buffer, 8, HEIGHT - MENU_HEIGHT + 28, (pixel_t){0, 128, 128, 255});

	char toolname[64];
	switch(toolselected){
		case VERTEX_TOOL:
			strcpy(toolname, "ADD VERTEX");
			break;
		case EDGE_ADD_TOOL:
			switch(edgetypeselected){
				case WALL:
					strcpy(toolname, "ADD EDGE - (W&P) WALL");
					break;
				case PORTAL:
					strcpy(toolname, "ADD EDGE - (W&P) PORTAL");
					break;
			}
			break;
		case EDGE_CHANGE_TOOL:
			switch(edgetypeselected){
				case WALL:
					strcpy(toolname, "CHANGE EDGE TYPE - (W&P) WALL");
					break;
				case PORTAL:
					strcpy(toolname, "CHANGE EDGE TYPE - (W&P) PORTAL");
					break;
			}
			break;
		case SECTOR_TOOL:
			strcpy(toolname, "ADD SECTOR");
			break;
		case MOVEMENT_TOOL:
			strcpy(toolname, "MOVE VERTEX");
			break;
		case REMOVAL_TOOL:
			strcpy(toolname, "REMOVE EDGE/VERTEX");
			break;
		default:
			strcpy(toolname, "ERROR");
			break;
	}

	pos = sprintf(buffer, "(1-6) %s", toolname);
	buffer[pos] = '\0';
	drawString(&tex, &font, buffer, 8, HEIGHT - MENU_HEIGHT + 38, (pixel_t){255, 0, 0, 255});
}

void renderMouse()
{
	char buffer[64];
	int pos = sprintf(buffer, "MOUSE: (%d,%d)", xmouse, ymouse);
	buffer[pos] = '\0';
	drawString(&tex, &font, buffer, 8, HEIGHT - MENU_HEIGHT + 8, (pixel_t){0, 64, 64, 255});

	drawLine(&tex, (xy_t){xmouse - 5, ymouse}, (xy_t){xmouse + 5, ymouse}, (pixel_t){255, 255, 0, 255});
	drawLine(&tex, (xy_t){xmouse, ymouse - 5}, (xy_t){xmouse, ymouse + 5}, (pixel_t){255, 255, 0, 255});

	if(vertselected != -1 && toolselected == EDGE_ADD_TOOL){
		xy_t mouse = {(v_t)xmouse, (v_t)ymouse};
		drawLine(&tex, mouse, vertices[vertselected], (pixel_t){0, 128, 0, 255});
	}
}

void renderMap()
{
	unsigned int i;
	for(i = 0; i < nsectors; i++){
		sector_t sect = sectors[i];
		unsigned int j;
		for(j = 0; j < sect.nvertices; j++){
			xy_t v1 = sect.vertices[j];
			xy_t v2;
			if(j > 0){
				v2 = sect.vertices[j - 1];
			}else{
				v2 = sect.vertices[sect.nvertices - 1];
			}

			drawLine(&tex, v1, v2, (pixel_t){0, 128, 128, 255});
		}
	}

	for(i = 0; i < nedges; i++){
		edge_t edge = edges[i];
		xy_t v1 = vertices[edge.vertex1];
		xy_t v2 = vertices[edge.vertex2];
		if(edge.type == WALL){
			drawLine(&tex, v1, v2, (pixel_t){128, 0, 128, 255});
		}else if(edge.type == PORTAL){
			drawLine(&tex, v1, v2, (pixel_t){64, 64, 255, 255});
		}
	}

	for(i = 0; i < nvertices; i++){
		xy_t v = vertices[i];
		if(v.x >= 0 && v.x < WIDTH && v.y >= 0 && v.y < HEIGHT - MENU_HEIGHT){
			drawCircle(&tex, v, 2, (pixel_t){255, 255, 0, 255});
		}
	}
}

void render()
{	
	glClear(GL_COLOR_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.pixels);

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

	unsigned int i;
	for(i = 0; i < tex.width * tex.height; i++){
		tex.pixels[i].r = tex.pixels[i].g = tex.pixels[i].b = tex.pixels[i].a = 0;
	}
}

void handleMouseClick()
{
	switch(toolselected){
		case VERTEX_TOOL:
			vertices = (xy_t*)realloc(vertices, ++nvertices * sizeof(*vertices));
			vertices[nvertices - 1] = (xy_t){(double)xmouse, (double)ymouse};
			break;
		case EDGE_ADD_TOOL:
			{
				int i;
				bool gotedge = false;
				for(i = 0; i < nvertices; i++){
					xy_t v = vertices[i];
					double dx = v.x - xmouse;
					double dy = v.y - ymouse;
					if(sqrt(dx * dx + dy * dy) < snapsize){
						if(vertselected == -1){
							vertselected = i;
						}else{
							if(vertselected == i){
								break;
							}
							edges = (edge_t*)realloc(edges, ++nedges * sizeof(*edges));
							edges[nedges - 1] = (edge_t){(unsigned int)i, (unsigned int)vertselected, edgetypeselected};
							vertselected = -1;
						}
						gotedge = true;
						break;
					}
				}
				if(!gotedge){
					vertselected = -1;
				}
			}
			break;
		case EDGE_CHANGE_TOOL:
			{
				int i;
				for(i = 0; i < nedges; i++){
					if(distanceToSegment((xy_t){(double)xmouse, (double)ymouse}, vertices[edges[i].vertex1], vertices[edges[i].vertex2]) < snapsize){
						edges[i].type = edgetypeselected;
						return;
					}
				}
			}
			break;
		case SECTOR_TOOL:
			if(sectorselected == -1){
			}
			break;
		case MOVEMENT_TOOL:
			if(vertselected == -1){
				int i;
				for(i = 0; i < nvertices; i++){
					xy_t v = vertices[i];
					double dx = v.x - xmouse;
					double dy = v.y - ymouse;
					if(sqrt(dx * dx + dy * dy) < snapsize){
						vertselected = i;
						break;
					}
				}
			}else{
				vertselected = -1;
			}
			break;
		case REMOVAL_TOOL:
			{
				int i;
				for(i = 0; i < nvertices; i++){
					xy_t v = vertices[i];
					double dx = v.x - xmouse;
					double dy = v.y - ymouse;
					if(sqrt(dx * dx + dy * dy) < snapsize){
						deleteVertex(i);
						return;
					}
				}
				for(i = 0; i < nedges; i++){
					if(distanceToSegment((xy_t){(double)xmouse, (double)ymouse}, vertices[edges[i].vertex1], vertices[edges[i].vertex2]) < snapsize){
						deleteEdge(i);
						return;
					}
				}
			}
			break;
	}
}

int main(int argc, char **argv)
{
	initTexture(&tex, WIDTH, HEIGHT);
	initFont(&font, fontwidth, fontheight);
	loadFont(&font, '!', '~' - '!', 8, (bool*)fontdata);

	ccDisplayInitialize();

	ccWindowCreate((ccRect){0, 0, WIDTH, HEIGHT}, "3D - editor", CC_WINDOW_FLAG_NORESIZE);
	ccWindowMouseSetCursor(CC_CURSOR_NONE);

	ccGLContextBind();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	glBindTexture(GL_TEXTURE_2D, 0);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	bool loop = true;
	while(loop){
		while(ccWindowEventPoll()){
			if(ccWindowEventGet().type == CC_EVENT_WINDOW_QUIT){
				loop = false;
			}else if(ccWindowEventGet().type == CC_EVENT_KEY_DOWN){
				switch(ccWindowEventGet().keyCode){
					case CC_KEY_ESCAPE:
						loop = false;
						break;
				}
			}else if(ccWindowEventGet().type == CC_EVENT_KEY_UP){
				switch(ccWindowEventGet().keyCode){
					case CC_KEY_S:
						snaptogrid = !snaptogrid;
						break;
					case CC_KEY_1:
						if(vertselected == -1){
							toolselected = MOVEMENT_TOOL;
						}
						break;
					case CC_KEY_2:
						if(vertselected == -1){
							toolselected = REMOVAL_TOOL;
						}
						break;
					case CC_KEY_3:
						if(vertselected == -1){
							toolselected = VERTEX_TOOL;
						}
						break;
					case CC_KEY_4:
						if(vertselected == -1){
							toolselected = EDGE_ADD_TOOL;
						}
						break;
					case CC_KEY_5:
						if(vertselected == -1){
							toolselected = EDGE_CHANGE_TOOL;
						}
						break;
					case CC_KEY_6:
						if(vertselected == -1){
							toolselected = SECTOR_TOOL;
						}
					case CC_KEY_W:
						edgetypeselected = WALL;
						break;
					case CC_KEY_P:
						edgetypeselected = PORTAL;
						break;
					case CC_KEY_9:
						if(gridsize > 0){
							gridsize--;
						}
						break;
					case CC_KEY_0:
						gridsize++;
						break;
				}
			}else if(ccWindowEventGet().type == CC_EVENT_MOUSE_UP){
				handleMouseClick();
			}
		}

		xmouse = ccWindowGetMouse().x;
		ymouse = ccWindowGetMouse().y;

		if(snaptogrid){
			xmouse = round(xmouse / gridsize) * gridsize;
			ymouse = round(ymouse / gridsize) * gridsize;
		}
		
		if(vertselected != -1 && toolselected == MOVEMENT_TOOL){
			vertices[vertselected].x = xmouse;
			vertices[vertselected].y = ymouse;
		}

		renderBackground();
		renderMap();
		renderMenu();
		renderMouse();

		render();
		ccGLBuffersSwap();

		ccTimeDelay(6);
	}

	ccFree();

	return 0;
}
