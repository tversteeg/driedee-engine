#include "l_render.h"

#include <string.h>

#include "l_colors.h"

typedef struct {
	int32_t minx, maxx, miny, maxy;
}	aabb_t;

// Sector information
// Start of the wall pointer
wallp_t s_fwall[MAX_SECTORS];
// Number of walls inside a sector
wallp_t s_nwalls[MAX_SECTORS];
// Floor height of sector
int16_t s_floor[MAX_SECTORS];
// Ceiling height of sector
int16_t s_ceil[MAX_SECTORS];
// AABB data
aabb_t s_aabb[MAX_WALLS];
// If the sector is visited, updated every frame
bool s_visited[MAX_WALLS];

// Wall information
// Wall vertices
p_t w_vert[MAX_WALLS];
// Portal data
sectp_t w_nextsect[MAX_WALLS];

sectp_t lastsect = 0;

sectp_t createSector(int16_t floor, int16_t ceil)
{
	if(lastsect == 0){
		s_fwall[lastsect] = 0;
	}else{
		s_fwall[lastsect] = s_fwall[lastsect - 1] + s_nwalls[lastsect - 1];
	}
	s_nwalls[lastsect] = 0;
	s_visited[lastsect] = 0;

	s_floor[lastsect] = floor;
	s_ceil[lastsect] = ceil;

	s_aabb[lastsect].minx = s_aabb[lastsect].miny = INT32_MAX;
	s_aabb[lastsect].maxx = s_aabb[lastsect].maxy = INT32_MIN;

	lastsect++;

	return lastsect - 1;
}

void addVertToSector(p_t vert, sectp_t nextsect)
{
	sectp_t sect = lastsect - 1;
	wallp_t wall = s_fwall[sect] + s_nwalls[sect];
	w_vert[wall][0] = vert[0];
	w_vert[wall][1] = vert[1];
	w_nextsect[wall] = nextsect;
	
	s_aabb[sect].minx = min(vert[0], s_aabb[sect].minx);
	s_aabb[sect].maxx = max(vert[0], s_aabb[sect].maxx);
	s_aabb[sect].miny = min(vert[1], s_aabb[sect].miny);
	s_aabb[sect].maxy = max(vert[1], s_aabb[sect].maxy);

	s_nwalls[sect]++;
}

//TODO change to concave polygon
/* http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html#Convex%20Polygons */
static bool pointInSector(sectp_t sect, p_t p)
{
	if(p[0] < s_aabb[sect].minx || p[0] > s_aabb[sect].maxx || p[1] < s_aabb[sect].miny || p[1] > s_aabb[sect].maxy){
		return false;
	}
	bool in = false;
	
	wallp_t swall = s_fwall[sect];
	wallp_t ewall = swall + s_nwalls[sect];
	wallp_t w1, w2;
	for(w1 = swall, w2 = ewall - 1; w1 < ewall; w2 = w1++){
		xy_t p1 = {w_vert[w1][0], w_vert[w1][1]};
		xy_t p2 = {w_vert[w2][0], w_vert[w2][1]};
		if(((p1.y > p[1]) != (p2.y > p[1])) 
				&& (p[0] < (p2.x - p1.x) * (p[1] - p1.y) / (p2.y - p1.y) + p1.x)){
			in = !in;
		}
	}

	return in;
}

sectp_t getSector(p_t xz, int32_t y)
{
	sectp_t sect;
	for(sect = 0; sect < lastsect; sect++){
		if(pointInSector(sect, xz) && y >= s_floor[sect] && y <= s_ceil[sect]){
			return sect;
		}
	}

	return -1;
}

texture_t *tex = NULL;
uint16_t texhw = 0, texhh = 0;
void setRenderTarget(texture_t *target)
{
	tex = target;
	texhw = tex->width >> 1;
	texhh = tex->height >> 1;
}

static void renderMinimapSector(sectp_t sect, p_t camloc, bool first)
{
	if(sect < 0 || s_visited[sect]){
		return;
	}

	s_visited[sect] = true;

	int xc = texhw - camloc[0];
	int yc = texhh - camloc[1];	
	
	wallp_t swall = s_fwall[sect];
	wallp_t ewall = swall + s_nwalls[sect];
	wallp_t w1, w2;
	for(w1 = swall, w2 = ewall - 1; w1 < ewall; w2 = w1++){
		xy_t p1 = {w_vert[w1][0] + xc, w_vert[w1][1] + yc};
		xy_t p2 = {w_vert[w2][0] + xc, w_vert[w2][1] + yc};
		if(w_nextsect[w1] < 0){
			if(first){
				drawLine(tex, p1, p2, COLOR_YELLOW);
			}else{
				drawLine(tex, p1, p2, COLOR_GRAY);
			}
		}else{
			drawLine(tex, p1, p2, COLOR_BLUE);
			renderMinimapSector(w_nextsect[w1], camloc, false);
		}
	}
}

static void renderMinimap(sectp_t sect, p_t camloc, v_t camangle)
{
	xy_t player = {texhw, texhh};
	xy_t pointto = {texhw + cos(camangle) * 5, texhh + sin(camangle) * 5};
	drawLine(tex, player, pointto, COLOR_DARKGREEN);
	drawCircle(tex, player, 3, COLOR_GREEN);

	memset(s_visited, 0, lastsect * sizeof(s_visited[0]));
	renderMinimapSector(sect, camloc, true);
}

void renderFromSector(camera_t cam)
{
	renderMinimap(cam.sect, cam.xz, cam.angle);
}
