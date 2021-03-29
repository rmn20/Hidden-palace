#include <nds/arm9/math.h>

#include <stdio.h>

#include "raycast.h"

void rayReset(Ray* ray) {
	ray->hit = false;
	ray->hitId = 0;
	ray->hitDistance = ray->maxDistance;
}

bool rayCastBox(Ray* ray, u32 hitId, vec3i* min, vec3i* max, const u8 xyz, s32 rayEndValue, s32 distance) {
	if(distance > ray->hitDistance) return false;
	
	vec3i hit = (vec3i) {rayEndValue, rayEndValue, rayEndValue};
	
	if(xyz != 0) hit.x = mulf32(ray->dir.x, distance) + ray->pos.x;
	if(xyz != 1) hit.y = mulf32(ray->dir.y, distance) + ray->pos.y;
	if(xyz != 2) hit.z = mulf32(ray->dir.z, distance) + ray->pos.z;
	
	if(hit.x >= min->x && hit.x <= max->x &&
			hit.y >= min->y && hit.y <= max->y &&
			hit.z >= min->z && hit.z <= max->z) {
		
		ray->hit = true;
		ray->hitId = hitId;
		ray->hitDistance = distance;
		ray->hitPos.x = hit.x;
		ray->hitPos.y = hit.y;
		ray->hitPos.z = hit.z;
		
		return true;
	}
	
	return false;
}

bool rayCast(Ray* ray, u32 hitId, vec3i* min, vec3i* max) {
	if(ray->pos.x >= min->x && ray->pos.x <= max->x &&
			ray->pos.y >= min->y && ray->pos.y <= max->y &&
			ray->pos.z >= min->z && ray->pos.z <= max->z) {
		
		ray->hit = true;
		ray->hitId = hitId;
		ray->hitDistance = 0;
		ray->hitPos.x = ray->pos.x;
		ray->hitPos.y = ray->pos.y;
		ray->hitPos.z = ray->pos.z;
		
		return true;
	}
	
	if(ray->dir.y < 0 && ray->pos.y >= max->y && ray->pos.y - ray->hitDistance <= max->y) {
		if(rayCastBox(ray, hitId, min, max, 1, max->y, divf32(max->y - ray->pos.y, ray->dir.y))) return true;
		
	} else if(ray->dir.y > 0 && ray->pos.y <= min->y && ray->pos.y + ray->hitDistance >= min->y) {
		if(rayCastBox(ray, hitId, min, max, 1, min->y, divf32(min->y - ray->pos.y, ray->dir.y))) return true;
		
	}
	
	if(ray->dir.x < 0 && ray->pos.x >= max->x && ray->pos.x - ray->hitDistance <= max->x) {
		if(rayCastBox(ray, hitId, min, max, 0, max->x, divf32(max->x - ray->pos.x, ray->dir.x))) return true;
		
	} else if(ray->dir.x > 0 && ray->pos.x <= min->x && ray->pos.x + ray->hitDistance >= min->x) {
		if(rayCastBox(ray, hitId, min, max, 0, min->x, divf32(min->x - ray->pos.x, ray->dir.x))) return true;
		
	}
	
	if(ray->dir.z < 0 && ray->pos.z >= max->z && ray->pos.z - ray->hitDistance <= max->z) {
		if(rayCastBox(ray, hitId, min, max, 2, max->z, divf32(max->z - ray->pos.z, ray->dir.z))) return true;
		
	} else if(ray->dir.z > 0 && ray->pos.z <= min->z && ray->pos.z + ray->hitDistance >= min->z) {
		if(rayCastBox(ray, hitId, min, max, 2, min->z, divf32(min->z - ray->pos.z, ray->dir.z))) return true;
		
	}
	
	return false;
}