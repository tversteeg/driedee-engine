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

#define WIDTH 800
#define HEIGHT 600

#define MENU_HEIGHT 64

#define MOVEMENT_TOOL 1
#define VERTEX_TOOL 2
#define EDGE_TOOL 3
#define REMOVAL_TOOL 4

typedef struct {
	xyz_t start;
	double angle, slope;
} plane_t;

typedef enum {PORTAL, WALL} edgetype_t;

typedef struct {
	unsigned int vertex1, vertex2;
	edgetype_t type;
	union {
		unsigned int neighbor;
	};
} edge_t;

typedef struct {
	xy_t *vertex;
	edge_t *walls;
	plane_t floor, ceil;
	unsigned int npoints, nneighbors, *neighbors, nvisited, *visited;
} sector_t;

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
unsigned int toolselected = 3;
edgetype_t edgetypeselected = WALL;
int vertselected = -1;
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

void load(char *map)
{
	FILE *fp;
	char *line, *ptr;
	int index, index2, scanlen;
	size_t len;
	ssize_t read;
	xy_t vert;
	sector_t *sect;
	line = NULL;
	len = 0;
	fp = fopen(map, "rt");
	if(!fp) {
		printf("Couldn't open: %s\n", map);
		return;
	}
	/* TODO: replace GNU readline with a cross platform solution */
	while((read = getline(&line, &len, fp)) != -1) {
		switch(line[0]){
			case 'v':
				ptr = line;
				sscanf(ptr, "%*s %lf%n", &vert.y, &scanlen);
				while(sscanf(ptr += scanlen, "%lf%n", &vert.x, &scanlen) == 1){
					vertices = (xy_t*)realloc(vertices, ++nvertices * sizeof(*vertices));
					vertices[nvertices - 1] = vert;
				}
				break;
			case 's':
				sectors = (sector_t*)realloc(sectors, ++nsectors * sizeof(*sectors));
				sect = sectors + nsectors - 1;
				ptr = line;
				sect->npoints = 0;
				sect->vertex = NULL;
				sect->visited = NULL;
				sect->nvisited = 0;
				sect->nneighbors = 0;
				sect->neighbors = NULL;
				sscanf(ptr, "%*s %lf %lf %lf %u %lf %lf %lf %u%n", &sect->floor.start.z, &sect->floor.slope, &sect->floor.angle, &index,
						&sect->ceil.start.z, &sect->ceil.slope, &sect->ceil.angle, &index2, &scanlen);
				sect->floor.start.x = vertices[index].x;
				sect->floor.start.y = vertices[index].y;
				sect->ceil.start.x = vertices[index2].x;
				sect->ceil.start.y = vertices[index2].y;
				while(sscanf(ptr += scanlen, "%d%n", &index, &scanlen) == 1){
					sect->vertex = (xy_t*)realloc(sect->vertex, ++sect->npoints * sizeof(*sect->vertex));
					sect->vertex[sect->npoints - 1] = vertices[index];
				}
				sscanf(ptr += scanlen, "%*c%n", &scanlen);
				while(sscanf(ptr += scanlen, "%u%n", &index, &scanlen) == 1){
					sect->neighbors = (unsigned int*)realloc(sect->neighbors, ++sect->nneighbors * sizeof(*sect->neighbors));
					sect->neighbors[sect->nneighbors - 1] = index;
				}
				break;
		}
	}
	fclose(fp);
	free(line);
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
	drawGrid(&tex, 0, 0, WIDTH, HEIGHT - MENU_HEIGHT, gridsize, gridsize, (pixel_t){32, 32, 32, 1});
}

