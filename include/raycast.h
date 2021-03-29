#ifndef GAME_RAYCAST_H
#define GAME_RAYCAST_H

#include <nds/ndstypes.h>

#include <stdbool.h>

#include "vec.h"

typedef struct {
	vec3i pos;
	//Direction should be normalized!
	vec3s dir;
	u32 maxDistance;
	
	bool hit;
	u32 hitId;
	u32 hitDistance;
	vec3i hitPos;
} Ray;

void rayReset(Ray* ray);
bool rayCast(Ray* ray, u32 hitId, vec3i* min, vec3i* max);
#endif