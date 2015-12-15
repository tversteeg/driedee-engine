#pragma once

#include <ccore/event.h>

#include "l_draw.h"
#include "l_console.h"

void initGameWorld(console_t *console);
void updateGameWorld();
void inputGameWorld(ccEvent event);
void renderGameWorld(texture_t *screen);
