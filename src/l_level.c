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
				firstedge = true;
				break;
			case 'e':
				{
					edge_t edge;
					xy_t vert;
					sscanf(line + 2, "%d (%lf,%lf)", (int*)&edge.type, &vert.x, &vert.y);

					if(edge.type == WALL){
						edge.walltop = 20;
						edge.wallbot = -5;
					}

					if(firstedge){
						sect = createSector(vert, &edge);
						firstedge = false;
					}else{
						createEdge(sect, vert, &edge);
					}
				}
				break;
			case 'd':
				{
					unsigned int id;
					double walltop, wallbot, uvdiv;
					unsigned long texturehash;

					sscanf(line + 3, "%u%lf%lf%lf%lu", &id, &wallbot, &walltop, &uvdiv, &texturehash);

					edge_t *edge = sect->edges + id;
					edge->wallbot = wallbot;
					edge->walltop = walltop;
					edge->uvdiv = uvdiv;
					edge->texture = 0;

					int i;
					for(i = 0; i < textures; i++){
						if(hashes[i] == texturehash){
							edge->texture = i;
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
	sect->lastsprite = (void*)sprite;

	return sprite;
}

void destroySprite(sector_t *sect, sprite_t *sprite)
{
	if(sect->lastsprite == sprite){
		sect->lastsprite = sprite->prev;
	}
	if(sprite->next != NULL){
		sprite->next->prev = sprite->prev;
	}
	if(sprite->prev != NULL){
		sprite->prev->next = sprite->next;
	}
	free(sprite);
}
