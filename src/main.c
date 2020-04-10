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
LVal *lset(LContext *context) {
	char *id = LVal_AssumeType(TYPE_IDENTIFIER, lookup("local_0", context))->Str;
	LVal *val = Eval(LFun_ConvertArg(lookup("local_1", context)), context);

	//Prev since adding to lset's context makes no sense
	Context_AddVar(context->Prev, LVar_New(id, val));

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

LVal *lfun(LContext *context) {
	LVal *args = LVal_AssumeType(TYPE_LIST, lookup("local_0", context));
	LVal *expr = LFun_ConvertArg(LVal_AssumeType(TYPE_LIST, lookup("local_1", context)));
	int argc = LList_Len(args);

	return LFun_New(expr, args, argc, false);;
}

LVal *lplus(LContext *context) {
	LVal *res = LNum_New(0);
	
	for(LVar *arg = context->Top; arg != NULL; arg = arg->Next) {
		*(res->Num) += *(LVal_AssumeType(TYPE_NUMBER, arg->Value)->Num);
	}

	return res;
}

LVal *lminus(LContext *context) {
	LVal *firstVal = LVal_AssumeType(TYPE_NUMBER, lookup("local_0", context));

	if(context->Top->Value == firstVal)
		return LNum_New(-(*(firstVal->Num)));
	
	LVal *res = LNum_New(*(firstVal->Num));

	for(LVar *arg = context->Top; arg->Next != NULL; arg = arg->Next) {
		*(res->Num) -= *(LVal_AssumeType(TYPE_NUMBER, arg->Value)->Num);
	}

	return res;
}

LVal *lmul(LContext *context) {
	LVal *res = LNum_New(1);
	
	for(LVar *arg = context->Top; arg != NULL; arg = arg->Next) {
		*(res->Num) *= *(LVal_AssumeType(TYPE_NUMBER, arg->Value)->Num);
	}

	return res;
}

LVal *_ldiv(LContext *context) {
	LVal *arg1 = LVal_AssumeType(TYPE_NUMBER, lookup("local_0", context));
	LVal *arg2 = LVal_AssumeType(TYPE_NUMBER, lookup("local_1", context));
	
	return LNum_New(*(arg1->Num) / *(arg2->Num));
}

LVal *ltick(LContext *context) {
	LVal *arg = LFun_ConvertArg(lookup("local_0", context));

	return arg;
}

LVal *lcons(LContext *context) {
	LVal *car = lookup("local_0", context);
	LVal *cdr = lookup("local_1", context);

	return LList_New(car, cdr);
}

LVal *lcar(LContext *context) {
	LVal *arg = context->Top->Value;
	if(arg->Type == TYPE_NIL) return &Nil;
	else return LVal_AssumeType(TYPE_LIST, arg)->List->CAR;
}


LVal *lcdr(LContext *context) {
	LVal *arg = context->Top->Value;
	if(arg->Type == TYPE_NIL) return &Nil;
	else return LVal_AssumeType(TYPE_LIST, arg)->List->CDR;
}

//TODO: Change read to work with strings or (file?)streams
//TODO: Free-ing almost anything 
//TODO: Defining macros from lisp
//TODO: Proper closures (lexical scoping)
//TODO: Garbage Collection
//TODO: Standard library

int main() {
	char *src = read_whole_file("./src.lisp");
	p = src;
	lineStart = p;

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
	Context_AddVar(base, LVar_New("/", LBuiltin_New(&_ldiv, 2, LFUN_FUNCTION))); 
	Context_AddVar(base, LVar_New("tick", LBuiltin_New(&ltick, 1, LFUN_MACRO))); 
	Context_AddVar(base, LVar_New("cons", LBuiltin_New(&lcons, 2, LFUN_FUNCTION))); 
	Context_AddVar(base, LVar_New("car", LBuiltin_New(&lcar, 1, LFUN_FUNCTION)));
	Context_AddVar(base, LVar_New("cdr", LBuiltin_New(&lcdr, 1, LFUN_FUNCTION)));

	LVal *val;
	while(val = Read())
		Eval(val, base);

	//free(src); Causes an exception!?
	return 0;
}