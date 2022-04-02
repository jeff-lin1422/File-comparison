#include <stdlib.h>
#include <stdio.h>
#include "strbuf.h"
#include <string.h>

#ifndef DEBUG
#define DEBUG 0
#endif

int sb_toString(strbuf_t *sb, char* dest){
	//make sure dest is allocated before calling
	//dest = malloc(sb->used*sizeof(char));
	for(int i = 0; i < sb->used; i++){
		dest[i] = sb->data[i];
	}
	//printf("%s\n",dest);
	return 0;
}

int sb_init(strbuf_t *sb, size_t length)
{
	if(length <= 0) return 1;
	sb->data = malloc(sizeof(char) * length);
	if(!sb->data) return 1;
	sb->used = 0;
	sb->length = length;
	sb->data[0] = '\0';
	return 0;
}

void sb_destroy(strbuf_t *sb)
{
    free(sb->data);
}

int sb_append(strbuf_t *sb, char letter)
{
	if (sb->used == sb->length) {

		size_t size = sb->length * 2;
		char *p = realloc(sb->data, sizeof(char) * size);
			if (!p) return 1;
		sb->data = p;
		sb->length = size;
		if (DEBUG) printf("Increased size to %lu\n", size);
	}
	if(letter != '\0'){
		sb->data[sb->used] = letter;
		++sb->used;
		sb_append(sb, '\0');
	}
	return 0;
}

int sb_print(strbuf_t* sb){
	for(int i = 0; i < sb->used; i++){
		printf("%c", sb->data[i]);
	}
	return 0;
}

int sb_remove(strbuf_t *sb, char *item)
{
	if(sb->length == 0) return 1;
	--sb->used;
	if(item) *item = sb->data[sb->used];
    	return 0;
}

int sb_concat(strbuf_t* sb, char* str){
	size_t wordLength = strlen(str);
	int counter = 0;

	char* p = realloc(sb->data, sizeof(char) * sb->used+wordLength+1);
	if(!p) return 1;
	sb->length = sb->used+wordLength+1;
	sb->data = p;

	for(int i = sb->used; i < sb->used+wordLength; i++){
		sb->data[i] = str[counter];
		counter++;
	}
	sb->used += wordLength;
	/*
	if(sb->data[sb->used-1] != '\0'){
		sb_append(sb, '\0');
	}*/
	return 0;
}
