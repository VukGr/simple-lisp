#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include "utils.h"
#include "tokenizer.h"
#include "lisp.h"

LVal Nil = {.Type = TYPE_NIL, .Value = NULL};

LVal *LVal_New(valType type) {
	LVal *val = malloc(sizeof(LVal));
	val->Type = type;
	val->RefCount = 0;
}

LVal *LVal_AssumeType(valType type, LVal *val) {
	if(val == NULL) {
		printf("Error: Assumed type %c (%d), got NULL ptr.\n", type);
		exit(1);
	}

	if(type != val->Type) {
		printf("Error: Assumed type %c (%d), got %c (%d). (Value: ", type, type, val->Type, val->Type);
		Print(val);
		printf(")\n");
		exit(1);
	}

	return val;
}

//This might be freeing too much in case of lists
//Since they can be made from other lists (by reference)
//Should probably check for reference count
void LVal_Free(LVal *val) {
	if(val == NULL) return;

	switch(val->Type) {
	case TYPE_IDENTIFIER:
	case TYPE_STRING:
	case TYPE_NUMBER:
	case TYPE_BUILTINFUNCTION:
	case TYPE_NIL:
		break;
	case TYPE_EXPR:
	case TYPE_LIST:
		LVal_Free(val->List->CAR); //Replace with refcount-1
		LVal_Free(val->List->CDR);
		break;
	case TYPE_FUNCTION:
		LVal_Free(val->Fun->Expr);
		LVal_Free(val->Fun->Argv);
		break;
	default:
		printf("Unknown token type %c (%d).\n", val->Type, val->Type);
	}
	if(val->Type != TYPE_NIL) free(val->Value);
}


LVal *LId_New(char *id) {
	LVal *val = LVal_New(TYPE_IDENTIFIER);

	int idlen = strlen(id)+1;
	val->Str = malloc(sizeof(char)*idlen);
	strcpy(val->Str, id);
	val->Str[idlen] = '\0';

	return val;
}

LVal *LNum_New(int num) {
	LVal *val = LVal_New(TYPE_NUMBER);

	val->Num = malloc(sizeof(int));
	*(val->Num) = num;

	return val;
}

LVal *LList_New(LVal *car, LVal *cdr) {
	LVal *val = LVal_New(TYPE_LIST);

	val->List = malloc(sizeof(LList));

	val->List->CAR = car;
	val->List->CDR = cdr;

	return val;
}

LVal *LExpr_New(LVal *car, LVal *cdr) {
	LVal *val = LList_New(car, cdr);
	val->Type = TYPE_EXPR;
	return val;
}

LVal *LList_Head(LVal *val) {
	if(val == &Nil) return &Nil;
	else if(val->Type != TYPE_LIST && val->Type != TYPE_EXPR) {
		printf("Error: LList_Head given non list ");
		Print(val);
		printf("  as argument.\n");
		exit(1);
	}

	return val->List->CAR;
}

LVal *LList_Tail(LVal *val) {
	if(val == &Nil) return &Nil;
	else if(val->Type != TYPE_LIST && val->Type != TYPE_EXPR) {
		printf("Error: LList_Tail given non list ");
		Print(val);
		printf(" as argument.\n");
		exit(1);
	}

	return val->List->CDR;
}

int LList_Len(LVal *val) {
	if(val == &Nil) return 0;
	else if(val->Type != TYPE_LIST && val->Type != TYPE_EXPR) {
		printf("Error: LList_Len given non list ");
		Print(val);
		printf(" as argument.\n");
		exit(1);
	}

	return 1 + LList_Len(LList_Tail(val));
}

LVal *LBuiltin_New(LBuiltinFun f, int argc, bool macro) {
	LVal *val = LVal_New(TYPE_BUILTINFUNCTION);

	val->Fun = malloc(sizeof(LFun));
	val->Fun->FunPtr = f;
	val->Fun->Argc = argc;
	val->Fun->IsMacro = macro;

	return val;
}

