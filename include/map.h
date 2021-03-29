#ifndef GAME_MAP_H
#define GAME_MAP_H

#include "model.h"
#include "texture.h"
#include "raycast.h"

#define SKY_FLAG 1

#define MUSIC_FLAG 2
#define MUSIC_STOP_FLAG 4

typedef struct {
	u8* data;
	u32 flags;
	
	u32 skyTexture;
	
	u32 boxesCount;
	vec3i* boxes;
	
	u32 scriptsCount;
	vec3i* scriptBoxes;
	u32* scriptFlags;
	u32* scripts;
	
	u32 texsCount;
	//Texture name length is 32
	char* texsNames;
	
	u32 modelsCount;
	u32* modelsTexs;
} MapFile;

typedef struct {
	u32 musicID;
	Model* skyModel;
	
	MapFile file;
	
	Texture** texs;
	Model** models;
} Map;

Map* mapNew();
void mapDestroy(Map* map);
void mapLoad(Map* map, char* path);

void mapDraw(Map* map, vec3i* camPos);

u8* mapScriptRaycast(Map* map, Ray* ray);
u32 mapClipX(Map* map, vec3i* pos, vec3s* speed, vec3s* size);
u32 mapClipZ(Map* map, vec3i* pos, vec3s* speed, vec3s* size);

#endif