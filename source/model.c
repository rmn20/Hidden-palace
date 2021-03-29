#include <nds.h>

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "model.h"
#include "assets.h"

Model* modelNew() {
	Model* mdl = malloc(sizeof(Model));
	if(mdl) mdl->data = (u8*) NULL;
	else fprintf(stderr, "Can't create model");
	return mdl;
}

void modelInitData(Model* mdl, u8* data) {
	if(mdl == NULL) return;
	
	u8* ptr = data;
	
	mdl->data = ptr;
	mdl->flags = *ptr;
	ptr += 4;
	
	mdl->min = (float32*) ptr;
	ptr += 4*3;
	
	mdl->max = (float32*) ptr;
	ptr += 4*3;
	
	//Offset and scale
	ptr += 4*3*2;
	
	if(mdl->flags & MDL_POLYDATA_FLAG) {
		mdl->renderData = (u16*) ptr;
		u16 vertSize = ((mdl->flags & MDL_COLOR_FLAG)?2:4) + 4 + 6;
		u16 pol4c = *(mdl->renderData);
		u16 pol3c = *(mdl->renderData + 1);
		u16 polsSize = pol4c*vertSize*4 + pol3c*vertSize*3;
		
		ptr += 4 +  polsSize;
		if(polsSize & 3) ptr += 4 - (polsSize & 3);
	}
	
	if(mdl->flags & MDL_CMDLIST_FLAG) mdl->list = (u32*) ptr;
}

void modelLoad(Model* mdl, char* path) {
	if(mdl == NULL) return;
	u8* file = loadFile(path, NULL);
	
	if(file) modelInitData(mdl, file);
	else fprintf(stderr, "Can't load model\n");
}

void modelDestroy(Model* mdl) {
	if(mdl == NULL) return;
	
	if(mdl->data) free(mdl->data);
	modelDestroyStruct(mdl);
}

void modelDestroyStruct(Model* mdl) {
	free(mdl);
}

void initMatrices(Model* mdl, s32 x, s32 y, s32 z, s16 rotY, u16 texW, u16 texH) {
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	s32* offsetp = (s32*) (mdl->max + 3);
	
	glTranslatef32(x, y, z);
	glRotateYi(rotY);
	glTranslatef32(*(offsetp), *(offsetp+1), *(offsetp+2));
	glScalef32(*(offsetp+3), *(offsetp+4), *(offsetp+5));
	glTranslatef32(32768, 32768, 32768);
	
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glScalef32((4096 * texW) >> 10, (4096 * texH) >> 10, 0);
}

void modelDraw(Model* mdl, s32 x, s32 y, s32 z, s16 rotY, u16 texW, u16 texH) {
	if(mdl == NULL || mdl->data == NULL || !(mdl->flags & MDL_CMDLIST_FLAG)) return;
	
	initMatrices(mdl, x, y, z, rotY, texW, texH);
	
	glCallList(mdl->list);
	
	glPopMatrix(1);
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix(1);
}

void drawPols(u16 verts, u16 frame, u16* data1, u16* data2, u16 vertSize, bool hasColor);

void modelDrawMorphed(Model* mdl1, Model* mdl2, u16 frame, s32 x, s32 y, s32 z, s16 rotY, u16 texW, u16 texH) {
	if(mdl1 == NULL || mdl2 == NULL || mdl1->data == NULL || mdl2->data == NULL) return;
	if(!(mdl1->flags & MDL_POLYDATA_FLAG) || !(mdl2->flags & MDL_POLYDATA_FLAG)) return;
	
	bool hasColor = mdl1->flags & MDL_COLOR_FLAG;
	u16 vertSize = (hasColor?1:2) + 2 + 3;
	
	u16 pol4c = *(mdl1->renderData);
	u16 pol3c = *(mdl1->renderData + 1);
	u16 datap = 2;
	
	initMatrices(mdl1, x, y, z, rotY, texW, texH);
	
	if(pol4c) {
		glBegin(GL_QUADS);
		
		drawPols(pol4c * 4, frame, mdl1->renderData + datap, mdl2->renderData + datap, vertSize, hasColor);
		glEnd();
		datap += pol4c * vertSize * 4;
	}
	
	if(pol3c) {
		glBegin(GL_TRIANGLES);
		
		drawPols(pol3c * 3, frame, mdl1->renderData + datap, mdl2->renderData + datap, vertSize, hasColor);
		glEnd();
	}
	
	glPopMatrix(1);
	
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix(1);
}

void drawPols(u16 verts, u16 frame, u16* data1, u16* data2, u16 vertSize, bool hasColor) {
	vertSize -= (hasColor?1:2);
	u16 invFrame = 1024 - frame;
	u16 frame5 =  frame >> 5;
	u16 invFrame5 =  32 - frame5;
	
	for(u16 i=0; i<verts; i++) {
		
		if(!hasColor) {
			u32 n1 = *data1 | ((*data1 + 1)<<16);
			u32 n2 = *data2 | ((*data2 + 1)<<16);
			
			u32 n = (((n1 & 0x3ff003ff) * invFrame + (n2 & 0x3ff003ff) * frame) >> 10) & 0x3ff003ff;
			n |= (((n1 & 0x000ffc00) * invFrame + (n2 & 0x000ffc00) * frame) >> 10) & 0x000ffc00;
			
			glNormal(n);
			data1 += 2;
			data2 += 2;
		} else {
			u16 c1 = *data1;
			u16 c2 = *data2;
			
			u16 c = (((c1 & 0x7c1f) * invFrame5 + (c2 & 0x7c1f) * frame5) >> 5) & 0x7c1f;
			c |= (((c1 & 0x03e0) * invFrame5 + (c2 & 0x03e0) * frame5) >> 5) & 0x03e0;
			
			glColor(c);
			data1 += 1;
			data2 += 1;
		}
		
		glTexCoord2t16( 
			((s16) *data1 * invFrame + (s16) *data2 * frame) >> 10, 
			((s16) *(data1 + 1) * invFrame + (s16) *(data2 + 1) * frame) >> 10 
			);
		
		glVertex3v16(
			((s16) *(data1 + 2) * invFrame + (s16) *(data2 + 2) * frame) >> 10, 
			((s16) *(data1 + 3) * invFrame + (s16) *(data2 + 3) * frame) >> 10, 
			((s16) *(data1 + 4) * invFrame + (s16) *(data2 + 4) * frame) >> 10
			);
		data1 += vertSize;
		data2 += vertSize;
	}
}