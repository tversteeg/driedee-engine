#include "g_game.h"

#include <math.h>

#include "l_vector.h"
#include "l_render.h"
#include "l_console.h"
#include "g_map.h"

#define PLAYER_DAMPING 0.9
#define PLAYER_MOVE_ACC 0.2
#define PLAYER_MOVE_ROT 0.05

struct {
	camera_t cam;
	xyz_t pos, vel;
	v_t angle;
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

	moveCamera(&player.cam, (p_t){player.pos.x, player.pos.z}, player.pos.y);

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

	sectp_t s1 = createSector(0, 20);
	addVertToSector((p_t){200, 200}, WALL);
	addVertToSector((p_t){500, 200}, WALL);
	addVertToSector((p_t){500, 500}, 1);
	addVertToSector((p_t){200, 500}, WALL);	
	
	createSector(0, 20);
	addVertToSector((p_t){500, 200}, s1);
	addVertToSector((p_t){700, 200}, WALL);
	addVertToSector((p_t){700, 500}, WALL);
	addVertToSector((p_t){500, 500}, WALL);

	player.pos.x = 300;
	player.pos.z = 300;
	player.pos.y = 10;
	player.angle = player.vel.x = player.vel.y = player.vel.z = 0;

	_con = console;

	updatePlayer();
}

bool _buttonpress[4] = {0};
void updateGameWorld()
{
	if(_buttonpress[0]){
		player.vel.x += cos(player.cam.pitch) * PLAYER_MOVE_ACC;
		player.vel.z += sin(player.cam.pitch) * PLAYER_MOVE_ACC;
		_moveplayer = true;
	}
	if(_buttonpress[1]){
		player.vel.x += cos(player.cam.pitch + M_PI) * PLAYER_MOVE_ACC;
		player.vel.z += sin(player.cam.pitch + M_PI) * PLAYER_MOVE_ACC;
		_moveplayer = true;
	}
	if(_buttonpress[2]){
		player.angle -= PLAYER_MOVE_ROT;
		if(player.angle < -M_PI){
			player.angle += M_PI * 2;
		}
		rotateCamera(&player.cam, player.angle);
		_moveplayer = true;
	}
	if(_buttonpress[3]){
		player.angle += PLAYER_MOVE_ROT;
		if(player.angle > M_PI){
			player.angle -= M_PI * 2;
		}
		rotateCamera(&player.cam, player.angle);
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
