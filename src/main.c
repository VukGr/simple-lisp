#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include "utils.h"
#include "tokenizer.h"
#include "lisp.h"

LVal *lprint(LContext *context) {
	Print(lookup("local_0", context));

	return &Nil;
}

//This one doesn't really work atm
//TODO: remake Read (and the tokenizer)to work with 
//      strings/file pointers directly rather than globals
LVal *lread(LContext *context) {
	return Read(lookup("local_0", context));
}

LVal *leval(LContext *context) {
	return Eval(lookup("local_0", context), context);
}

//Macro
//Sets the id from arg 0 to the value from arg 1
//IMPROVE: Type check the arg 0
LVal *lset(LContext *context) {
	char *id = lookup("local_0", context)->Str;
	LVal *val = Eval(LFun_ConvertArg(lookup("local_1", context)), context);

	//Prev since adding to lset's context makes no sense
	Context_AddVar(context->Prev, 
		LVar_New(id, val));

	return &Nil;
}

//Macro
//Just does every expression that's passed to it as an argument
LVal *ldo(LContext *context) {
	//Reverse the argument list
	//Since they're treated like a stack
	//TODO: Make this into a function, something like LList_Reverse
	LVar *prev = NULL, *next = NULL, *cur;
	for(cur = context->Top; cur != NULL ; cur = next) {
		next = cur->Next;
		cur->Next = prev;
		prev = cur;
	}
	context->Top = prev;

	LVal *res;
	for(LVar *expr = context->Top; expr != NULL ; expr = expr->Next) {
		res = Eval(LFun_ConvertArg(expr->Value), context);
	}

	return res;
}

//TODO: Typecheck
LVal *lfun(LContext *context) {
	LVal *args = lookup("local_0", context);
	LVal *expr = LFun_ConvertArg(lookup("local_1", context));
	int argc = LList_Len(args);

	return LFun_New(expr, args, argc, false);;
}

//This evals right to left, but oh well
//Or it doesn't, since they're eval-ed before passing to this?
LVal *lplus(LContext *context) {
	LVal *sum = LNum_New(0);
	
	for(LVar *arg = context->Top; arg != NULL; arg = arg->Next) {
		*(sum->Num) += *(arg->Value->Num);
	}

	return sum;
}

LVal *lminus(LContext *context) {
	LVal *sum = LNum_New(0);
	
	for(LVar *arg = context->Top; arg != NULL; arg = arg->Next) {
		*(sum->Num) -= *(arg->Value->Num);
	}

	return sum;
}

LVal *lmul(LContext *context) {
	LVal *prod = LNum_New(1);
	
	for(LVar *arg = context->Top; arg != NULL; arg = arg->Next) {
		*(prod->Num) *= *(arg->Value->Num);
	}

	return prod;
}

LVal *ltick(LContext *context) {
	LVal *arg = LFun_ConvertArg(lookup("local_0", context));

	return arg;
}

//TODO: Negative numbers
//TODO: Free-ing almost anything 
//TODO: Defining macros from lisp
//TODO: Proper closures (lexical scoping)
//TODO: Garbage Collection
//TODO: Standard library

int main() {
	char *src = read_whole_file("./bin/src.lisp");
	p = src;
	lineStart = p;
	printf("Src:\n%s\n---\n", src);

	LContext *base = Context_New(NULL);
	Context_AddVar(base, LVar_New("read", LBuiltin_New(&lread, 1, LFUN_FUNCTION))); //Probably not smart to call this one for now
	Context_AddVar(base, LVar_New("print", LBuiltin_New(&lprint, 1, LFUN_FUNCTION)));
	Context_AddVar(base, LVar_New("eval", LBuiltin_New(&leval, 1, LFUN_MACRO)));
	Context_AddVar(base, LVar_New("set", LBuiltin_New(&lset, 2, LFUN_MACRO)));
	Context_AddVar(base, LVar_New("do", LBuiltin_New(&ldo, VARIABLE_FUNCTION, LFUN_MACRO))); 
	Context_AddVar(base, LVar_New("fun", LBuiltin_New(&lfun, 2, LFUN_MACRO))); 
	Context_AddVar(base, LVar_New("+", LBuiltin_New(&lplus, VARIABLE_FUNCTION, LFUN_FUNCTION))); 
	Context_AddVar(base, LVar_New("-", LBuiltin_New(&lminus, VARIABLE_FUNCTION, LFUN_FUNCTION))); 
	Context_AddVar(base, LVar_New("*", LBuiltin_New(&lmul, VARIABLE_FUNCTION, LFUN_FUNCTION))); 
	Context_AddVar(base, LVar_New("tick", LBuiltin_New(&ltick, 1, LFUN_MACRO))); 

	//while(next()) {
		//printf("tkType: %c ", tkType);
		//if(tkType != '(' && tkType != ')') printf("tkVal: %s", tkVal);
		//printf("\n");
	//}

	LVal *val;
	while(val = Read())
		Eval(val, base);

	//free(src); Causes an exception!?
	return 0;
}