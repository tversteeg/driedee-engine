#include <rogueliek/render.h>

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <rogueliek/colors.h>

#define MAX_SECTOR_STACK 32

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

typedef struct {
	wallp_t *walls, nwalls;
} bunch_t;

void renderFromSector(camera_t cam)
{
	renderMinimap(cam.sect, cam.xz, cam.pitch);

	memset(s_visited, 0, lastsect * sizeof(s_visited[0]));

	xy_t camvec = {cos(cam.pitch), sin(cam.pitch)};

	uint16_t maxbunches = 32;
	uint16_t nbunches = 0;
	bunch_t *bunches = (bunch_t*)malloc(maxbunches * sizeof(bunch_t));

	uint16_t nsectors = 1;
	sectp_t sectors[MAX_SECTOR_STACK];
	sectors[0] = cam.sect;

	while(nsectors > 0){
		nsectors--;
		sectp_t sect = sectors[nsectors];
		s_visited[sect] = true;

		wallp_t swall = s_fwall[sect];
		wallp_t ewall = swall + s_nwalls[sect];
		wallp_t w1, w2;
		for(w1 = swall, w2 = ewall - 1; w1 < ewall; w2 = w1++){
			// Clip the wall when it's not in view
			xy_t wallvec = {w_vert[w2][0] - w_vert[w1][0], w_vert[w2][1] - w_vert[w1][1]};
			if(vectorCrossProduct(wallvec, camvec) < 0){
				continue;
			}

			// Add the wall to a bunch when they connect or create a new one
			uint16_t i;
			for(i = 0; i < nbunches; i++){
				bunch_t *bunch = bunches + i;
				wallp_t j;
				for(j = 0; j < bunch->nwalls; j++){
					int32_t bx = w_vert[bunch->walls[j]][0];
					int32_t by = w_vert[bunch->walls[j]][1];
					// Add to the bunch if the existing bunches already match vertices with the wall somewhere
					if(bx == w_vert[w1][0] && by == w_vert[w1][1]){
						bunch->nwalls++;
						bunch->walls = (wallp_t*)realloc(bunch->walls, bunch->nwalls * sizeof(wallp_t));
						bunch->walls[bunch->nwalls - 1] = w1;
						goto walladded;
					}
				}
			}
			// Wall is not found, create a new bunch
			nbunches++;
			if(nbunches > maxbunches){
				maxbunches *= 2;
				bunches = (bunch_t*)realloc(bunches, maxbunches * sizeof(bunch_t));
			}
			bunch_t *bunch = bunches + nbunches - 1;
			bunch->nwalls = 1;
			bunch->walls = (wallp_t*)malloc(sizeof(wallp_t));
			bunch->walls[0] = w1;

walladded:;
			// If the wall is a portal, and not visited yet, add the sector to the stack
			if(w_nextsect[w1] >= 0 && !s_visited[w_nextsect[w1]]){
				nsectors++;
				if(nsectors > MAX_SECTOR_STACK){
					fprintf(stderr, "To much sectors added to stack\n");
					exit(1);
				}

				sectors[nsectors - 1] = w_nextsect[w1];
			}
		}
	}

	while(nbunches > 0){
		nbunches--;

		bunch_t *bunch = bunches + nbunches;

		uint16_t i;
		for(i = 0; i < nbunches - 1; i++){

		}
	}

	uint16_t i;
	for(i = 0; i < nbunches; i++){
		free(bunches[i].walls);
	}
	free(bunches);
}

static void createModelMatrix(camera_t *cam)
{
	ccVec3 translation;
	translation.x = cam->xz[0];
	translation.y = cam->y;
	translation.z = cam->xz[1];
	ccMat4x4SetTranslation(cam->modelm, translation);
	ccMat4x4RotateY(cam->modelm, cam->yaw);
	ccMat4x4RotateX(cam->modelm, cam->pitch);
}

void createPerspProjMatrix(camera_t *cam, v_t fov, v_t aspect, v_t znear, v_t zfar)
{
	ccMat4x4Perspective(cam->persm, fov, aspect, znear, zfar);
}

void moveCamera(camera_t *cam, p_t xz, int32_t y)
{
	cam->xz[0] = xz[0];
	cam->xz[1] = xz[1];
	cam->y = y;

	createModelMatrix(cam);

	cam->sect = getSector(xz, y);
}

void rotateCamera(camera_t *cam, v_t angle)
{
	cam->pitch = angle;
	cam->yaw = 0;

	createModelMatrix(cam);
}
