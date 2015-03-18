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
					unsigned int sector1, edge1, sector2, edge2;
					sscanf(line, "%*s %u %u %u %u", &sector1, &edge1, &sector2, &edge2);
					printf("%u %u %u %u\n", sector1, edge1, sector2, edge2);
				}
				break;
		}
	}
}
