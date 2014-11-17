#include "sprites.h"

#include <stdlib.h>
#include <string.h>

float *_x, *_y, *_width, *_height;
char *_active;

unsigned int _max;

void spriteInit()
{
	_max = 1;

	_active = malloc(sizeof(char));
	_x = malloc(sizeof(float));
	_y = malloc(sizeof(float));
	_width = malloc(sizeof(float));
	_height = malloc(sizeof(float));
}

int spriteCreate()
{
	unsigned int i, prevMax;	

	i = 0;
	while(_active[i] != 0 && i < _max){
		i++;
	}

	if(i == _max){
		prevMax = _max;
		_max <<= 1;

		_active = realloc(_active, _max * sizeof(char));
		_x = realloc(_x, _max * sizeof(float));
		_y = realloc(_y, _max * sizeof(float));
		_width = realloc(_width, _max * sizeof(float));
		_height = realloc(_height, _max * sizeof(float));

		memset(_active + prevMax, 0, _max - prevMax);
	}

	_active[i] = 1;
	_x[i] = _y[i] = _width[i] = _height[i] = 0;

	return (int)i;
}

void spriteDestroy(int sprite)
{
	_active[sprite] = 0;
}

void spriteSetX(int sprite, float x)
{
	_x[sprite] = x;
}

void spriteSetY(int sprite, float y)
{
	_y[sprite] = y;
}

void spriteSetWidth(int sprite, float width)
{
	_width[sprite] = width;
}

void spriteSetHeight(int sprite, float height)
{
	_height[sprite] = height;
}

float spriteGetX(int sprite)
{
	return _x[sprite];
}

float spriteGetY(int sprite)
{
	return _y[sprite];
}

float spriteGetWidth(int sprite)
{
	return _width[sprite];
}

float spriteGetHeight(int sprite)
{
	return _height[sprite];
}
