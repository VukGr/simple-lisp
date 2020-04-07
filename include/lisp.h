#ifndef __LISP_H__
#define __LISP_H__

#define VARIABLE_FUNCTION -1
#define LFUN_FUNCTION false
#define LFUN_MACRO true

typedef enum {
	TYPE_IDENTIFIER = 'I',
	TYPE_SYMBOL = 'J',
	TYPE_NUMBER = 'N',
	TYPE_STRING = 'S',
	TYPE_LIST = 'L',
	TYPE_EXPR = 'E',
	TYPE_BUILTINFUNCTION = 'B',
	TYPE_FUNCTION = 'F',
	TYPE_NIL = '0',
	
} valType;

struct LList;
struct LContext;

typedef struct LVal {
	valType Type;
	int RefCount;
	union {
		void *Value;
		int *Num;
		char *Str;
		struct LList *List;
		struct LFun *Fun;
	};
}  LVal;

typedef struct LList {
	LVal *CAR;
	LVal *CDR;
} LList;

typedef LVal * (*LBuiltinFun)(struct LContext *);

typedef struct LFun {
	union {
		LVal *Expr;
		LBuiltinFun FunPtr; //Kinda messy that it gets rid of the indication that it's a pointer
	};
	int IsMacro;
	LVal *Argv;
	int Argc;
} LFun;

typedef struct LVar {
	char *Id;
	LVal *Value;
	struct LVar *Next;
}  LVar;

typedef struct LContext { 
	LVar *Top;
	struct LContext *Prev;
}  LContext;

LVal *Read();
void Print(LVal *val);
LVal *Eval(LVal *val, LContext *context);

LVal *LVal_New(valType type);
void LVal_Free(LVal *val);

LVal *LId_New(char *id);
LVal *LNum_New(int num); 

LVal *LList_New(LVal *car, LVal *cdr);
LVal *LExpr_New(LVal *car, LVal *cdr);
LVal *LList_Head(LVal *val);
LVal *LList_Tail(LVal *val);
int LList_Len(LVal *val);

LVal *LBuiltin_New(LBuiltinFun f, int argc, bool macro);
LVal *LFun_New(LVal *expr, LVal *argv, int argc, bool macro);
LVal *LFun_ConvertArg(LVal *val);
LVal *LMacro_ConvertArg(LVal *val);

LVar *LVar_New(char *id, LVal *val);

LContext *Context_New(LContext *old);
void Context_AddVar(LContext *c, LVar *var);

LVal *Read_List();
LVal *Read();
LVal *fcall(LVal *fun, LVal *args, LContext *context);
LVal *lookup(char *id, LContext *context);

extern LVal Nil;

#endif