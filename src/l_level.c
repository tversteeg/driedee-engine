#include "l_level.h"

#include "l_utils.h"

#include <stdio.h>

bool loadLevel(const char *filename)
{
	if(filename == NULL){
		printf("No level file supplied\n");
		return false;
	}

	FILE *fp = fopen(filename, "rt");
	if(!fp){
		printf("Couldn't open file for reading: %s\n", filename);
		return false;
	}

	char *line = NULL;
	size_t len = 0, textures = 0;
	bool firstedge = true;
	sector_t *sect = NULL;
	hash_t *hashes = NULL;
	sector_t tempsect;

	while(getline(&line, &len, fp) != -1){
		switch(line[0]){
			case 't':
				{
					char name[80];
					sscanf(line + 2, "%s", name);
					hash_t namehash = hash(name);

					hashes = (hash_t*)realloc(hashes, ++textures * sizeof(hash_t));
					hashes[textures - 1] = namehash;
				}
				break;
			case 's':
				{
					unsigned long ceiltexhash, floortexhash;
					sscanf(line + 2, "%lf%lf%lu%lu", &tempsect.ceil, &tempsect.floor, &ceiltexhash, &floortexhash);
					firstedge = true;

					tempsect.ceiltex = tempsect.floortex  =  0;

					unsigned int i;
					for(i = 0; i < textures; i++){
						if(hashes[i] == ceiltexhash){
							tempsect.ceiltex = i;
						}else if(hashes[i] == floortexhash){
							tempsect.floortex = i;
						}
					}

					//TODO fix hashes not loading correctly
				}
				break;
			case 'e':
				{
					edgetype_t type;
					xy_t vert;
					sscanf(line + 2, "%d (%f,%f)", (int*)&type, &vert.x, &vert.y);

					if(firstedge){
						sect = createSector(vert, type);
						sect->ceil = tempsect.ceil;
						sect->floor = tempsect.floor;
						sect->ceiltex = tempsect.ceiltex;
						sect->floortex = tempsect.floortex;
						firstedge = false;
					}else{
						createEdge(sect, vert, type);
					}
				}
				break;
			case 'd':
				{
					unsigned int id;
					double uvdiv;
					unsigned long texturehash;

					sscanf(line + 3, "%u%lf%lu", &id, &uvdiv, &texturehash);

					edge_t *edge = sect->edges + id;
					edge->uvdiv = uvdiv;
					edge->texture = 0;

					unsigned int i;
					for(i = 0; i < textures; i++){
						if(hashes[i] == texturehash){
							edge->texture = i;
							break;
						}
					}
				}
				break;
			case 'p':
				{
					unsigned int isector1, iedge1, isector2, iedge2;
					sscanf(line + 2, "%u%u%u%u", &isector1, &iedge1, &isector2, &iedge2);

					sector_t *sect1 = getSector(isector1);
					sector_t *sect2 = getSector(isector2);
					if(sect1 == NULL || sect2 == NULL){
						exit(1);
					}

					edge_t *edge1 = sect1->edges + iedge1;
					edge_t *edge2 = sect2->edges + iedge2;

					edge1->neighbor = edge2;
					edge2->neighbor = edge1;
				}
				break;
		}
	}

	free(hashes);

	return true;
}

void moveSprite(sector_t *to, sector_t *from, sprite_t *sprite)
{
	if(from->lastsprite == sprite){
		from->lastsprite = sprite->next;
	}

	if(sprite->next != NULL){
		sprite->next->prev = sprite->prev;
	}		

	if(sprite->prev != NULL){
		sprite->prev->next = sprite->next;
	}

	sprite->next = NULL;
	sprite->prev = (sprite_t*)to->lastsprite;
	if(sprite->prev != NULL){
		sprite->prev->next = sprite;
	}
	to->lastsprite = (void*)sprite;
}

sprite_t *spawnSprite(sector_t *sect, xyz_t pos, xy_t scale, char texture)
{
	if(sect == NULL){
		fprintf(stderr, "Sprites can only be added to sectors.\n");
		return NULL;
	}

	sprite_t *sprite = (sprite_t*)malloc(sizeof(sprite_t));
	sprite->pos = pos;
	sprite->scale = scale;
	sprite->texture = texture;

	sprite->next = NULL;
	sprite->prev = (sprite_t*)sect->lastsprite;
	if(sprite->prev != NULL){
		sprite->prev->next = sprite;
	}
	sect->lastsprite = (void*)sprite;

	return sprite;
}

void destroySprite(sector_t *sect, sprite_t *sprite)
{
	if(sect->lastsprite == sprite){
		sect->lastsprite = sprite->next;
	}

	if(sprite->next != NULL){
		sprite->next->prev = sprite->prev;
	}		

	if(sprite->prev != NULL){
		sprite->prev->next = sprite->next;
	}

	free(sprite);
}

sector_t *tryMoveSprite(sector_t *sect, sprite_t *sprite, xy_t pos)
{
	if(pointInSector(sect, pos)){
		sprite->pos.x = pos.x;
		sprite->pos.z = pos.y;
		return sect;
	}

	xy_t oldpos = {sprite->pos.x, sprite->pos.z};
	unsigned int i;
	for(i = 0; i < sect->nedges; i++){
		edge_t *edge = sect->edges + i;
		if(edge->type != PORTAL){
			continue;
		}
		xy_t edge1 = sect->vertices[edge->vertex1];
		xy_t edge2 = sect->vertices[edge->vertex2];
		xy_t result;
		if(segmentSegmentIntersect(pos, oldpos, edge1, edge2, &result)){
			moveSprite(getSector(edge->neighbor->sector), sect, sprite);
			return getSector(edge->neighbor->sector);
		}
	}
	return NULL;
}

edge_t *findWallRay(xy_t *result, const sector_t *sect, xy_t point, xy_t dir)
{
	edge_t *previous = NULL;
	const sector_t *current = sect;
	do {
		previous = NULL;
		unsigned int i;
		for(i = 0; i < current->nedges; i++){
			edge_t *edge = current->edges + i;
			xy_t edge1 = current->vertices[edge->vertex1];
			xy_t edge2 = current->vertices[edge->vertex2];
			xy_t isect;
			if(raySegmentIntersect(point, dir, edge1, edge2, &isect)){
				if(edge->type == WALL){
					*result = isect;
					return edge;
				}else if(edge->type == PORTAL && edge->neighbor != previous){
					point.x = isect.x + dir.x;
					point.y = isect.y + dir.y;
					current = getSector(edge->neighbor->sector);
					previous = edge;
					break;
				}
			}
		}
	} while(current != NULL && previous != NULL);

	return NULL;
}
