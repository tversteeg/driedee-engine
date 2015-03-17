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
					unsigned int neighborsector, neighboredge;
					edgetype_t type;
					xy_t vert;
					int nscanned = sscanf(line, "%*s %d (%lf,%lf) %d %u", 
							(int*)&type, &vert.x, &vert.y, &neighborsector, &neighboredge);

					edge_t *edge;
					if(firstedge){
						firstedge = false;

						sect = createSector(vert, type);
						edge = sect->edges;
					}else{
						edge = createEdge(sect, vert, type);
					}

					if(nscanned == 5 && neighborsector < cursector){
						// Store neighbors
						printf("%d\n", neighborsector);
						sector_t *neighbor = getFirstSector();
						while(neighborsector > 0){
							neighborsector--;

							neighbor = getNextSector(neighbor);
						}
						printf("%p:%p -> ", edge, edge->sector);
						edge_t *nedge = neighbor->edges + neighboredge;
						printf("%p:%p\n", nedge, nedge->sector);
						//(neighbor->edges + neighboredge)->neighbor = edge;
						edge->neighbor = nedge;
						nedge->neighbor = edge;
					}
				}
				break;
		}
	}
}
