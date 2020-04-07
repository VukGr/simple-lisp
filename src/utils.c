#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "utils.h"

char* read_whole_file(char *name) {
	FILE *f = fopen(name, "rb");
	
	fseek(f, 0L, SEEK_END);
	int size = ftell(f);
	fseek(f, 0L, SEEK_SET);
	
	char *str = (char *)malloc((size)*sizeof(char));
	fread(str, 1, size, f);
	str[size] = '\0';
	
	fclose(f);
	
	return str;
}

char isvalidid_start(char p) {
	return isalpha(p) || (ispunct(p) && (p != '(') && (p != ')') && (p != '"'));
}

char isvalidid(char p) {
	return isvalidid_start(p) || isdigit(p);
}