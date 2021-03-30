#include <nds.h>
#include <maxmod9.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "map.h"
#include "assets.h"

void mapInit(Map* map) {
	map->musicID = 0;
	
	map->skyModel = NULL;
	
	map->file.data = NULL;
	
	map->texs = NULL;
	map->models = NULL;
}

Map* mapNew() {
	Map* map = malloc(sizeof(Map));
	
	if(map != NULL) mapInit(map);
	else fprintf(stderr, "Can't create map\n");
	
	return map;
}

void mapUnload(Map* map) {
	if(map == NULL) return;
	
	if(map->file.data) {
		free(map->file.data);
		map->file.data = NULL;
		
		u32 texsCount = map->file.texsCount;
		for(u32 i=0; i<texsCount; i++) {
			textureDestroy(*(map->texs + i));
		}
		
		u32 modelsCount = map->file.modelsCount;
		for(u32 i=0; i<modelsCount; i++) {
			modelDestroyStruct(*(map->models + i));
		}
	
		free(map->texs);
		free(map->models);
		map->texs = NULL;
		map->models = NULL;
	}
}

void mapDestroy(Map* map) {
	if(map == NULL) return;
	
	mapUnload(map);
	if(map->musicID) {
		if(mmActive()) mmStop();
		mmUnload(map->musicID - 1);
	}
	if(map->skyModel) modelDestroy(map->skyModel);
	
	free(map);
}

void mapLoadSky(Map* map) {
	map->skyModel = modelNew();
	modelLoad(map->skyModel, "nitro:/sky.dsmd");
}

void mapLoad(Map* map, char* path) {
	if(map == NULL) return;
	glFlush(0); //We don't need to see weird artifacts when textures are changing
	
	mapUnload(map);
	
	char fullPath[17 + 32] = "nitro:/maps/";
	strcat(fullPath, path);
	strcat(fullPath, ".dsmp");
	
	u32* file32 = (u32*) loadFile(fullPath, NULL);
	
	if(file32) {
		MapFile* mapFile = &(map->file);
		
		mapFile->data = (u8*) file32;
		mapFile->flags = *file32;
		
		file32++;
		
		//Load skybox
		if(mapFile->flags & SKY_FLAG) {
			if(map->skyModel == NULL) mapLoadSky(map);
			
			mapFile->skyTexture = *file32;
			
			file32++;
		}
		
		//Play music
		if(mapFile->flags & MUSIC_FLAG) {
			u32 musicID = *file32;
			file32++;
			
			if(map->musicID - 1 != musicID) {
				if(map->musicID) {
					//idk why but maxmod just breaks after this function;
					//https://devkitpro.org/viewtopic.php?f=29&t=281 bug in maxmod
					if(mmActive()) mmStop();
					mmUnload(map->musicID - 1);
				}
				mmLoad(musicID);
				mmStart(musicID, MM_PLAY_LOOP);
			
				map->musicID = musicID + 1;
				
			} else if(mapFile->flags & MUSIC_STOP_FLAG) {
				if(mmActive()) mmStop();
				mmStart(musicID, MM_PLAY_LOOP);
			}
			
		} else if(mapFile->flags & MUSIC_STOP_FLAG && map->musicID) {
			if(mmActive()) mmStop();
			mmUnload(map->musicID - 1);
			map->musicID = 0;
		}
		
		//Load bounding boxes
		u32 boxesCount = *file32;
		mapFile->boxesCount = boxesCount;
		mapFile->boxes = (vec3i*) (file32 + 1);
		
		file32 += (boxesCount * 2 * sizeof(vec3i) / 4) + 1;
		
		//Load script boxes
		u32 scriptsCount = *file32;
		mapFile->scriptsCount = scriptsCount;
		mapFile->scriptBoxes = (vec3i*) (file32 + 1);
		
		file32 += (scriptsCount * 2 * sizeof(vec3i) / 4) + 1;
		
		//Script flags
		mapFile->scriptFlags = file32;
		file32 += scriptsCount;
		
		//Skip scripts...
		mapFile->scripts = file32;
		for(u32 i=0; i<scriptsCount; i++) {
			file32 += 1 + *(file32);
		}
		
		//Load textures
		u32 texsCount = *file32;
		mapFile->texsCount = texsCount;
		mapFile->texsNames = (char*) (file32 + 1);
		
		map->texs = calloc(texsCount, sizeof(Texture*));
		for(u32 i=0; i<texsCount; i++) {
			Texture* tex = textureNew();
			
			char texName[20 + 32] = "nitro:/textures/";
			strcat(texName, (char*) (mapFile->texsNames + 32 * i)  );
			strcat(texName, ".pcx");
			
			textureLoad(tex, texName);
			
			*(map->texs + i) = tex;
		}
		
		file32 += texsCount * 8 + 1;
		
		//Load models
		u32 modelsCount = *file32;
		mapFile->modelsCount = modelsCount;
		mapFile->modelsTexs = file32 + 1;
		
		file32 += modelsCount + 1;
		
		map->models = calloc(modelsCount, sizeof(Model*));
		for(u32 i=0; i<modelsCount; i++) {
			u32 modelSize = *(file32);
			
			Model* model = modelNew();
			modelInitData(model, (u8*) (file32 + 1) );
			*(map->models + i) = model;
			
			file32 += modelSize + 1;
		}
		
	} else fprintf(stderr, "Can't load map\n");
}

