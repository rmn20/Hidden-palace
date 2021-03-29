#include <nds.h>

#include <stdio.h>
#include <stdlib.h>

u8* loadImpl(char* file, char* type, u32* size) {
	FILE* inf = fopen(file, "rb");
	
	if(inf) {
		fseek(inf, 0, SEEK_END);
		u32 len = ftell(inf);
		if(size) *size = len;
		fseek(inf, 0, SEEK_SET);

		char* entireFile = (char*) malloc(len);
		
		u32 read = fread(entireFile, 1, len, inf);
		if(read != len) {
			fprintf(stderr, "Can't read file %s\n", file);
			free(entireFile);
			fclose(inf);
			
			return NULL;
		}

		fclose(inf);
		return (u8*) entireFile;
	} else fprintf(stderr, "Can't open file %s\n", file);
	
	return NULL;
}

u8* loadFile(char* file, u32* size) {
	return loadImpl(file, "rb", size);
}

u8* loadText(char* file, u32* size) {
	return loadImpl(file, "r", size);
}