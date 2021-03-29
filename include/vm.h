#include <nds/ndstypes.h>

typedef struct {
	u16 stackSize;
	u16 stackPos;
	
	u8* stack;
} VM;


typedef enum OPCODE {
	OP_NOP, OP_END,
	OP_FUNC,
	OP_PUSH, OP_PUSH2, OP_PUSH4, OP_PUSHN
} OPCODE;

typedef enum FUNC {
	FN_MAP, FN_PLAYERPOS, FN_PLAYERROT
} FUNC;

VM* vmNew(u16 stackSize);

void vmRun(VM* vm, u8* bytecode);