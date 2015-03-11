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
#include "l_level.h"

#define WIDTH 800
#define HEIGHT 600

#define MENU_HEIGHT 64

#define COLOR_RED (pixel_t){255, 0, 0, 255}
#define COLOR_GREEN (pixel_t){0, 255, 0, 255}
#define COLOR_BLUE (pixel_t){0, 0, 255, 255}
#define COLOR_BLUEGRAY (pixel_t){0, 128, 128, 255}
#define COLOR_YELLOW (pixel_t){255, 255, 0, 255}

typedef enum {VERTEX_MOVE_TOOL, SECTOR_ADD_TOOL, EDGE_ADD_TOOL, EDGE_CHANGE_TOOL, EDGE_CONNECT_TOOL} tool_t;

GLuint texture;
texture_t tex;
font_t font;

bool snaptogrid = false;
sector_t *sectorselected = NULL;
edge_t *edgeselected = NULL;
tool_t toolselected = SECTOR_ADD_TOOL;
edgetype_t edgetypeselected = WALL;
int vertselected = -1;

char *saveto = NULL;

int gridsize = 25;
int snapsize = 10;

xy_t mouse;

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

#if 0
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
	if(index >= nedges){
		return;
	}

	memmove(vertices + index, vertices + index + 1, (nedges - index - 1) * sizeof(vertices[index]));
	vertices = (xy_t*)realloc(vertices, --nedges * sizeof(*vertices));

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
#endif

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
	drawString(&tex, &font, buffer, 8, HEIGHT - MENU_HEIGHT + 8, COLOR_BLUEGRAY);

	pos = sprintf(buffer, "(G) SNAP TO GRID: %s", snaptogrid ? "ON" : "OFF");
	buffer[pos] = '\0';
	drawString(&tex, &font, buffer, 8, HEIGHT - MENU_HEIGHT + 18, COLOR_BLUEGRAY);

	if(saveto != NULL){
		pos = sprintf(buffer, "(S) SAVE TO FILE: \"%s\"", saveto);
		buffer[pos] = '\0';
		drawString(&tex, &font, buffer, 8, HEIGHT - MENU_HEIGHT + 28, COLOR_BLUEGRAY);
	}

	char toolname[64];
	switch(toolselected){
		case SECTOR_ADD_TOOL:
			strcpy(toolname, "ADD SECTOR");
			break;
		case EDGE_ADD_TOOL:
			strcpy(toolname, "CHANGE SECTOR");
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
		case EDGE_CONNECT_TOOL:
			if(edgeselected != NULL){
				sprintf(toolname, "CONNECT EDGES - SECTOR SELECTED %p", edgeselected->sector);
			}else{
				strcpy(toolname, "CONNECT EDGES - SELECT EDGE");
			}
			break;
		case VERTEX_MOVE_TOOL:
			strcpy(toolname, "MOVE VERTEX");
			break;
		default:
			strcpy(toolname, "ERROR");
			break;
	}

	pos = sprintf(buffer, "(1-5) %s", toolname);
	buffer[pos] = '\0';
	drawString(&tex, &font, buffer, 8, HEIGHT - MENU_HEIGHT + 38, COLOR_RED);
}

void renderMouse()
{
	char buffer[64];
	int pos = sprintf(buffer, "(%.f,%.f)", mouse.x, mouse.y);
	buffer[pos] = '\0';
	drawString(&tex, &font, buffer, 8, 8, COLOR_YELLOW);

	drawLine(&tex, (xy_t){mouse.x - 5, mouse.y}, (xy_t){mouse.x + 5, mouse.y}, COLOR_YELLOW);
	drawLine(&tex, (xy_t){mouse.x, mouse.y - 5}, (xy_t){mouse.x, mouse.y + 5}, COLOR_YELLOW);
}

