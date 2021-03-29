#ifndef GAME_MDL_H
#define GAME_MDL_H

#define MDL_COLOR_FLAG 1
#define MDL_POLYDATA_FLAG 2
#define MDL_CMDLIST_FLAG 4

typedef struct {
	u8* data;
	u32 flags;
	
	float32* min;
	float32* max;
	
	u16* renderData;
	u32* list;
} Model;


Model* modelNew();
void modelInitData(Model* mdl, u8* data);
void modelLoad(Model* mdl, char* path);
void modelDestroy(Model* mdl);
void modelDestroyStruct(Model* mdl);
void modelDraw(Model* mdl, s32 x, s32 y, s32 z, s16 rotY, u16 texW, u16 texH);
void modelDrawMorphed(Model* mdl1, Model* mdl2, u16 frame, s32 x, s32 y, s32 z, s16 rotY, u16 texW, u16 texH);

#endif