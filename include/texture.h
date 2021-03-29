#ifndef GAME_TEX_H
#define GAME_TEX_H

typedef struct {
	int id;
	u16 w, h;
} Texture;

Texture* textureNew();
void textureLoad(Texture* tex, char* path);
void textureDestroy(Texture* tex);
#endif