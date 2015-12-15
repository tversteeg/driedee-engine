#pragma once

#include <ccore/event.h>

#include "l_draw.h"

void initGameWorld();
void updateGameWorld();
void inputGameWorld(ccEvent event);
void renderGameWorld(texture_t *screen);
