#include <nds.h>
#include <maxmod9.h>

#include <stdio.h>
#include <stdlib.h>

#include "soundbank.h"

#include "game.h"
#include "map.h"
#include "assets.h"
#include "vm.h"

const u16 WALK_BUTTONS = KEY_DOWN | KEY_UP | KEY_LEFT | KEY_RIGHT;

//todo move to struct??
u16 textBG;
u16* textMap;
u16* handGfx;

touchPosition touchXY, prevXY, touchPressXY;

vec3i playerPos;
vec3s playerLookDir;
s16 playerRotateX, playerRotateY;
s16 renderRotateX, renderRotateY;

u8 lastWalkPressTime;
u16 lastWalkButtons;
bool run;
u8 currentStep;
u16 stepDistX, stepDistZ;

vec3i newMapPlayerPos;
s16 newMapPlayerRotX, newMapPlayerRotY;
char pendingMap[32];
VM* vm;
Map* map;

Ray ray;

void gameInit() {
	//Backgrounds init
	bgSetPriority(0, 1);
	textBG = bgInit(3, BgType_Text4bpp, BgSize_T_256x256, 0, 1);
	
	u8* defaultFont = loadFile("nitro:/font.img.bin", NULL);
	dmaCopy(defaultFont, bgGetGfxPtr(textBG), 128*8*8 / 2);
	free(defaultFont);
	BG_PALETTE[1] = RGB15(31, 31, 31);
	textMap = bgGetMapPtr(textBG);
	
	//Bottom screen
	oamInit(&oamSub, SpriteMapping_Bmp_1D_128, false);
	oamEnable(&oamSub);
	handGfx = oamAllocateGfx(&oamSub, SpriteSize_32x32, SpriteColorFormat_16Color);
	
	u8* hand = loadFile("nitro:/hand.img.bin", NULL);
	dmaCopy(hand, handGfx, 32 * 32 / 2);
	free(hand);
	SPRITE_PALETTE_SUB[1] = RGB15(31, 31, 31);
	
	oamSet(&oamSub, 0, 112, 0, 
			0, 0, SpriteSize_32x32, SpriteColorFormat_16Color, handGfx,
			-1, false, false, false, false, false);
	oamSetHidden(&oamSub, 0, true); //libnds bug?
	
	//3D
	glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
	//any floating point gl call is being converted to fixed prior to being implemented
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(70, 256.0 / 192.0, 0.1, 500);
	
	glMaterialf(GL_EMISSION, RGB15(31, 31, 31));
	glMaterialShinyness();
	
	//Sound
	mmLoadEffect(SFX_DOOR);
	for(u16 i=0; i<3; i++) {
		mmLoadEffect(SFX_STEP1 + i);
	}
	lastWalkPressTime = 255;
	lastWalkButtons = 0;
	currentStep = 0;
	stepDistX = stepDistZ = 0;
	
	//Map
	playerPos = (vec3i) {86016, 0, 73728};
	playerRotateX = renderRotateX = 0;
	playerRotateY = renderRotateY = -8192;
	
	*pendingMap = 0;
	map = mapNew();
	vm = vmNew(1024); //1kb stack
	gameQueueMap("house", NULL, NULL, NULL);
}

void gameDestroy() {
	oamFreeGfx(&oamSub, handGfx);
	oamClearSprite(&oamSub, 0);
	
	mapDestroy(map);
	
	mmUnloadEffect(SFX_STEP1);
	mmUnloadEffect(SFX_STEP2);
	mmUnloadEffect(SFX_STEP3);
	mmUnloadEffect(SFX_DOOR);
}

void gamePlayDoorSound() {
	mmEffect(SFX_DOOR);
}

void gameQueueMap(char* map, vec3i* newPlayerPos, s16* newRotX, s16* newRotY) {
	if(newPlayerPos) {
		memcpy(&newMapPlayerPos, newPlayerPos, sizeof(vec3i));
	} else {
		newMapPlayerPos.x = playerPos.x;
		newMapPlayerPos.y = playerPos.y;
		newMapPlayerPos.z = playerPos.z;
	}
	
	if(newRotX) newMapPlayerRotY = *newRotX;
	else newMapPlayerRotX = playerRotateX;
	
	if(newRotY) newMapPlayerRotY = *newRotY;
	else newMapPlayerRotY = playerRotateY;
	
	strncpy(pendingMap, map, 31);
}

void gameLoadMap() {
	// BG must be opaque for AA to work
	//glClearColor(1, 3, 5, 31);
	glClearColor(0, 0, 0, 31);
	mapLoad(map, pendingMap);
	*pendingMap = 0;
	
	memcpy(&playerPos, &newMapPlayerPos, sizeof(vec3i));
	playerRotateX = renderRotateX = newMapPlayerRotX;
	playerRotateY = renderRotateY = newMapPlayerRotY;
}

