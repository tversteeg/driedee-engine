#pragma once

#include <ccore/event.h>
#include <ccTerm/ccTerm.h>

#include <driedee/draw.h>

void initGameWorld(cctTerm *console);
void updateGameWorld();
void inputGameWorld(ccEvent event);
void renderGameWorld(texture_t *screen);
