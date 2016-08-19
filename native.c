#include "native.h"
#include "classloader.h"
#include "runtime.h"
#include <stdio.h>

NativeFunction * native = 0;
NativeFunction * lastNative = 0;

void * get_native_function(const char * name, const char * descriptor, bool isInstanceMethod)
{
	if (native) {
		NativeFunction * n = native;
		while (n) {
			if (strcmp(n->name, name) == 0 && strcmp(n->descriptor, descriptor) == 0 && n->isInstanceMethod == isInstanceMethod)
				return n;
			n = n->next;
		}
	}
	return 0;
}

void register_native_function(NativeFunction * function)
{
	if (native) {
		function->prev = lastNative;
		function->next = 0;
		lastNative->next = function;
		lastNative = function;
	} else {
		native = lastNative = function;
		function->next = function->prev = 0;
	}
}

void unreigster_native_function(NativeFunction * function)
{
	if (native) {
		if (function->prev) {
			function->prev->next = function->next;
		}
		if (function->next) {
			function->next->prev = function->prev;
		}
		if (lastNative == function)
			lastNative = lastNative->prev;
		if (!lastNative)
			native = 0;
	}
}

void jvm_ntv_print_int(JavaClass * jclass, int i);
void jvm_ntv_print_long(JavaClass * jclass, int64_t i);
void jvm_ntv_print_float(JavaClass * jclass, int i);
void jvm_ntv_print_double(JavaClass * jclass, double i);
void jvm_ntv_print_short(JavaClass * jclass, short i);
void jvm_ntv_print_char(JavaClass * jclass, char i);
void jvm_ntv_print_byte(JavaClass * jclass, uint8_t i);
void jvm_ntv_print_array(JavaClass * jclass, JArrayInstance * jarray);
void jvm_ntv_print_string(JavaClass * jclass, JClassInstance * string);
void jvm_ntv_write_sdout(JavaClass * jclass, int i);

NativeFunction nPrintI;
NativeFunction nPrintL;
NativeFunction nPrintF;
NativeFunction nPrintD;
NativeFunction nPrintS;
NativeFunction nPrintC;
NativeFunction nPrintB;
NativeFunction nPrintArray_c;
NativeFunction nPrintArray_i;
NativeFunction nPrintArray_l;
NativeFunction nPrintArray_f;
NativeFunction nPrintArray_d;
NativeFunction nPrintArray_s;
NativeFunction nPrintArray_b;
NativeFunction nPrintArray_bool;
NativeFunction nPrintString;
NativeFunction nPrintObject;
NativeFunction nWriteStdout;

//extern JavaClass * stringJclass;
//int stringCharacterArrayIndex;

void register_jvm_natives(void)
{
	nPrintI.name = "jvm_ntv_print_var";
	nPrintI.descriptor = "(I)V";
	nPrintI.function = jvm_ntv_print_int;
	nPrintI.isInstanceMethod = false;
	register_native_function(&nPrintI);

	nPrintL.name = "jvm_ntv_print_var";
	nPrintL.descriptor = "(J)V";
	nPrintL.function = jvm_ntv_print_long;
	nPrintL.isInstanceMethod = false;
	register_native_function(&nPrintL);

	nPrintF.name = "jvm_ntv_print_var";
	nPrintF.descriptor = "(F)V";
	nPrintF.function = jvm_ntv_print_float;
	nPrintF.isInstanceMethod = false;
	register_native_function(&nPrintF);

	nPrintD.name = "jvm_ntv_print_var";
	nPrintD.descriptor = "(D)V";
	nPrintD.function = jvm_ntv_print_double;
	nPrintD.isInstanceMethod = false;
	register_native_function(&nPrintD);

	nPrintS.name = "jvm_ntv_print_var";
	nPrintS.descriptor = "(S)V";
	nPrintS.function = jvm_ntv_print_short;
	nPrintS.isInstanceMethod = false;
	register_native_function(&nPrintS);

	nPrintC.name = "jvm_ntv_print_var";
	nPrintC.descriptor = "(C)V";
	nPrintC.function = jvm_ntv_print_char;
	nPrintC.isInstanceMethod = false;
	register_native_function(&nPrintC);

	nPrintB.name = "jvm_ntv_print_var";
	nPrintB.descriptor = "(B)V";
	nPrintB.function = jvm_ntv_print_byte;
	nPrintB.isInstanceMethod = false;
	register_native_function(&nPrintB);

	nPrintArray_c.name = "jvm_ntv_print_var";
	nPrintArray_c.descriptor = "([C)V";
	nPrintArray_c.function = jvm_ntv_print_array;
	nPrintArray_c.isInstanceMethod = false;
	register_native_function(&nPrintArray_c);

	nPrintArray_i.name = "jvm_ntv_print_var";
	nPrintArray_i.descriptor = "([I)V";
	nPrintArray_i.function = jvm_ntv_print_array;
	nPrintArray_i.isInstanceMethod = false;
	register_native_function(&nPrintArray_i);

	nPrintArray_l.name = "jvm_ntv_print_var";
	nPrintArray_l.descriptor = "([J)V";
	nPrintArray_l.function = jvm_ntv_print_array;
	nPrintArray_l.isInstanceMethod = false;
	register_native_function(&nPrintArray_l);

	nPrintArray_f.name = "jvm_ntv_print_var";
	nPrintArray_f.descriptor = "([F)V";
	nPrintArray_f.function = jvm_ntv_print_array;
	nPrintArray_f.isInstanceMethod = false;
	register_native_function(&nPrintArray_f);

	nPrintArray_d.name = "jvm_ntv_print_var";
	nPrintArray_d.descriptor = "([D)V";
	nPrintArray_d.function = jvm_ntv_print_array;
	nPrintArray_d.isInstanceMethod = false;
	register_native_function(&nPrintArray_d);

	nPrintArray_s.name = "jvm_ntv_print_var";
	nPrintArray_s.descriptor = "([S)V";
	nPrintArray_s.function = jvm_ntv_print_array;
	nPrintArray_s.isInstanceMethod = false;
	register_native_function(&nPrintArray_s);

	nPrintArray_b.name = "jvm_ntv_print_var";
	nPrintArray_b.descriptor = "([B)V";
	nPrintArray_b.function = jvm_ntv_print_array;
	nPrintArray_b.isInstanceMethod = false;
	register_native_function(&nPrintArray_b);

	nPrintArray_bool.name = "jvm_ntv_print_var";
	nPrintArray_bool.descriptor = "([Z)V";
	nPrintArray_bool.function = jvm_ntv_print_array;
	nPrintArray_bool.isInstanceMethod = false;
	register_native_function(&nPrintArray_bool);

	nPrintString.name = "jvm_ntv_print_var";
	nPrintString.descriptor = "(Ljava/lang/String;)V";
	nPrintString.function = jvm_ntv_print_string;
	nPrintString.isInstanceMethod = false;
	register_native_function(&nPrintString);

	nPrintObject.name = "jvm_ntv_print_var";
	nPrintObject.descriptor = "(Ljava/lang/Object;)V";
	nPrintObject.function = jvm_ntv_print_int;
	nPrintObject.isInstanceMethod = false;
	register_native_function(&nPrintObject);

	nWriteStdout.name = "jvm_ntv_write_stdout";
	nWriteStdout.descriptor = "(I)V";
	nWriteStdout.function = jvm_ntv_write_sdout;
	nWriteStdout.isInstanceMethod = false;
	register_native_function(&nWriteStdout);

}

