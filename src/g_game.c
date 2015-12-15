#include "g_game.h"

#include <stdlib.h>

#include "l_vector.h"
#include "l_render.h"
#include "l_console.h"
#include "g_map.h"

static void c_addsector(console_t *con, int argc, char **argv)
{
	if(argc != 2){
		printConsole(con, "Usage: addsector floor ceil\n");
	}else{
		createSector(atoi(argv[0]), atoi(argv[1]));
	}
}

p_t cam_pos;
v_t cam_angle;
sectp_t cam_sect;

void initGameWorld(console_t *console)
{
	map_t *map = createMap(20, 20);

	generateNoiseMap(map, 0);

	debugPrintMap(map);

	cam_sect = createSector(0, 10);
	addVertToSector((p_t){20, 20}, WALL);
	addVertToSector((p_t){50, 20}, WALL);
	addVertToSector((p_t){50, 50}, WALL);
	addVertToSector((p_t){20, 50}, WALL);

	cam_pos[0] = 30;
	cam_pos[1] = 30;
	cam_angle = 0;

	mapCmdConsole(console, "addsect", c_addsector);
}

void updateGameWorld()
{

}

void inputGameWorld(ccEvent event)
{

}

void renderGameWorld(texture_t *screen)
{
	setRenderTarget(screen);
	renderFromSector(cam_sect, cam_pos, cam_angle);
}
