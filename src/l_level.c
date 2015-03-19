#include "l_level.h"

#include <stdio.h>

void loadLevel(const char *filename)
{
	if(filename == NULL){
		printf("No level file supplied\n");
		return;
	}

	FILE *fp = fopen(filename, "rt");
	if(!fp){
		printf("Couldn't open file for reading: %s\n", filename);
		return;
	}

	unsigned int cursector = 0;
	char *line = NULL;
	size_t len = 0;
	bool firstedge = true;
	sector_t *sect = NULL;
	while(getline(&line, &len, fp) != -1){
		switch(line[0]){
			case 's':
				sscanf(line,"%*s %u", &cursector);
				firstedge = true;
				break;
			case 'e':
				{
					edgetype_t type;
					xy_t vert;
					sscanf(line, "%*s %d (%lf,%lf)", (int*)&type, &vert.x, &vert.y);

					if(firstedge){
						sect = createSector(vert, type);
						firstedge = false;
					}else{
						createEdge(sect, vert, type);
					}
				}
				break;
			case 'p':
				{
					unsigned int isector1, iedge1, isector2, iedge2;
					sscanf(line, "%*s %u %u %u %u", &isector1, &iedge1, &isector2, &iedge2);

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
}