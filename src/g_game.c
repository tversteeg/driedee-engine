#include "g_game.h"

#include "l_vector.h"
#include "l_render.h"
#include "l_console.h"
#include "g_map.h"

#define PLAYER_DAMPING 0.9

struct {
	camera_t cam;
	xyz_t pos, vel;
} player;

static bool _moveplayer = true;
static void updatePlayer()
{
	if(!_moveplayer){
		return;
	}
	player.pos.x += player.vel.x;
	player.pos.z += player.vel.z;
	player.pos.y += player.vel.y;

	player.vel.x *= PLAYER_DAMPING;
	player.vel.z *= PLAYER_DAMPING;
	player.vel.y *= PLAYER_DAMPING;

	player.cam.xz[0] = (int32_t)(player.pos.x + 0.5);
	player.cam.xz[1] = (int32_t)(player.pos.z + 0.5);
	player.cam.y = (int32_t)(player.pos.y + 0.5);

	//player.cam.sect = getSector(player.cam.xz, player.cam.y);
	if(player.vel.x == 0 && player.vel.y == 0 && player.vel.z == 0){
		_moveplayer = false;
	}
}

static console_t *_con;
void initGameWorld(console_t *console)
{
	map_t *map = createMap(20, 20);

	generateNoiseMap(map, 0);

	debugPrintMap(map);

	createSector(0, 10);
	addVertToSector((p_t){20, 20}, WALL);
	addVertToSector((p_t){50, 20}, WALL);
	addVertToSector((p_t){50, 50}, WALL);
	addVertToSector((p_t){20, 50}, WALL);

	player.cam.xz[0] = 30;
	player.cam.xz[1] = 30;
	player.cam.y = 10;
	player.cam.angle = 0;

	_con = console;
}

bool _buttonpress[4] = {0};
void updateGameWorld()
{
	if(_buttonpress[0]){
		player.vel.z -= 0.1;
		_moveplayer = true;
	}
	if(_buttonpress[1]){
		player.vel.z += 0.1;
		_moveplayer = true;
	}
	if(_buttonpress[2]){
		player.vel.x -= 0.1;
		_moveplayer = true;
	}
	if(_buttonpress[3]){
		player.vel.x += 0.1;
		_moveplayer = true;
	}

	updatePlayer();
}

void inputGameWorld(ccEvent event)
{
	if(event.type == CC_EVENT_KEY_DOWN){
		switch(event.keyCode){
			case CC_KEY_W:
				_buttonpress[0] = true;
				break;
			case CC_KEY_S:
				_buttonpress[1] = true;
				break;
			case CC_KEY_A:
				_buttonpress[2] = true;
				break;
			case CC_KEY_D:
				_buttonpress[3] = true;
				break;
			default: break;
		}
	}else if(event.type == CC_EVENT_KEY_UP){
		switch(event.keyCode){
			case CC_KEY_W:
				_buttonpress[0] = false;
				break;
			case CC_KEY_S:
				_buttonpress[1] = false;
				break;
			case CC_KEY_A:
				_buttonpress[2] = false;
				break;
			case CC_KEY_D:
				_buttonpress[3] = false;
				break;
			default: break;
		}
	}
}

void renderGameWorld(texture_t *screen)
{
	setRenderTarget(screen);
	renderFromSector(player.cam);
}
