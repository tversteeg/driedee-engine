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
						sector_t *neighbor = getFirstSector();
						while(neighborsector > 0){
							neighborsector--;

							neighbor = getNextSector(neighbor);
						}
						(neighbor->edges + neighboredge)->neighbor = edge;
						edge->neighbor = neighbor->edges + neighboredge;
					}
				}
				break;
		}
	}
}
