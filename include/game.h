#ifndef GAME_GAME_H
#define GAME_GAME_H

#include "vec.h"

void gameInit();
void gamePlayDoorSound();
void gameQueueMap(char* map, vec3i* newPos, s16* newRotX, s16* newRotY);
void gameDestroy();

void gameTick(u16 held, u16 press, u16 release);
void gameRender();
#endif