void gameTick(u16 held, u16 press, u16 release) {
	if(*pendingMap) gameLoadMap();
	
	if(press & KEY_TOUCH) {
		touchRead(&prevXY);
		touchRead(&touchPressXY);
	} else if(held & KEY_TOUCH) {
		touchRead(&touchXY);
		
		s16 dx = prevXY.rawx - touchXY.rawx;
		s16 dy = prevXY.rawy - touchXY.rawy;
		
		const s16 noiseLevel = 4096 * 1 / 256;
		
		if(abs(dx) < noiseLevel) {
			touchXY.rawx = prevXY.rawx;
			dx = 0;
		}
		if(abs(dy) < noiseLevel) {
			touchXY.rawy = prevXY.rawy;
			dy = 0;
		}
		
		playerRotateX += dy * degreesToAngle(70*2) / 3072;
		playerRotateY += dx * degreesToAngle(360) / 4096;
		
		prevXY = touchXY;
	}
	
	//Limit camera x rotation
	const s16 rotYLimit = degreesToAngle(85);
	if(playerRotateX < -rotYLimit) playerRotateX = -rotYLimit;
	if(playerRotateX > rotYLimit) playerRotateX = rotYLimit;
	
	//Smooth camera movement
	if(renderRotateY < -16384 && playerRotateY > 16384) renderRotateY *= -1;
	if(renderRotateY > 16384 && playerRotateY < -16384) renderRotateY *= -1;
	renderRotateX = (renderRotateX + playerRotateX) >> 1;
	renderRotateY = (renderRotateY + playerRotateY) >> 1;
	
	//Run handling
	u16 pressedWalkButtons = press & WALK_BUTTONS;
	if(pressedWalkButtons) {
		if((pressedWalkButtons == lastWalkButtons) && (lastWalkPressTime <= 15)) {
			run = true;
			lastWalkPressTime = 255;
		} else {
			lastWalkPressTime = 0;
			lastWalkButtons = pressedWalkButtons;
		}
	}
	if(run && (release & WALK_BUTTONS) && !(held & WALK_BUTTONS)) {
		run = false;
	}
	
	if(lastWalkPressTime < 255) lastWalkPressTime++;
	
	int movX = 0, movZ = 0;
	if(held & KEY_RIGHT) movX += 1;
	else if(held & KEY_LEFT) movX -= 1;
	if(held & KEY_UP) movZ += 1;
	else if(held & KEY_DOWN) movZ -= 1;
	
	if(movX || movZ) {
		s16 walkDir = playerRotateY;
		
		if(movZ) {
			if(movZ == -1) walkDir +=  16384;
			walkDir -= movX*movZ * 4096;
		} else walkDir -= movX * 8192;
		
		s16 sin = sinLerp(walkDir);
		s16 cos = cosLerp(walkDir);
		
		u16 walkSpeed = run ? 250 : 150;
		vec3s speed = (vec3s) {(sin * walkSpeed) >> 12, 0, (cos * walkSpeed) >> 12};
		//50, 180, 50
		vec3s size = (vec3s) {2048, 7373, 2048};
		
		speed.x = mapClipX(map, &playerPos, &speed, &size);
		speed.z = mapClipZ(map, &playerPos, &speed, &size);
		
		playerPos.x += speed.x;
		playerPos.z += speed.z;
		
		stepDistX += abs(speed.x);
		stepDistZ += abs(speed.z);
		//79 sm * 4096 / 100 sm ~= 3236
		if(stepDistX*stepDistX + stepDistZ*stepDistZ >= 3236*2 * 3236*2) {
			stepDistX = stepDistZ = 0;
			mmEffect(SFX_STEP1 + currentStep);
			
			if(currentStep == 2) currentStep = 0;
			else currentStep++;
		}
	}
	
	//Are we sure this is right place for script activation?
	ray.pos.x = playerPos.x;
	ray.pos.y = playerPos.y + 6963;
	ray.pos.z = playerPos.z;
	ray.dir.x = playerLookDir.x;
	ray.dir.y = playerLookDir.y;
	ray.dir.z = playerLookDir.z;
	ray.maxDistance = 6144;
	rayReset(&ray);
	u8* script = mapScriptRaycast(map, &ray);
	oamSetHidden(&oamSub, 0, !script);
	
	if(script && (release & KEY_TOUCH) && 
			(touchPressXY.px >= 112 && touchPressXY.px < 144 && touchPressXY.py < 32) &&
			(touchXY.px >= 112 && touchXY.px < 144 && touchXY.py < 32)
			) {
		vmRun(vm, script);
	}
}

void gameRender() {
	glMatrixMode(GL_POSITION);
	glLoadIdentity();
	
	{
		playerLookDir.x = sinLerp(renderRotateY);
		playerLookDir.z = cosLerp(renderRotateY);
		playerLookDir.y = sinLerp(renderRotateX);
		
		s16 mul = cosLerp(renderRotateX);
		playerLookDir.x = mulf32(playerLookDir.x, mul);
		playerLookDir.z = mulf32(playerLookDir.z, mul);
		
		glLoadIdentity();
		//1.7 * 4096 ~= 6963
		gluLookAtf32(playerPos.x, playerPos.y + 6963, playerPos.z,
					playerPos.x+playerLookDir.x, playerPos.y+playerLookDir.y + 6963, playerPos.z+playerLookDir.z,
					0, 4096, 0);
	}
				
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	
	s32 oldY = playerPos.y;
	playerPos.y += 6963;
	mapDraw(map, &playerPos);
	playerPos.y = oldY;
	
	glPopMatrix(1);
		
	/*glMatrixMode(GL_POSITION);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity(); 
	glOrthof32(0, 256, 192, 0, 0, 500);
	
	glBegin(GL_QUADS);
	glColor(0);
	glVertex3v16(0, 144, 0);
	glVertex3v16(256, 144, 0);
	glVertex3v16(256, 192, 0);
	glVertex3v16(0, 192, 0);
	glEnd();
	
	glPopMatrix(1);
	glMatrixMode(GL_POSITION);
	glPopMatrix(1);*/
	
}