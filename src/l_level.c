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

	char *line = NULL;
	size_t len = 0;
	sector_t *sect = NULL;
	xy_t vert;
	while(getline(&line, &len, fp) != -1){
		switch(line[0]){
			case 's':
				if(sect != NULL){
					sect->vertices[0] = vert;
				}
				sect = createSector((xy_t){0, 0});
				break;
			case 'e':
				{
					unsigned int neighborsector, neighboredge;
					edgetype_t type;
					int nscanned = sscanf(line, "%*s %d (%lf,%lf) %u %u", 
							(int*)&type, &vert.x, &vert.y, &neighborsector, &neighboredge);
					unsigned int edgenum = createEdge(sect, vert, type);
					if(nscanned == 5){
						printf("%d\n", edgenum);
						// Store neighbors
					}
				}
				break;
		}
	}

	if(sect != NULL){
		sect->vertices[0] = vert;
	}
}