LVal *LFun_New(LVal *expr, LVal *argv, int argc, bool macro) {
	LVal *val = LVal_New(TYPE_FUNCTION);

	val->Fun = malloc(sizeof(LFun));
	val->Fun->Expr = LVal_AssumeType(TYPE_EXPR, expr);
	val->Fun->Argv = LVal_AssumeType(TYPE_LIST, argv); 
	val->Fun->Argc = argc;
	val->Fun->IsMacro = macro;

	return val;
}

LVal *LFun_ConvertArg(LVal *val) {
	if(val->Type == TYPE_LIST) {
		val->Type = TYPE_EXPR;

		LFun_ConvertArg(LList_Head(val));
		LFun_ConvertArg(LList_Tail(val));	
	}
	else if(val->Type == TYPE_SYMBOL) {
		val->Type = TYPE_IDENTIFIER;
	}

	return val;
}

LVal *LMacro_ConvertArg(LVal *val) {
	if(val->Type == TYPE_EXPR) {
		val->Type = TYPE_LIST;

		LMacro_ConvertArg(LList_Head(val));
		LMacro_ConvertArg(LList_Tail(val));	
	}
	else if(val->Type == TYPE_IDENTIFIER) {
		val->Type = TYPE_SYMBOL;
	}

	return val;
}

LVar *LVar_New(char *id, LVal *val) {
	LVar *var = malloc(sizeof(LVar));

	int idlen = strlen(id)+1;
	var->Id = malloc(sizeof(char)*idlen);
	strcpy(var->Id, id);
	var->Id[idlen] = '\0';

	var->Value = val;
	var->Next = NULL;

	return var;
}

LContext *Context_New(LContext *old) {
	LContext *c = malloc(sizeof(LContext));
	c->Prev = old;
	c->Top = NULL;
	return c;
}

void Context_AddVar(LContext *c, LVar *var) {
	if(c->Top != NULL) {
		var->Next = c->Top;
	}
	c->Top = var;
}

//TODO: Probably a good idea to refactor this at some point \
        Though it's questionable how much can be refactored \
        since it's mostly edge case handling that can't go  \
        into Read
LVal *Read_List() {
	LVal *this;

	int amount;
	if(amount = next()) {
		if(tkType == ')') {
			return &Nil;
		}
		else {
			push_back(amount);
			this = LExpr_New(Read(), NULL);
		}
	}
	else {
		printf("Error at Line %d, Pos %d: Unexpected end of line while parsing CAR.\n", lineCount, p - lineStart);
		exit(1);
	}
	
	if(amount = next()) {
		if(tkType == ')') {
			this->List->CDR = &Nil;
		}
		else{
			push_back(amount);
			this->List->CDR = Read_List();
		}
	}
	else {
		printf("Error at Line %d, Pos %d: Unexpected end of line while parsing CDR.\n", lineCount, p - lineStart);
		exit(1);
	}

	return this;
}

LVal *Read() {
	LVal *this;

	if(next()) {
		//This probably shouldn't be hardcoded
		// 'X -> (tick X)
		switch(tkType) {
		case TKN_TICK:
			this = LExpr_New(LId_New("tick"), LExpr_New(Read(), &Nil));
			break;
		case TKN_OPENP: 
			this = Read_List();
			break;
		case TKN_NUMBER:
			this = LNum_New(atoi(tkVal));
			free(tkVal);
			break;
		case TKN_STRING:
		case TKN_IDENTIFIER:
			this = LVal_New(tkType); //FIX: this is prolly bad practice, use the proper enum.
			this->Str = tkVal; //Not using LId/LStr_New since there's no point to malloc-ing a new str
			break;
		default:
			printf("Error at Line %d, Pos %d: Unexpected token %c (%d).\n", lineCount, p - lineStart, tkType, tkType);
			exit(1);
		}
		return this;
	}
	return 0;
}

