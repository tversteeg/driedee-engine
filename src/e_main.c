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
#include "l_colors.h"
#include "l_vector.h"
#include "l_sector.h"
#include "l_level.h"
#include "l_render.h"
#include "l_png.h"
#include "l_gui.h"

#define EDITOR_WIDTH 600
#define PREVIEW_WIDTH 600
#define HEIGHT 600
#define MENU_HEIGHT 50

#define CAM_SPEED 1

#define NONE_SELECTED -1

typedef enum {NO_TOOL, VERTEX_MOVE_TOOL, SECTOR_ADD_TOOL, EDGE_ADD_TOOL, SECTOR_SELECT_TOOL, SECTOR_DELETE_TOOL, EDGE_CHANGE_TOOL, EDGE_CONNECT_TOOL} tool_t;

GLuint texture;
texture_t previewtex, editortex, tex, wall;
font_t font;

bool snaptogrid = false;
sector_t *sectorselected = NULL;
edge_t *edgeselected = NULL;
tool_t toolselected = NO_TOOL;
edgetype_t edgetypeselected = WALL;
int vertselected = NONE_SELECTED;

bool redrawpreview = true;
bool redraweditor = true;
int redrawupdate = 0;

char *saveto = NULL;

int gridsize = 24;
int snapsize = 10;

xy_t mapoffset;
xy_t mouse;
camera_t cam;
sector_t *camsector = NULL;

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
		
	fprintf(fp, "// s (Sector:) id\n");
	fprintf(fp, "// e (Edge:) type, vertex position\n");
	fprintf(fp, "// dw (Edge wall data:) id, bottom, top, uv scale, texture id\n");
	fprintf(fp, "// p (Portal:) sector1, edge1, sector2, edge2\n\n");

	unsigned int totalsectors = 0;
	sector_t *sect = getFirstSector();
	while(sect != NULL){
		fprintf(fp, "s %u\n", totalsectors);

		unsigned int i;
		for(i = 0; i < sect->nedges; i++){
			edge_t edge = sect->edges[i];
			xy_t v = sect->vertices[edge.vertex1];
			fprintf(fp, "e %d (%.lf,%.lf)\n", edge.type, v.x, v.y);
		}

		for(i = 0; i < sect->nedges; i++){
			edge_t edge = sect->edges[i];
			if(edge.type == WALL){
				fprintf(fp, "dw %d %.lf %.lf %.lf %d\n", i, edge.wallbot, edge.walltop, edge.uvdiv, edge.texture);
			}
		}
		fprintf(fp, "\n");
		sect = getNextSector(sect);
		totalsectors++;
	}

	totalsectors = 0;
	sect = getFirstSector();
	while(sect != NULL){
		unsigned int i;
		for(i = 0; i < sect->nedges; i++){
			edge_t edge = sect->edges[i];
			if(edge.type != PORTAL || edge.neighbor == NULL){
				continue;
			}

			sector_t *neighbor = edge.neighbor->sector;
			int sector2 = getIndexSector(neighbor);
			if(sector2 == -1){
				exit(1);
			}
			fprintf(fp, "p %u %u %u %u\n", totalsectors, i, sector2, (unsigned int)(edge.neighbor - neighbor->edges));
		}
		sect = getNextSector(sect);
		totalsectors++;
	}

	fclose(fp);

	printf("Succesfully written level to file \"%s\"!\n", saveto);
}

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

void renderBackground()
{
	drawGrid(&editortex, 0, 0, EDITOR_WIDTH, HEIGHT - MENU_HEIGHT, gridsize, gridsize, (pixel_t){32, 32, 32, 255});
	
	drawLine(&editortex, (xy_t){0, HEIGHT - MENU_HEIGHT}, (xy_t){EDITOR_WIDTH, HEIGHT - MENU_HEIGHT}, (pixel_t){255, 255, 0, 255});

	drawRect(&editortex, (xy_t){0, HEIGHT - MENU_HEIGHT + 1}, EDITOR_WIDTH, MENU_HEIGHT, (pixel_t){16, 16, 16, 255});
}

void renderMenu()
{
	renderGui(&editortex);
}

void renderMouse()
{
	char buffer[64];
	int pos = sprintf(buffer, "(%.f,%.f)", mouse.x, mouse.y);
	buffer[pos] = '\0';
	drawString(&editortex, &font, buffer, 8, 8, COLOR_YELLOW);

	drawLine(&editortex, (xy_t){mouse.x - 5, mouse.y}, (xy_t){mouse.x + 5, mouse.y}, COLOR_YELLOW);
	drawLine(&editortex, (xy_t){mouse.x, mouse.y - 5}, (xy_t){mouse.x, mouse.y + 5}, COLOR_YELLOW);
}