void jvm_ntv_print_int(JavaClass * jclass, int i)
{
	printf("Java app printed this integer: %i (0x%x)\n", i, i);
}

void jvm_ntv_print_long(JavaClass * jclass, int64_t i)
{
	printf("Java app printed this long: %I64d (0x%I64x)\n", i, i);
}

void jvm_ntv_print_float(JavaClass * jclass, int i)
{
	printf("Java app printed this float: %f (0x%x)\n", ((float *)&i)[0], i);
}

void jvm_ntv_print_double(JavaClass * jclass, double i)
{
	printf("Java app printed this double: %.15f (0x%I64x)\n", i, *(uint64_t *)&i);
}

void jvm_ntv_print_short(JavaClass * jclass, short i)
{
	printf("Java app printed this short: %i\n", i);
}

void jvm_ntv_print_char(JavaClass * jclass, char i)
{
	printf("Java app printed this char: '%c' (%u) (0x%X)\n", i, i, i);
}

void jvm_ntv_print_byte(JavaClass * jclass, uint8_t i)
{
	printf("Java app printed this byte: %i\n", i);
}

void jvm_ntv_print_array(JavaClass * jclass, JArrayInstance * jarray)
{
	if (jarray->atype == 4) {
		printf("outputted boolean array:\n");
		for (int i = 0; i < jarray->size; i++) {
			printf("\t(%u) %s\n", i, jarray->data_bool[i] ? "True" : "False");
		}
	}
	else if (jarray->atype == 5) {
		printf("outputted char array: ");
		for (int i = 0; i < jarray->size; i++) {
			putchar(jarray->data_c[i]);
		}
		putchar('\n');
	}
	else if (jarray->atype == 6) {
		printf("outputted float array:\n");
		for (int i = 0; i < jarray->size; i++) {
			printf("\t(%u) %f\n", i, jarray->data_f[i]);
		}
	}
	else if (jarray->atype == 7) {
		printf("outputted double array:\n");
		for (int i = 0; i < jarray->size; i++) {
			printf("\t(%u) %f\n", i, jarray->data_d[i]);
		}
	}
	else if (jarray->atype == 8) {
		printf("outputted byte array:\n");
		for (int i = 0; i < jarray->size; i++) {
			printf("\t(%u) %i\n", i, jarray->data_b[i]);
		}
	}
	else if (jarray->atype == 9) {
		printf("outputted short array:\n");
		for (int i = 0; i < jarray->size; i++) {
			printf("\t(%u) %i\n", i, jarray->data_s[i]);
		}
	}
	else if (jarray->atype == 10) {
		printf("outputted int array:\n");
		for (int i = 0; i < jarray->size; i++) {
			printf("\t(%u) %i\n", i, jarray->data_i[i]);
		}
	}
	else if (jarray->atype == 11) {
		printf("outputted long array:\n");
		for (int i = 0; i < jarray->size; i++) {
			printf("\t(%u) %I64d\n", i, jarray->data_l[i]);
		}
	}
}

extern JavaClass * stringJclass;

void jvm_ntv_print_string(JavaClass * jclass, JClassInstance * string)
{
	for (int i = 0; i < stringJclass->intanceFieldsCount; i++) {
		if (strcmp(stringJclass->fields[i].name, "characters") == 0 && strcmp(stringJclass->fields[i].desc, "[C") == 0) {
			int index = stringJclass->fields[i].instanceIndex;

			JArrayInstance * a = (JArrayInstance * ) ( (uint32_t) string->fields[index]);
			if (a->atype == 5) {
				for (int i = 0; i < a->size; i++) {
					printf("%c", a->data_c[i]);
				}
				printf("\n");
			}
			break;
		}
	}
}

void jvm_ntv_write_sdout(JavaClass * jclass, int i)
{
	(void)jclass;
	putchar(i);
}

