#include <nds.h>
#include <filesystem.h>
#include <maxmod9.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "soundbank.h"

#include "game.h"
#include "assets.h"

//Todo make game struct???
extern u16* textMap;

void clearText() {
	memset(textMap, 0, 32*24*2);
}

void showText(char* path) {
	clearText();
	u32 fSize;
	u8* text = loadText(path, &fSize);
	
	u8 lines = 0;
	u8 linesLength[24];
	memset(linesLength, 0, 24);
	
	for(u32 i=0; i<fSize;) {
		u8* find = memchr(text + i, '\n', fSize - i);
		
		if(find) {
			linesLength[lines] = find - text - i;
			i = find - text + 1;
		} else {
			linesLength[lines] = fSize - i;
			i = fSize;
		}
		
		lines++;
	}
	
	u16 drawY = (24 - lines) >> 1;
	
	u16 textPos = 0;
	for(u8 line = 0; line<lines; line++, drawY++) {
		u8 lineLength = linesLength[line];
		
		u16 drawPos = (drawY << 5) + ((32 - lineLength) >> 1);
		
		for(u8 i=0; i < lineLength; i++, drawPos++) {
			textMap[drawPos] = text[textPos + i];
		}
		
		textPos += lineLength + 1;
	}
	
	free(text);
}

int main() {	
	if(!nitroFSInit(NULL)) return 1;
	
	//set mode 0, enable BG0 and set it to 3D
	videoSetMode(MODE_0_3D);
	videoSetModeSub(MODE_0_2D);
	
	vramSetBankA(VRAM_A_TEXTURE_SLOT0);
	vramSetBankB(VRAM_B_TEXTURE_SLOT1);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);
	vramSetBankD(VRAM_D_TEXTURE_SLOT2);
	vramSetBankE(VRAM_E_MAIN_BG);
	vramSetBankF(VRAM_F_TEX_PALETTE_SLOT0);
	vramSetBankG(VRAM_G_TEX_PALETTE_SLOT1);
	vramSetBankH(VRAM_H_LCD);
	vramSetBankI(VRAM_I_SUB_SPRITE);
	
	u16 bgText = bgInitSub(3, BgType_Text4bpp, BgSize_T_256x256, 0, 1);
	bgSetPriority(bgText, 1);
	consoleInit(NULL, 3, BgType_Text4bpp, BgSize_T_256x256, bgGetMapBase(bgText), bgGetTileBase(bgText), false, true);
	/*consoleDebugInit(DebugDevice_NOCASH);
	stdout = stderr;*/
	
	glInit();
	//enable textures
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ANTIALIAS);
	
	// setup the rear plane
	glClearPolyID(63); // BG must have a unique polygon ID for AA to 
	glClearColor(0, 0, 0, 31);
	glClearDepth(0x7FFF);

	//this should work the same as the normal gl call
	glViewport(0,0,255,191);
	
	mmInitDefaultMem((mm_addr) loadFile("nitro:/soundbank.bin", NULL));
	mmSelectMode(MM_MODE_B); //Interpolated 16ch
	
	gameInit();
	
	{
		mmLoad(2);
		mmStart(2, MM_PLAY_LOOP);
		char path[] = "nitro:/text/text0.txt";
		u8 state = 0;
		showText(path);
		
		while(1) {
			scanKeys();
			if(keysDown()) {
				state++;
				if(state == 5) break;
				else {
					path[16] = state + '0';
					showText(path);
				}
			}
			
			swiWaitForVBlank();
		}
		
		clearText();
		mmStop();
		mmUnload(2);
	}
	
	while(1) {
		//Update
		scanKeys();
		u16 held = keysHeld();
		u16 press = keysDown();
		u16 release = keysUp();
		gameTick(held, press, release);
		
		//Draw
		gameRender();
		
		glFlush(0);
		swiWaitForVBlank();
		//oamUpdate(&oamMain);
		oamUpdate(&oamSub);

		if(held & KEY_START) break;
	}
	
	mmEffectCancelAll();
	gameDestroy();

	return 0;
}