void renderMap()
{
	pixel_t color;
	sector_t *sect = getFirstSector();
	while(sect != NULL){
		// Draw edges
		unsigned int i, start = 0;
		if(sectorselected == sect && toolselected == EDGE_ADD_TOOL){
			start = 1;
		}
		for(i = start; i < sect->nedges; i++){
			edge_t edge = sect->edges[i];
			if(sectorselected != sect){
				if(edge.type == WALL){
					color = COLOR_YELLOW;
				}else{
					if(edge.neighbor != NULL){
						color = COLOR_BLUE;
					}else{
						color = COLOR_CYAN;
					}
				}
			}else{
				if(edge.type == WALL){
					color = COLOR_MAGENTA;
				}else{
					if(edge.neighbor != NULL){
						color = COLOR_VIOLET;
					}else{
						color = COLOR_ROSE;
					}
				}
			}
			xy_t vert1 = sect->vertices[edge.vertex1];
			vert1.x += mapoffset.x;
			vert1.y += mapoffset.y;
			xy_t vert2 = sect->vertices[edge.vertex2];
			vert2.x += mapoffset.x;
			vert2.y += mapoffset.y;
			drawLine(&editortex, vert1, vert2, color);
		}
		// Draw vertices
		for(i = 0; i < sect->nedges; i++){
			xy_t vert = sect->vertices[i];
			vert.x += mapoffset.x;
			vert.y += mapoffset.y;
			drawCircle(&editortex, vert, 2, COLOR_RED);
		}
		sect = getNextSector(sect);
	}

	if(sectorselected != NULL){
		// Draw edge add lines
		if(toolselected == EDGE_ADD_TOOL){
			if(edgetypeselected == WALL){
				color = COLOR_YELLOW;
			}else{
				color = COLOR_BLUE;
			}
			xy_t vert = sectorselected->vertices[0];
			vert.x += mapoffset.x;
			vert.y += mapoffset.y;
			drawLine(&editortex, vert, mouse, sectorselected->edges[0].type == WALL ? COLOR_YELLOW : COLOR_BLUE);
			
			vert = sectorselected->vertices[sectorselected->nedges - 1];
			vert.x += mapoffset.x;
			vert.y += mapoffset.y;
			drawLine(&editortex, vert, mouse, color);
		}

		// Draw dragged vertex
		if(vertselected != NONE_SELECTED){
			xy_t vert = sectorselected->vertices[vertselected == sectorselected->nedges - 1 ? 0 : vertselected + 1];
			vert.x += mapoffset.x;
			vert.y += mapoffset.y;
			drawLine(&editortex, vert, mouse, COLOR_GREEN);
			
			vert = sectorselected->vertices[vertselected == 0 ? sectorselected->nedges - 1 : vertselected - 1];
			vert.x += mapoffset.x;
			vert.y += mapoffset.y;
			drawLine(&editortex, vert, mouse, COLOR_GREEN);
			drawCircle(&editortex, mouse, 2, COLOR_YELLOW);
		}
	}

	if(camsector != NULL){
		xy_t p1 = {cam.pos.x + mapoffset.x, cam.pos.z + mapoffset.y};
		xy_t p2 = {cam.pos.x + mapoffset.x + sin(cam.angle + M_PI) * 10, cam.pos.z + mapoffset.y + cos(cam.angle + M_PI) * 10};

		drawCircle(&editortex, p1, 3, COLOR_WHITE);
		drawLine(&editortex, p1, p2, COLOR_WHITE);
	}
}

void render()
{	
	if(camsector != NULL && redrawpreview){
		renderFromSector(&previewtex, &wall, camsector, &cam);
		drawTexture(&tex, &previewtex, EDITOR_WIDTH, 0);
		clearTexture(&previewtex, COLOR_NONE);
		redrawpreview = false;
	}

	if(redraweditor){
		renderBackground();
		renderMap();
		renderMenu();
		renderMouse();

		drawTexture(&tex, &editortex, 0, 0);
		clearTexture(&editortex, COLOR_BLACK);
		redraweditor = false;
	}

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
}

