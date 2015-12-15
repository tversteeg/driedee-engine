#include "g_game.h"

#include "g_map.h"

void initGameWorld()
{
	map_t *map = createMap(20, 20);

	generateNoiseMap(map, 0);

	debugPrintMap(map);
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
