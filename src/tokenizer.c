#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "tokenizer.h"
#include "utils.h"

char *p = NULL;

int lineCount = 1;
char* lineStart = 0;

TokenType tkType = TKN_EOF;
char *tkVal = NULL;

static int next_id() {
	int size = 1;
	for (++p; isvalidid(*p); ++p, ++size)
		;
	tkVal = malloc(sizeof(char)*(size+1));
	tkType = TKN_IDENTIFIER;
	
	strncpy(tkVal, p-size, size);
	tkVal[size] = '\0';

	return size;
}

static int next_number() {
	int size = 0;
	for (; isdigit(*p); ++p, ++size)
		;
	tkVal = malloc(sizeof(char)*(size+1));
	tkType = TKN_NUMBER;
	
	strncpy(tkVal, p-size, size);
	tkVal[size] = '\0';

	return size;
}

static int next_string() {
	int size = 0;
	for (++p; *p != '"' && *p != '\0'; ++p, ++size)
		;

	if(*p == '\0') {
		//TODO: Replace these with proper error functions
		printf("\nError at Line %d, Pos %d: Unexpected file end while parsing string.\n", lineCount, p - lineStart);
		return 0;
	}

	tkVal = malloc(sizeof(char)*(size+1));
	tkType = TKN_STRING;
	
	strncpy(tkVal, p-size, size);
	tkVal[size] = '\0';
	++p;

	return size+2;
}

//TODO: needs better name, since it also handle '
static int next_sym() {
	tkType = *(p++);
	return 1;
}

int next() {
	for (; isspace(*p); ++p) {
		if(*p == '\n') {
			lineCount++;
			lineStart = p;
		}
	}

	if(*p == '(' || *p == ')' || *p == '\'') return next_sym();
	else if(isvalidid_start(*p)) return next_id();
	else if(isdigit(*p)) return next_number();
	else if(*p == '"') return next_string();
	else if(*p == '\0') return 0;

	printf("\nError at Line %d, Pos %d: Unexpected character %c while parsing tokens.\n", lineCount, p - lineStart, *p);
	return 0;
}

int expect(TokenType expected) {
	int ret = next();
	if(tkType != expected) {
		printf("\nError at Line %d, Pos %d: Unexpected token %c, expected %c.\n", lineCount, p - lineStart, tkType, expected);
		exit(1);
	}
	return ret;
}

void push_back(int amount) {
	p -= amount;
}