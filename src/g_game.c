#include "g_game.h"

#include "l_render.h"
#include "g_map.h"

void initGameWorld()
{
	map_t *map = createMap(20, 20);

	generateNoiseMap(map, 0);

	debugPrintMap(map);

	sectp_t sect = createSector(0, 10);
	addVertToSector((p_t){0, 0}, WALL);
	addVertToSector((p_t){10, 0}, WALL);
	addVertToSector((p_t){10, 10}, WALL);
	addVertToSector((p_t){0, 10}, WALL);
}

void updateGameWorld()
{

}

void inputGameWorld(ccEvent event)
{

}

void renderGameWorld(texture_t *screen)
{

}