void handleMouseClick()
{
	xy_t mousemap = {mouse.x - mapoffset.x, mouse.y - mapoffset.y};
	
	switch(toolselected){
		case SECTOR_ADD_TOOL:
			{
				edge_t edge;
				edge.type = edgetypeselected;
				if(edge.type == WALL){
					edge.wallbot = 0;
					edge.walltop = 10;
				}
				sectorselected = createSector(mousemap, &edge);
				toolselected = EDGE_ADD_TOOL;
			}
			break;
		case SECTOR_SELECT_TOOL:
			{
				sector_t *sect = getFirstSector();
				while(sect != NULL){
					if(sect != sectorselected){
						unsigned int i;
						for(i = 0; i < sect->nedges; i++){
							edge_t *edge = sect->edges + i;
							if(distanceToSegment(mousemap, sect->vertices[edge->vertex1], sect->vertices[edge->vertex2]) < snapsize){
								sectorselected = sect;
								return;
							}
						}
					}
					sect = getNextSector(sect);
				}
			}
			break;
		case SECTOR_DELETE_TOOL:
			{
				sector_t *sect = getFirstSector();
				while(sect != NULL){
					unsigned int i;
					for(i = 0; i < sect->nedges; i++){
						edge_t *edge = sect->edges + i;
						if(distanceToSegment(mousemap, sect->vertices[edge->vertex1], sect->vertices[edge->vertex2]) < snapsize){
							if(sectorselected == sect){
								sectorselected = NULL;
							}
							if(camsector == sect){
								camsector = NULL;
							}
							deleteSector(sect);
							return;
						}
					}
					sect = getNextSector(sect);
				}
			}
			break;
		case EDGE_ADD_TOOL:
			{
				edge_t edge;
				edge.type = edgetypeselected;
				if(edge.type == WALL){
					edge.wallbot = 0;
					edge.walltop = 10;
				}
				createEdge(sectorselected, mousemap, &edge);

				if(camsector == NULL && sectorselected->nedges > 2){
					cam.pos.x = (sectorselected->vertices[0].x + sectorselected->vertices[1].x + sectorselected->vertices[2].x) / 3;
					cam.pos.z = (sectorselected->vertices[0].y + sectorselected->vertices[1].y + sectorselected->vertices[2].y) / 3;
					camsector = sectorselected;
				}
			}
			break;
		case EDGE_CHANGE_TOOL:
			{
				sector_t *sect = getFirstSector();
				while(sect != NULL){
					unsigned int i;
					for(i = 0; i < sect->nedges; i++){
						edge_t *edge = sect->edges + i;
						if(distanceToSegment(mousemap, sect->vertices[edge->vertex1], sect->vertices[edge->vertex2]) < snapsize){
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
						if(distanceToSegment(mousemap, sect->vertices[edge->vertex1], sect->vertices[edge->vertex2]) < snapsize){
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
			if(vertselected == NONE_SELECTED){
				unsigned int i;
				for(i = 0; i < sectorselected->nedges; i++){
					xy_t v = sectorselected->vertices[i];
					int dx = mousemap.x - v.x;
					int dy = mousemap.y - v.y;
					if(sqrt(dx * dx + dy * dy) < snapsize){
						vertselected = i;
						break;
					}
				}
			}else{
				sectorselected->vertices[vertselected] = mousemap;
				vertselected = NONE_SELECTED;
			}
			break;
		default:
			break;
	}
}

void moveCam(bool up, bool down, bool left, bool right)
{
	xyz_t oldcam = cam.pos;

	if(up){
		cam.pos.x += cos(cam.angle + M_PI / 2) * CAM_SPEED;
		cam.pos.z -= sin(cam.angle + M_PI / 2) * CAM_SPEED;
	}
	if(down){
		cam.pos.x += cos(cam.angle - M_PI / 2) * CAM_SPEED;
		cam.pos.z -= sin(cam.angle - M_PI / 2) * CAM_SPEED;
	}
	if(left){
		cam.angle -= 0.035f;
	}
	if(right){
		cam.angle += 0.035f;
	}

	unsigned int i;
	for(i = 0; i < camsector->nedges; i++){
		edge_t *edge = camsector->edges + i;
		if(edge->type != PORTAL || edge->neighbor == NULL){
			continue;
		}

		xy_t p1 = {cam.pos.x, cam.pos.z};
		xy_t p2 = {oldcam.x, oldcam.z};
		xy_t edge1 = camsector->vertices[edge->vertex1];
		xy_t edge2 = camsector->vertices[edge->vertex2];
		xy_t result;
		if(segmentSegmentIntersect(p1, p2, edge1, edge2, &result)){
			camsector = edge->neighbor->sector;
			break;
		}
	}

	redrawpreview = true;
	redrawupdate++;
	if(redrawupdate >= 10){
		redrawupdate = 0;
		redraweditor = true;
	}
}

void buttonEventDown(button_t *button)
{
}

int main(int argc, char **argv)
{
	sectorInitialize();

	initTexture(&tex, EDITOR_WIDTH + PREVIEW_WIDTH, HEIGHT);
	initTexture(&editortex, EDITOR_WIDTH, HEIGHT);
	initTexture(&previewtex, PREVIEW_WIDTH, HEIGHT);

	initFont(&font, fontwidth, fontheight);
	loadFont(&font, '!', '~' - '!', 8, (bool*)fontdata);

	mapoffset = XY_ZERO;

	if(argc >= 2){
		saveto = (char*)malloc(strlen(argv[1]) + 1);
		strcpy(saveto, argv[1]);
	}

	if(argc >= 3 && loadLevel(argv[2])){
		cam.pos.x = getFirstSector()->vertices[0].x + 10;
		cam.pos.z = getFirstSector()->vertices[0].y;
		camsector = getFirstSector();
	}

	bindFont(&font, "default");
	loadGuiFromFile("gui.cfg");
	bindButtonEvent("Save", buttonEventDown, EVENT_ON_MOUSE_DOWN);

	unsigned int width, height;
	getSizePng("wall1.png", &width, &height);
	initTexture(&wall, width, height);
	loadPng(&wall, "wall1.png");

	cam.pos.y = cam.angle = 0;
	cam.znear = 1;
	cam.zfar = 200;
	calculateViewport(&cam, (xy_t){1, 1});

	ccDisplayInitialize();

	ccWindowCreate((ccRect){0, 0, (int)tex.width, (int)tex.height}, "3D - editor", CC_WINDOW_FLAG_NORESIZE);
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
	bool leftpressed, rightpressed, uppressed, downpressed, rightmousepressed, leftmousepressed;
	rightmousepressed = leftmousepressed = leftpressed = rightpressed = uppressed = downpressed = false;
	while(loop){
		while(ccWindowEventPoll()){
			if(ccWindowEventGet().type == CC_EVENT_WINDOW_QUIT){
				loop = false;
			}else if(ccWindowEventGet().type == CC_EVENT_KEY_DOWN){
				switch(ccWindowEventGet().keyCode){
					case CC_KEY_ESCAPE:
						loop = false;
						break;
					case CC_KEY_UP:
						uppressed = true;
						break;
					case CC_KEY_DOWN:
						downpressed = true;
						break;
					case CC_KEY_LEFT:
						leftpressed = true;
						break;
					case CC_KEY_RIGHT:
						rightpressed = true;
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
						toolselected = SECTOR_SELECT_TOOL;
						break;
					case CC_KEY_3:
						toolselected = EDGE_ADD_TOOL;
						break;
					case CC_KEY_4:
						toolselected = EDGE_CHANGE_TOOL;
						break;
					case CC_KEY_5:
						toolselected = EDGE_CONNECT_TOOL;
						break;
					case CC_KEY_6:
						toolselected = VERTEX_MOVE_TOOL;
						break;
					case CC_KEY_7:
						toolselected = SECTOR_DELETE_TOOL;
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
					case CC_KEY_UP:
						uppressed = false;
						break;
					case CC_KEY_DOWN:
						downpressed = false;
						break;
					case CC_KEY_LEFT:
						leftpressed = false;
						break;
					case CC_KEY_RIGHT:
						rightpressed = false;
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
				redraweditor = true;
			}else if(ccWindowEventGet().type == CC_EVENT_MOUSE_UP){
				if(ccWindowEventGet().mouseButton == CC_MOUSE_BUTTON_LEFT && ccWindowGetMouse().x <= EDITOR_WIDTH){
					handleMouseClick();
					redrawpreview = true;
					redraweditor = true;
				}
				if(ccWindowEventGet().mouseButton == CC_MOUSE_BUTTON_RIGHT){
					rightmousepressed = false;
				}
				if(ccWindowEventGet().mouseButton == CC_MOUSE_BUTTON_LEFT){
					leftmousepressed = false;
				}
			}else if(ccWindowEventGet().type == CC_EVENT_MOUSE_DOWN){
				if(ccWindowEventGet().mouseButton == CC_MOUSE_BUTTON_RIGHT){
					rightmousepressed = true;
				}
				if(ccWindowEventGet().mouseButton == CC_MOUSE_BUTTON_LEFT){
					leftmousepressed = true;
				}
			}
		}

		if(camsector != NULL && (uppressed || downpressed || leftpressed || rightpressed)){
			moveCam(uppressed, downpressed, leftpressed, rightpressed);
		}

		xy_t oldmouse = mouse;

		mouse.x = ccWindowGetMouse().x;
		mouse.y = ccWindowGetMouse().y;

		updateGui((int)mouse.x, (int)mouse.y, leftmousepressed);

		if(snaptogrid){
			mouse.x = round(mouse.x / gridsize) * gridsize;
			mouse.y = round(mouse.y / gridsize) * gridsize;
		}

		if(!vectorIsEqual(oldmouse, mouse)){
			redraweditor = true;
			if(rightmousepressed){
				mapoffset.x += mouse.x - oldmouse.x;
				mapoffset.y += mouse.y - oldmouse.y;
			}
		}

		render();
		ccGLBuffersSwap();

		ccTimeDelay(5);
	}

	ccFree();

	return 0;
}