void renderMap()
{
	pixel_t color;
	if(sectorselected != NULL){
		// Draw edge add lines
		if(toolselected == EDGE_ADD_TOOL){
			if(edgetypeselected == WALL){
				color = COLOR_YELLOW;
			}else{
				color = COLOR_BLUE;
			}
			drawLine(&tex, sectorselected->vertices[0], mouse, sectorselected->edges[0].type == WALL ? COLOR_YELLOW : COLOR_BLUE);
			drawLine(&tex, sectorselected->vertices[sectorselected->nedges - 1], mouse, color);
		}

		// Draw dragged vertex
		if(vertselected != -1){
			drawLine(&tex, sectorselected->vertices[vertselected == sectorselected->nedges - 1 ? 0 : vertselected + 1], mouse, COLOR_GREEN);
			drawLine(&tex, sectorselected->vertices[vertselected == 0 ? sectorselected->nedges - 1 : vertselected - 1], mouse, COLOR_GREEN);
			drawCircle(&tex, mouse, 2, COLOR_YELLOW);
		}
	}

	sector_t *sect = getFirstSector();
	while(sect != NULL){
		// Draw edges
		unsigned int i, start = 0;
		if(sectorselected == sect && toolselected == EDGE_ADD_TOOL){
			start = 1;
		}
		for(i = start; i < sect->nedges; i++){
			edge_t edge = sect->edges[i];
			if(edge.type == WALL){
				color = COLOR_YELLOW;
			}else{
				color = COLOR_BLUE;
			}
			drawLine(&tex, sect->vertices[edge.vertex1], sect->vertices[edge.vertex2], color);
		}
		// Draw vertices
		for(i = 0; i < sect->nedges; i++){
			drawCircle(&tex, sect->vertices[i], 2, COLOR_RED);
		}
		sect = getNextSector(sect);
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
		case SECTOR_ADD_TOOL:
			sectorselected = createSector(mouse);
			toolselected = EDGE_ADD_TOOL;
			break;
		case EDGE_ADD_TOOL:
			createEdge(sectorselected, mouse, edgetypeselected);
			break;
		case EDGE_CHANGE_TOOL:
			{
				sector_t *sect = getFirstSector();
				while(sect != NULL){
					unsigned int i;
					for(i = 0; i < sect->nedges; i++){
						edge_t *edge = sect->edges + i;
						if(distanceToSegment(mouse, sect->vertices[edge->vertex1], sect->vertices[edge->vertex2]) < snapsize){
							edge->type = edgetypeselected;
							if(edge->type == WALL){
								edge->sector = NULL;
							}
							break;
						}
					}
					sect = getNextSector(sect);
				}
			}
			break;
		case EDGE_CONNECT_TOOL:
			{
				sector_t *sect = getFirstSector();
				while(sect != NULL){
					unsigned int i;
					for(i = 0; i < sect->nedges; i++){
						edge_t *edge = sect->edges + i;
						if(distanceToSegment(mouse, sect->vertices[edge->vertex1], sect->vertices[edge->vertex2]) < snapsize){
							if(edgeselected == NULL){
								edgeselected = edge;
								return;
							}else	if(edgeselected != edge && edgeselected->sector != edge->sector){
								edge->neighbor = edgeselected;
								edgeselected->neighbor = edge;
								edge->type = PORTAL;
								edgeselected->type = PORTAL;
								edgeselected = NULL;
								return;
							}
						}
					}
					sect = getNextSector(sect);
				}
			}
			break;
		case VERTEX_MOVE_TOOL:
			if(vertselected == -1){
				unsigned int i;
				for(i = 0; i < sectorselected->nedges; i++){
					xy_t v = sectorselected->vertices[i];
					int dx = mouse.x - v.x;
					int dy = mouse.y - v.y;
					if(sqrt(dx * dx + dy * dy) < snapsize){
						vertselected = i;
						break;
					}
				}
			}else{
				sectorselected->vertices[vertselected] = mouse;
				vertselected = -1;
			}
			break;
	}
}

void save()
{
	if(saveto == NULL){
		printf("No save file supplied.\n");
		exit(1);
	}

	FILE *fp = fopen(saveto, "wt");
	if(!fp){
		printf("Couldn't open file for writing: %s\n", saveto);
		exit(1);
	}

	unsigned int totalsectors = 0;
	sector_t *sect = getFirstSector();
	while(sect != NULL){
		fprintf(fp, "s %u\n", totalsectors);
		unsigned int i;
		for(i = 0; i < sect->nedges; i++){
			edge_t edge = sect->edges[i];
			xy_t v = sect->vertices[edge.vertex1];
			fprintf(fp, "e %d (%.lf,%.lf)", edge.type, v.x, v.y);

			if(edge.neighbor != NULL){
				unsigned int totalsectors2 = 0;
				sector_t *sect2 = getFirstSector();
				while(sect2 != NULL){
					if(edge.neighbor->sector == sect2){
						unsigned int j;
						for(j = 0; j < sect2->nedges && sect2->edges + j != edge.neighbor; j++);

						fprintf(fp, " %u %u", totalsectors2, j);
						break;
					}
					totalsectors2++;
					sect2 = getNextSector(sect2);
				}
			}
			fprintf(fp, "\n");
		}
		fprintf(fp, "\n");
		sect = getNextSector(sect);
		totalsectors++;
	}

	fclose(fp);

	printf("Succesfully written level to file \"%s\"!\n", saveto);
}

int main(int argc, char **argv)
{
	sectorInitialize();

	initTexture(&tex, WIDTH, HEIGHT);
	initFont(&font, fontwidth, fontheight);
	loadFont(&font, '!', '~' - '!', 8, (bool*)fontdata);

	if(argc > 0){
		saveto = (char*)malloc(strlen(argv[1]) + 1);
		strcpy(saveto, argv[1]);
	}

	if(argc > 1){
		loadLevel(argv[2]);
	}

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
					case CC_KEY_G:
						snaptogrid = !snaptogrid;
						break;
					case CC_KEY_1:
						toolselected = SECTOR_ADD_TOOL;
						break;
					case CC_KEY_2:
						toolselected = EDGE_ADD_TOOL;
						break;
					case CC_KEY_3:
						toolselected = EDGE_CHANGE_TOOL;
						break;
					case CC_KEY_4:
						toolselected = EDGE_CONNECT_TOOL;
						break;
					case CC_KEY_5:
						toolselected = VERTEX_MOVE_TOOL;
						break;
					case CC_KEY_S:
						save();
						break;
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

		mouse.x = ccWindowGetMouse().x;
		mouse.y = ccWindowGetMouse().y;

		if(snaptogrid){
			mouse.x = round(mouse.x / gridsize) * gridsize;
			mouse.y = round(mouse.y / gridsize) * gridsize;
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
