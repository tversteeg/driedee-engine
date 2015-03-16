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
	xy_t vert;
	while(getline(&line, &len, fp) != -1){
		switch(line[0]){
			case 's':
				sect = createSector((xy_t){0, 0});
				sscanf(line,"%*s %u", &cursector);
				printf("%d: %p\n", cursector, sect);
				firstedge = true;
				break;
			case 'e':
				{
					unsigned int neighborsector, neighboredge;
					edgetype_t type;
					int nscanned = sscanf(line, "%*s %d (%lf,%lf) %u %u", 
							(int*)&type, &vert.x, &vert.y, &neighborsector, &neighboredge);

					edge_t *edge;
					if(firstedge){
						sect->vertices[0] = vert;
						edge = sect->edges;
						firstedge = false;
					}else{
						edge = createEdge(sect, vert, type);
					}
					if(nscanned == 5 && neighborsector < cursector){
						// Store neighbors
						printf("- %d:%d\n", neighborsector, neighboredge);
						sector_t *neighbor = getFirstSector();
						if(neighborsector != 0){
							while(--neighborsector > 0){
								neighbor = getNextSector(neighbor);
							}
						}
						neighbor->edges[neighboredge].neighbor = edge;
						edge->neighbor = neighbor->edges + neighboredge;
					}
				}
				break;
		}
	}

	if(sect != NULL){
		sect->vertices[0] = vert;
	}
}
