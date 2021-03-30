#include <nds.h>

#include <stdio.h>
#include <stdlib.h>

#include "texture.h"
#include "assets.h"

int getFormat(sImage* img);
int getSize(int size);

Texture* textureNew() {
	Texture* tex = malloc(sizeof(Texture));
	if(tex == NULL) fprintf(stderr, "Can't allocate texture struct\n");
	return tex;
}

void textureLoad(Texture* tex, char* path) {
	u8* file = loadFile(path, NULL);
	
	if(file != NULL) {
		sImage img;
		loadPCX(file, &img);
		free(file);
		
		if(img.bpp == 24) image24to16(&img);
		
		u16 w = getSize(img.width);
		u16 h = getSize(img.height);
		
		if(w != -1 && h != -1) {
			int textureID = 0;
			glGenTextures(1, &textureID);
			glBindTexture(0, textureID);
			
			u32 alpha = 0;
			
			if(img.palette != NULL) {
				glColorTableEXT(0, 0, 1 << img.bpp, 0, 0, img.palette);
				if(*((u16*) img.palette) == RGB15(0, 31, 0)) alpha |= GL_TEXTURE_COLOR0_TRANSPARENT;
			}
			
			DC_FlushRange(img.image.data8, img.width * img.height * (img.bpp >> 3)); //Why?
			glTexImage2D(0, 0, getFormat(&img), w, h, 0, TEXGEN_TEXCOORD | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | alpha, img.image.data8);
			
			glBindTexture(0, 0);
			
			tex->id = textureID;
			tex->w = img.width;
			tex->h = img.height;
		} else {
			fprintf(stderr, "%s Unsupported image size!\n", path);
		}
			
		imageDestroy(&img);
	} else fprintf(stderr, "Can't load image\n");
}

int getFormat(sImage* img) {
	if(img->bpp == 16) return GL_RGBA;
	else if(img->bpp == 8) return GL_RGB256;
	else return GL_RGB16;
}

int getSize(int size) {
	if(size == 1024) return TEXTURE_SIZE_1024;
	else if(size == 512) return TEXTURE_SIZE_512;
	else if(size == 256) return TEXTURE_SIZE_256;
	else if(size == 128) return TEXTURE_SIZE_128;
	else if(size == 64) return TEXTURE_SIZE_64;
	else if(size == 32) return TEXTURE_SIZE_32;
	else if(size == 16) return TEXTURE_SIZE_16;
	else if(size == 8) return TEXTURE_SIZE_8;
	else return -1;
}

void textureDestroy(Texture* tex) {
	if(tex == NULL) return;
	
	glDeleteTextures(1, &(tex->id));
	free(tex);
}