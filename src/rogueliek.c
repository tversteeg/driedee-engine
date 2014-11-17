#include <ccore/display.h>
#include <ccore/window.h>
#include <ccore/event.h>
#include <ccore/time.h>
#include <ccore/print.h>

#include "sprites.h"

int main(int arg, char **argv)
{
	bool loop;
	ccEvent event;
	int sprite;

	ccDisplayInitialize();
	ccWindowCreate((ccRect){.x = 0, .y = 0, .width = 800, .height = 600},
			"Rogueliek", 0);

	spriteInit();

	loop = true;
	while(loop){
		while(ccWindowEventPoll()){
			event = ccWindowEventGet();
			switch(event.type){
				case CC_EVENT_WINDOW_QUIT:
					loop = false;
					break;
				default:
					break;
			}
		}

		sprite = spriteCreate();
		if(sprite % 100 == 0){
			ccPrintf("%d\n", sprite);
		}

		ccTimeDelay(2);
	}

	ccFree();
	
	return 0;
}
