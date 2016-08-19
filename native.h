#ifndef _native_h_
#define _native_h_

#include <stdbool.h>

struct _NativeFunction;
typedef struct _NativeFunction 
{
	struct _NativeFunction * next;
	struct _NativeFunction * prev;
	const char * name;
	const char * descriptor;
	void * function;
	bool isInstanceMethod;
} NativeFunction;

void register_jvm_natives(void);

void * get_native_function(const char * name, const char * descriptor, bool isInstanceMethod);
void register_native_function(NativeFunction * function);
void unreigster_native_function(NativeFunction * function);

#endif
