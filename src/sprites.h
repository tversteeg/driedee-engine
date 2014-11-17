#pragma once

void spriteInit();
int spriteCreate();
void spriteDestroy(int sprite);

void spriteSetX(int sprite, float x);
void spriteSetY(int sprite, float y);
void spriteSetWidth(int sprite, float width);
void spriteSetHeight(int sprite, float height);

float spriteGetX(int sprite);
float spriteGetY(int sprite);
float spriteGetWidth(int sprite);
float spriteGetHeight(int sprite);
