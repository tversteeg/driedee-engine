#pragma once

#include "l_draw.h"

bool getSizePng(const char *file, unsigned int *width, unsigned int *height);
bool loadPng(texture_t *tex, const char *file);
