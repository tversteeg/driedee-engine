#include <driedee/render.h>

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <driedee/colors.h>

#define MAX_SECTOR_STACK 32

#define Z_NEAR 1e-4f
#define Z_FAR 5.0f
#define SIDE_NEAR 1e-5f
#define SIDE_FAR 20.0f

#define H_FOV 0.73f
#define V_FOV 0.2f

// Taken from Bisqwit: http://bisqwit.iki.fi/jutut/kuvat/programming_examples/portalrendering.html
#define clamp(a, mi, ma)                                                       \
  min(max(a, mi), ma) // clamp: Clamp value into set range.
#define vxs(x0, y0, x1, y1)                                                    \
  ((x0) * (y1) - (x1) * (y0)) // vxs: Vector cross product
// Overlap:  Determine whether the two number ranges overlap.
#define Overlap(a0, a1, b0, b1)                                                \
  (min(a0, a1) <= max(b0, b1) && min(b0, b1) <= max(a0, a1))
// IntersectBox: Determine whether two 2D-boxes intersect.
#define IntersectBox(x0, y0, x1, y1, x2, y2, x3, y3)                           \
  (Overlap(x0, x1, x2, x3) && Overlap(y0, y1, y2, y3))
// PointSide: Determine which side of a line the point is on. Return value: <0,
// =0 or >0.
#define PointSide(px, py, x0, y0, x1, y1)                                      \
  vxs((x1) - (x0), (y1) - (y0), (px) - (x0), (py) - (y0))
// Intersect: Calculate the point of intersection between two lines.
#define Intersect(x1, y1, x2, y2, x3, y3, x4, y4)                              \
  ((xy_t){vxs(vxs(x1, y1, x2, y2), (x1) - (x2), vxs(x3, y3, x4, y4),      \
                   (x3) - (x4)) /                                              \
                   vxs((x1) - (x2), (y1) - (y2), (x3) - (x4), (y3) - (y4)),    \
               vxs(vxs(x1, y1, x2, y2), (y1) - (y2), vxs(x3, y3, x4, y4),      \
                   (y3) - (y4)) /                                              \
                   vxs((x1) - (x2), (y1) - (y2), (x3) - (x4), (y3) - (y4))})

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

static inline xy_t worldToCam(p_t p, camera_t cam)
{
	xy_t c = {cam.xz[0] - p[0], cam.xz[1] - p[1]};

	return (xy_t){cam.anglecos * c.x - cam.anglesin * c.y, cam.anglesin * c.x + cam.anglecos * c.y};
}

static void renderRooms(camera_t cam)
{
	bool visited[lastsect];
	memset(visited, 0, lastsect * sizeof(visited[0]));

	typedef struct {
		sectp_t sect;
		int sx1, sx2;
	} _item;

	_item queue[MAX_SECTOR_STACK];
	_item *head = queue, *tail = queue;

	*head = (_item){cam.sect, 0, tex->width - 1};
	head++;

	v_t hfovf = 1.0f / (H_FOV * (v_t)tex->height);
	v_t vfovf = 1.0f / (V_FOV * (v_t)tex->height);

	do{
		_item item = *tail;
		if(++tail == queue + MAX_SECTOR_STACK){
			tail = queue;
		}
		if(visited[item.sect]){
			continue;
		}

		// Render walls
		wallp_t swall = s_fwall[item.sect], ewall = swall + s_nwalls[item.sect];
		for(wallp_t w1 = swall, w2 = ewall - 1; w1 < ewall; w2 = w1++){
			xy_t v1 = worldToCam(w_vert[w1], cam);
			xy_t v2 = worldToCam(w_vert[w2], cam);

			// Clip walls fully behind the player & find the intersection point when the walls are partially hidden
			if(v1.y < 0 && v2.y < 0){
				continue;
			}else if(v1.y < Z_NEAR){
				xy_t i = Intersect(v1.x, v1.y, v2.x, v2.y, -SIDE_NEAR, Z_NEAR, -SIDE_FAR, Z_FAR);
				if(i.y > 0){
					v1 = i;
				}else{
					v1 = Intersect(v1.x, v1.y, v2.x, v2.y, SIDE_NEAR, Z_NEAR, SIDE_FAR, Z_FAR);
				}
			}else if(v2.y < Z_NEAR){
				xy_t i = Intersect(v1.x, v1.y, v2.x, v2.y, -SIDE_NEAR, Z_NEAR, -SIDE_FAR, Z_FAR);
				if(i.y > 0){
					v2 = i;
				}else{
					v2 = Intersect(v1.x, v1.y, v2.x, v2.y, SIDE_NEAR, Z_NEAR, SIDE_FAR, Z_FAR);
				}
			}

			// Do perspective transformation
			v_t xscale1 = v1.y * hfovf;
			int x1 = texhw - (int)(v1.x * xscale1);
			v_t xscale2 = v2.y * hfovf;
			int x2 = texhw - (int)(v2.x * xscale2);
			if(x1 >= x2 || x2 < item.sx1 || x1 > item.sx2){
				continue;
			}

			v_t yceil = s_ceil[item.sect] - cam.y;
			v_t yfloor = s_floor[item.sect] - cam.y;

			sectp_t neighbor = w_nextsect[w1];
			float nyceil = 0, nyfloor = 0;
			if(neighbor >= 0){
				nyceil = s_ceil[neighbor] - cam.y;
				nyfloor = s_floor[neighbor] - cam.y;
			}
		}
		visited[item.sect] = true;
	}while(head != tail);
}

void renderFromSector(camera_t cam)
{
	renderMinimap(cam.sect, cam.xz, cam.pitch);

	renderRooms(cam);

	/*

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

	*/
}

void moveCamera(camera_t *cam, p_t xz, int32_t y)
{
	cam->xz[0] = xz[0];
	cam->xz[1] = xz[1];
	cam->y = y;

	cam->sect = getSector(xz, y);
}

void rotateCamera(camera_t *cam, v_t angle)
{
	cam->pitch = angle;
	cam->yaw = 0;

	cam->anglecos = cosf(angle);
	cam->anglesin = sinf(angle);
}
