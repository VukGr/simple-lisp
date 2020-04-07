#ifndef __TOKENIZER_H__
#define __TOKENIZER_H__

typedef enum {
	TKN_IDENTIFIER = 'I',
	TKN_NUMBER = 'N',
	TKN_STRING = 'S',
	TKN_TICK = '\'',
	TKN_OPENP = '(',
	TKN_CLOSEDP = ')',
	TKN_EOF = '\0',
} TokenType;

extern char *p;

extern TokenType tkType; 
extern char *tkVal;

extern int lineCount;
extern char* lineStart;

int next();
int expect(TokenType expected);
void push_back(int amount);

#endif