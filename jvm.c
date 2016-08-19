#if defined(_WIN32) || defined(WIN32) || defined(__WIN32)
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include <stdio.h>
#include <stdlib.h>
#include "classloader.h"
#include "encoder.h"
#include "native.h"
#include "runtime.h"

typedef void (*function) (int v3, int v2, int v1, int v0);

char * run_dir = 0;
JavaClass * stringJclass;
JavaClass * systemJclass;

int main(int argc, char ** argv)
{
	char * classfile = 0;
	for(int i = 1; i < argc; i++){
		if(argv[i][0] != '-'){
			classfile = argv[i];
			break;
		}
	}

	if (!classfile) {
		printf("Please specify class file to run\n");
		classfile = (char *)malloc(80);
		scanf("%79s", classfile);
	}

	char * s = classfile;
	while (*s) {
		if (*s == '\\')
			*s = '/';
		s++;
	}

	int k = strlen(classfile);
	while (k && classfile[k] != '/') {
		k--;
	}

	if (k) {
		run_dir = malloc(k+1);
		memcpy(run_dir, classfile, k);
		run_dir[k] = '/';
		run_dir[k + 1] = 0;
	}

	register_jvm_natives();

	/* Load Object.class */

	JavaClass * c = load_class("jre/java/lang/Object.class");
	if (!c) {
		printf("Failed to load Object.class\n");
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		return 1;
	}
	c->access_flags[4] = c->access_flags[5] = 0; // set super class to 0
	if (run_static_constructor(c)) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		return 1;
	}

	Method * objInit = load_method_in_class(c, "<init>", "()V");
	objInit->bytecodeLength = 1;
	static uint8_t ret = 0xb1;
	objInit->bytecode = &ret;

	/* Load String.class */
	
	stringJclass = load_class("jre/java/lang/String.class");
	if (!stringJclass) {
		printf("Failed to load String.class\n");
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		return 1;
	}
	if (run_static_constructor(stringJclass)) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		return 1;
	}

	/* Load System.class */

	systemJclass = load_class("jre/java/lang/System.class");
	if (!systemJclass) {
		printf("Failed to load System.class\n");
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		return 1;
	}

	Field * field = systemJclass->fields;
	while (field) {
		if (strcmp(field->name, "out") == 0 && strcmp(field->desc, "Ljava/io/PrintStream;") == 0) {
			JClassInstance * x = new_jclass(load_class("jre/java/io/PrintStream.class"));
			field->data = (uintptr_t) x;
			break;
		}

		field = field->next;
	}

	if (run_static_constructor(systemJclass)) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		return 1;
	}

	/* Load main class */

	c = load_class(classfile);
	if (!c) { 
		printf("Failed to load main class\n");
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		return 1; 
	}

	if (run_static_constructor(c)) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		return 1;
	}


	Method * mainMethod = load_method_in_class(c, "main", "([Ljava/lang/String;)V");
	if (!mainMethod) {
		fputs("Could not find main method!\n", stderr);
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		return 1;
	}
		
	unsigned char * data = convert_bytecode(mainMethod);
	if (!data) {
		fputs("Could not convert bytecode in main method!\n", stderr);
		#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
				system("PAUSE");
		#endif
		return 1;
	}


	function f = (function)data;
	f(0,0,0,0);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
	system("PAUSE");
#endif

	return 0;
}