void Print(LVal *val) {
	switch(val->Type) {
	case TYPE_SYMBOL:
		printf("'");
	case TYPE_IDENTIFIER:	//Is there a point to having these? There's no way to pass an id/expr as an argument
		printf("%s", val->Str);
		break;
	case TYPE_STRING:
		printf("\"%s\"", val->Str);
		break;
	case TYPE_NUMBER:
		printf("%d", *(val->Num)); 
		break;
	case TYPE_LIST:
		printf("'");
	case TYPE_EXPR:
		printf("(");
		Print(val->List->CAR);
		printf(" . ");
		Print(val->List->CDR);
		printf(")");
		break;
	case TYPE_FUNCTION:
		printf("FUNCTION: ARGS: ");
		Print(val->Fun->Argv);
		printf(" DEF: ");
		Print(val->Fun->Expr);
		printf(" MACRO?: %d.", val->Fun->IsMacro);
		break;
	case TYPE_BUILTINFUNCTION:
		printf("BUILTIN_FUNCTION(%X)", val->Fun->FunPtr);
		printf(" MACRO?: %d.", val->Fun->IsMacro);
		break;
	case TYPE_NIL:
		printf("nil");
		break;
	default:
		printf("Unknown token type %c (%d).\n", val->Type, val->Type);
	}
}

//TODO: Make an error function?
LVal *fcall(LVal *fun, LVal *args, LContext *context) {
	if(fun->Type != TYPE_FUNCTION && fun->Type != TYPE_BUILTINFUNCTION) {
		printf("Error: Attempt to call ");
		Print(fun);
		printf(" as a function.\n");
		exit(1);
	}
	else if(args->Type != TYPE_EXPR && args->Type != TYPE_NIL) {
		printf("Error: Function given non expr ");
		Print(args);
		printf(" as arguments.\n");
		exit(1);
	}
	else {
		//Handle functions with variable arguments
		if(fun->Fun->Argc != VARIABLE_FUNCTION) {
			if(LList_Len(args) != fun->Fun->Argc) {
				printf("Error: Argument mismatch for function ");
				Print(fun);
				printf(" given: ");
				Print(args);
				printf(" expected: ");
				if(fun->Type != TYPE_BUILTINFUNCTION)
					Print(fun->Fun->Argv);
				else 
					printf("%d arguments, got %d.", fun->Fun->Argc, LList_Len(args));

				exit(1);
			}
		}

		LContext *funContext = Context_New(context);

		//TODO: This needs heavy refactoring
		char namebuf[500];
		char *idStr = namebuf;
		LVal *idList = fun->Fun->Argv;
		int i = 0;
		for(; args->Type != TYPE_NIL ; args = LList_Tail(args)) {
			LVal *argVal = LList_Head(args);

			if(fun->Type == TYPE_FUNCTION) {
				idStr = LList_Head(idList)->Str;
				idList = LList_Tail(idList);
			}
			else if(fun->Type == TYPE_BUILTINFUNCTION) {
				sprintf(namebuf, "local_%d\0", i++);
			}

			if(fun->Fun->IsMacro) 
				LMacro_ConvertArg(argVal);
			else 
				argVal = Eval(argVal, context);

			Context_AddVar(funContext, 
					LVar_New(idStr, argVal));
		} 

		if(fun->Type == TYPE_FUNCTION)
			return Eval(fun->Fun->Expr, funContext);
		else if(fun->Type == TYPE_BUILTINFUNCTION)
			return ((*(fun->Fun->FunPtr))(funContext));
	}
}

LVal *lookup(char *id, LContext *context) {
	for(LContext *contextCur = context; contextCur != NULL; contextCur = contextCur->Prev) 
		for(LVar *var = contextCur->Top; var != NULL; var = var->Next) 
			if(!strcmp(var->Id, id)) 
				return var->Value;

	printf("Error: Couldn't find variable %s.", id);
	exit(1);
}

LVal *Eval(LVal *val, LContext *context) {
	if(val->Type == TYPE_EXPR) {
		return fcall(Eval(val->List->CAR, context), val->List->CDR, context);
	}
	else if(val->Type == TYPE_IDENTIFIER) {
		return lookup(val->Str, context);
	}
	else {
		return val;
	}
}