void renderMenu()
{
	//hline(HEIGHT - MENU_HEIGHT, 0, WIDTH, 255, 255, 0, 1);

	/*
	int i;
	for(i = HEIGHT - MENU_HEIGHT + 1; i < HEIGHT; i++){
		hline(i, 0, WIDTH, 16, 16, 16, 1);
	}
	*/

	char buffer[64];
	int pos = sprintf(buffer, "(9&0) GRID SIZE: (%dx%d)", gridsize, gridsize);
	buffer[pos] = '\0';
	drawString(&tex, &font, buffer, 8, HEIGHT - MENU_HEIGHT + 18, (pixel_t){0, 128, 128, 1});

	pos = sprintf(buffer, "(S) SNAP TO GRID: %s", snaptogrid ? "ON" : "OFF");
	buffer[pos] = '\0';
	drawString(&tex, &font, buffer, 8, HEIGHT - MENU_HEIGHT + 28, (pixel_t){0, 128, 128, 1});

	char toolname[64];
	switch(toolselected){
		case VERTEX_TOOL:
			strcpy(toolname, "VERTEX");
			break;
		case EDGE_TOOL:
			switch(edgetypeselected){
				case WALL:
					strcpy(toolname, "EDGE - (W&P) WALL");
					break;
				case PORTAL:
					strcpy(toolname, "EDGE - (W&P) PORTAL");
					break;
				default:
					strcpy(toolname, "EDGE - UNDEFINED");
					break;
			}
			break;
		case MOVEMENT_TOOL:
			strcpy(toolname, "MOVEMENT");
			break;
		case REMOVAL_TOOL:
			strcpy(toolname, "REMOVAL");
			break;
		default:
			strcpy(toolname, "NO");
			break;
	}

	pos = sprintf(buffer, "(1-4) %s TOOL SELECTED", toolname);
	buffer[pos] = '\0';
	drawString(&tex, &font, buffer, 8, HEIGHT - MENU_HEIGHT + 38, (pixel_t){255, 0, 0, 1});
}

void renderMouse()
{
	char buffer[64];
	int pos = sprintf(buffer, "MOUSE: (%d,%d)", xmouse, ymouse);
	buffer[pos] = '\0';
	drawString(&tex, &font, buffer, 8, HEIGHT - MENU_HEIGHT + 8, (pixel_t){0, 64, 64, 1});


	//vline(xmouse, ymouse - 5, ymouse + 5, 255, 255, 0, 1);
	//hline(ymouse, xmouse - 5, xmouse + 5, 255, 255, 0, 1);

	if(vertselected != -1 && toolselected == EDGE_TOOL){
		xy_t mouse = {(double)xmouse, (double)ymouse};
		drawLine(&tex, mouse, vertices[vertselected], (pixel_t){0, 128, 0, 1});
	}
}

void renderMap()
{
	unsigned int i;
	for(i = 0; i < nsectors; i++){
		sector_t sect = sectors[i];
		unsigned int j;
		for(j = 0; j < sect.npoints; j++){
			xy_t v1 = sect.vertex[j];
			xy_t v2;
			if(j > 0){
				v2 = sect.vertex[j - 1];
			}else{
				v2 = sect.vertex[sect.npoints - 1];
			}

			drawLine(&tex, v1, v2, (pixel_t){0, 128, 128, 1});
		}
	}

	for(i = 0; i < nedges; i++){
		edge_t edge = edges[i];
		xy_t v1 = vertices[edge.vertex1];
		xy_t v2 = vertices[edge.vertex2];
		if(edge.type == WALL){
			drawLine(&tex, v1, v2, (pixel_t){128, 0, 128, 1});
		}else if(edge.type == PORTAL){
			drawLine(&tex, v1, v2, (pixel_t){64, 64, 255, 1});
		}
	}

	for(i = 0; i < nvertices; i++){
		xy_t v = vertices[i];
		if(v.x >= 0 && v.x < WIDTH && v.y >= 0 && v.y < HEIGHT - MENU_HEIGHT){
			drawCircle(&tex, v, 2, (pixel_t){255, 255, 0, 1});
		}
	}
}

void render()
{	
	glClear(GL_COLOR_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.pixels);

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
		case EDGE_TOOL:
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
	load(argv[1]);
	
	initTexture(&tex, WIDTH, HEIGHT);
	initFont(&font, fontwidth, fontheight);
	loadFont(&font, '!', '~' - '!', 8, (bool*)&fontdata[0]);

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
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
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
							toolselected = EDGE_TOOL;
						}
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
