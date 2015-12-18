#include "l_render.h"

#include "l_colors.h"
#include "l_colors.h"

// Sector information
// Start of the wall pointer
wallp_t s_fwall[MAX_SECTORS];
// Number of walls inside a sector
wallp_t s_nwalls[MAX_SECTORS];
// Floor height of sector
int16_t s_floor[MAX_SECTORS];
// Ceiling height of sector
int16_t s_ceil[MAX_SECTORS];

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

	s_floor[lastsect] = floor;
	s_ceil[lastsect] = ceil;

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

	s_nwalls[sect]++;
}

//TODO change to concave polygon
/* http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html#Convex%20Polygons */
static bool pointInSector(sectp_t sect, p_t p)
{
	bool in = false;
	
	wallp_t swall = s_fwall[sect];
	wallp_t ewall = swall + s_nwalls[sect];
	wallp_t w1, w2;
	for(w1 = swall, w2 = ewall - 1; w1 < ewall; w2 = w1++){
		xy_t p1 = {w_vert[w1][0], w_vert[w1][1]};
		xy_t p2 = {w_vert[w2][0], w_vert[w2][1]};
		if(((p1.y > p[1]) != (p2.y > p[1])) && (p[0] < (p2.x - p1.x) * (p[1] - p1.y) / (p2.y - p1.y) + p1.x)){
			in = !in;
		}
	}

	return in;
}

sectp_t getSector(p_t xz, int32_t y)
{
	sectp_t sect;
	for(sect = 0; sect < lastsect; sect++){
		if(pointInSector(sect, xz) && s_floor[sect] <= y && s_ceil[sect] >= y){
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

static void renderMinimap(sectp_t sect, p_t camloc)
{
	wallp_t wall = s_fwall[sect];
	wallp_t maxwall = wall + s_nwalls[sect] - 1;

	int xc = texhw - camloc[0];
	int yc = texhh - camloc[1];

	xy_t v1 = {xc + w_vert[wall][0], yc + w_vert[wall][1]};
	xy_t v2 = {xc + w_vert[maxwall][0], yc + w_vert[maxwall][1]};
	drawLine(tex, v1, v2, COLOR_YELLOW);

	while(wall < maxwall){
		wall++;
		xy_t v1 = {xc + w_vert[wall][0], yc + w_vert[wall][1]};
		xy_t v2 = {xc + w_vert[wall - 1][0], yc + w_vert[wall - 1][1]};
		drawLine(tex, v1, v2, COLOR_YELLOW);
	}

	drawPixel(tex, texhw, texhh, COLOR_GREEN);
}

void renderFromSector(camera_t cam)
{
	renderMinimap(cam.sect, cam.xz);
}