void mapDraw(Map* map, vec3i* camPos) {
	if(map == NULL || map->file.data == NULL) return;
	
	if(map->file.flags & SKY_FLAG) {
		Texture* skyTex = *(map->texs + map->file.skyTexture);
		
		glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK | POLY_ID(63));
		glBindTexture(0, skyTex->id);
		
		modelDraw(map->skyModel, camPos->x, camPos->y, camPos->z, 0, skyTex->w, skyTex->h);
		
		glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
	}
	
	u32 modelsCount = map->file.modelsCount;
	for(u32 i=0; i<modelsCount; i++) {
		Model* model = *(map->models + i);
		Texture* tex = *(map->texs + *(map->file.modelsTexs + i) );
		
		glBindTexture(0, tex->id);
		modelDraw(model, 0, 0, 0, 0, tex->w, tex->h);
	}
	
	glBindTexture(0, 0);;
}

//Raycasting for scripts so they were activating
u8* mapScriptRaycast(Map* map, Ray* ray) {
	if(map == NULL || map->file.data == NULL) return NULL;
	
	u32 boxesCount = map->file.scriptsCount;
	vec3i* boxes = map->file.scriptBoxes;
	
	u32* hitScript = NULL;
	u32* script = map->file.scripts;
	
	for(u32 i=0; i<boxesCount; i++) {
		if(rayCast(ray, i, boxes, boxes + 1)) {
			hitScript = script + 1;
		}
		
		boxes += 2;
		script += *(script) + 1;
	}
	
	boxesCount = map->file.boxesCount;
	boxes = map->file.boxes;
	
	for(u32 i=0; i<boxesCount; i++) {
		if(rayCast(ray, i, boxes, boxes + 1)) return NULL;
		boxes += 2;
	}
	
	return (u8*) hitScript;
}

//Collision code
u32 mapClipX(Map* map, vec3i* pos, vec3s* speed, vec3s* size) {
	if(map == NULL || map->file.data == NULL || speed->x == 0) return speed->x;
	
	s16 output = speed->x;
	
	vec3i minPos = (vec3i) {pos->x - (size->x >> 1), pos->y, pos->z - (size->z >> 1)},
		maxPos = (vec3i) {pos->x + (size->x >> 1), pos->y + size->y, pos->z + (size->z >> 1)};
	
	u32 boxesCount = map->file.boxesCount;
	vec3i* boxes = map->file.boxes;
	
	for(u32 i=0; i<boxesCount; i++) {
		vec3i min = *(boxes), max = *(boxes + 1);
		
		if(minPos.y < max.y && maxPos.y > min.y &&
			minPos.z < max.z && maxPos.z > min.z) {
			
			if(output > 0 && maxPos.x <= min.x) {
				s32 c = min.x - maxPos.x;
				if(c < output) output = c;
			} 
			if(output < 0 && minPos.x >= max.x) {
				s32 c = max.x - minPos.x;
				if(c > output) output = c;
			}
		}
		
		boxes += 2;
	}
	
	return output;
}

u32 mapClipZ(Map* map, vec3i* pos, vec3s* speed, vec3s* size) {
	if(map == NULL || map->file.data == NULL || speed->z == 0) return speed->z;
	
	s16 output = speed->z;
	
	vec3i minPos = (vec3i) {pos->x - (size->x >> 1), pos->y, pos->z - (size->z >> 1)},
		maxPos = (vec3i) {pos->x + (size->x >> 1), pos->y + size->y, pos->z + (size->z >> 1)};
	
	u32 boxesCount = map->file.boxesCount;
	vec3i* boxes = map->file.boxes;
	
	for(u32 i=0; i<boxesCount; i++) {
		vec3i min = *(boxes), max = *(boxes + 1);
		
		if(minPos.x < max.x && maxPos.x > min.x && 
			minPos.y < max.y && maxPos.y > min.y) {
			
			if(output > 0 && maxPos.z <= min.z) {
				s32 c = min.z - maxPos.z;
				if(c < output) output = c;
			} 
			if(output < 0 && minPos.z >= max.z) {
				s32 c = max.z - minPos.z;
				if(c > output) output = c;
			}
		}
		
		boxes += 2;
	}
	
	return output;
}