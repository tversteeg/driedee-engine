#pragma once

#include <ccore/event.h>

#include <rogueliek/draw.h>
#include <rogueliek/console.h>

void initGameWorld(console_t *console);
void updateGameWorld();
void inputGameWorld(ccEvent event);
void renderGameWorld(texture_t *screen);
