#include <nds/ndstypes.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "vm.h"
#include "game.h"
#include "map.h"
#include "vec.h"

//why i spend time on writing this instead of making maps
//move to struct maybe
extern vec3i playerPos;
extern s16 playerRotateX, playerRotateY;
extern s16 renderRotateX, renderRotateY;

extern Map* map;

void vmInit(VM* vm, u16 stackSize) {
	vm->stack = malloc(stackSize);
	vm->stackSize = stackSize;
	vm->stackPos = stackSize;
}

VM* vmNew(u16 stackSize) {
	VM* vm = malloc(sizeof(VM));
	
	if(!vm) fprintf(stderr, "Can't allocate VM!\n");
	else vmInit(vm, stackSize);
	
	return vm;
}

bool vmStackPush(VM* vm, void* push, u16 size) {
	if(vm->stackPos - size < 0) {
		fprintf(stderr, "Can't push %d bytes to vm stack!\n", size);
		return false;
	}
	
	vm->stackPos -= size;
	memcpy(vm->stack + vm->stackPos, push, size);
	return true;
}

u8* vmStackPull(VM* vm, u16 size) {
	if(vm->stackPos + size > vm->stackSize) {
		fprintf(stderr, "Can't pull %d bytes from vm stack! Out of stack.\n", size);
		return NULL;
	}
	
	u8* pos = vm->stack + vm->stackPos;
	vm->stackPos += size;
	return pos;
}

char* vmStackPullString(VM* vm, u16* size) {
	char* pull = (char*) (vm->stack + vm->stackPos);
	if(size) *size = 0;
	
	//todo rewrite
	while(1) {
		if(vm->stackPos == vm->stackSize) {
			fprintf(stderr, "Can't pull string from vm stack! Out of stack.\n");
			break;
		}
		
		if(*(vm->stack + vm->stackPos) == '\0') {
			vm->stackPos++;
			break;
		} else if(size) (*size)++;
		
		vm->stackPos++;
	}
	
	return pull;
}

u16 vmStackPull2(VM* vm) {
	return *vmStackPull(vm, 1) | (*vmStackPull(vm, 1) << 8);
}

u32 vmStackPull4(VM* vm) {
	return vmStackPull2(vm) | (vmStackPull2(vm) << 16);
}	

u16 vmCallFunc(VM* vm, u8* bytecode, u16 pos) {
	FUNC func = *(bytecode + pos);
	pos++;
	
	switch(func) {
		case FN_MAP:
		{
			u8 flags = *vmStackPull(vm, 1);
			gamePlayDoorSound();
			char* map = vmStackPullString(vm, NULL);
			
			vec3i newPos = (vec3i) {0, 0, 0};
			if(flags & 1) {
				newPos.x = vmStackPull4(vm);
				newPos.y = vmStackPull4(vm);
				newPos.z = vmStackPull4(vm);
			}
			
			s16 newRotX = 0, newRotY = 0;
			if(flags & 2) {
				newRotX = vmStackPull2(vm);
				newRotY = vmStackPull2(vm);
			}
			
			gameQueueMap(map, (flags & 1) ? &newPos : NULL, (flags & 2) ? &newRotX : NULL, (flags & 2) ? &newRotY : NULL);
		}
			break;
		case FN_PLAYERPOS:
			playerPos.x = vmStackPull4(vm);
			playerPos.y = vmStackPull4(vm);
			playerPos.z = vmStackPull4(vm);
			break;
		case FN_PLAYERROT:
			playerRotateX = renderRotateX = vmStackPull2(vm);
			playerRotateY = renderRotateY = vmStackPull2(vm);
			break;
	}
	
	return pos;
}

void vmRun(VM* vm, u8* bytecode) {
	
	u16 pos = 0;
	while(1) {
		OPCODE opcode = *(bytecode + pos);
		
		switch (opcode) {
			case OP_NOP:
				break;
			case OP_END:
				return;
			case OP_PUSH:
				pos++;
				vmStackPush(vm, bytecode + pos, 1);
				break;
			case OP_PUSH2:
				vmStackPush(vm, bytecode + pos + 1, 2);
				pos += 2;
				break;
			case OP_PUSH4:
				vmStackPush(vm, bytecode + pos + 1, 4);
				pos += 4;
				break;
			case OP_PUSHN:
			{
				u8 push = *(bytecode + pos + 1);
				vmStackPush(vm, bytecode + pos + 2, push);
				pos += 2 + push - 1;
			}
				break;
			case OP_FUNC:
				pos = vmCallFunc(vm, bytecode, pos + 1) - 1;
				break;
		}
		
		pos++;
	}
}