/* Functions called by the generated machine code */

#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>

#include "classloader.h"
#include "encoder.h"


void * get_ptr_for_method(Method * m)
{
	void * m_ptr = m->machineCode;
	if(!m_ptr)
		m_ptr = convert_bytecode(m);
	if (!m_ptr) {
		printf("error converting bytecode for method %s %s\n", m->descriptor, m->name);
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		exit(1);
	}
	return m_ptr;
}

typedef void(*function) (void);

JClassInstance * new_jclass(JavaClass * jclass)
{
	JClassInstance * j = malloc(sizeof(JClassInstance));
	memset(j, 0, sizeof(JClassInstance));
	j->jclass = jclass;

	j->fields = malloc(8 * jclass->intanceFieldsCount);
	memset(j->fields, 0, 8 * jclass->intanceFieldsCount);

	return j;
}

JArrayInstance * loadedArray = 0;
JArrayInstance * lastLoadedArray = 0;

JArrayInstance * new_array(char atype, int size)
{
	if (size < 0) {
		fprintf(stderr, "size (%i) is less than 0 creating new array\n", size);
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		exit(1);
	}
	static int sizes[] = {4, 4, 4, 0, 1, 2, 4, 8, 1, 2, 4, 8};
	int elementSize = sizes[atype];

	JArrayInstance * a = malloc(sizeof(JArrayInstance));
	memset(a, 0, sizeof(JArrayInstance));
	if (!a) {
		fprintf(stderr, "malloc failed creating new array\n");
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		exit(1);
	}

	if (!loadedArray) {
		loadedArray = a;
		a->next = 0;
		a->prev = 0;
		lastLoadedArray = loadedArray;
	}
	else {
		lastLoadedArray->next = a;
		a->prev = lastLoadedArray;
		a->next = 0;
		lastLoadedArray = a;
	}

	a->size = size;
	a->atype = atype;
	if (size) {
		a->data = malloc(size * elementSize);
		memset(a->data, 0, size * elementSize);
	}
	else {
		a->data = (void *)0xBADC0DE;
	}
	return a;
}

void castore(uint16_t c, int index, JArrayInstance * jarray)
{
	if (jarray->atype != 5 || index < 0 || index >= jarray->size) {
		fprintf(stderr, "castore failed\n");
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		exit(1);
	}
	jarray->data_c[index] = c;
}

unsigned short caload(int index, JArrayInstance * jarray)
{
	if (jarray->atype != 5 || index < 0 || index >= jarray->size) {
		fprintf(stderr, "caload failed\n");
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		exit(1);
	}
	return jarray->data_c[index];
}

void iastore(int32_t c, int index, JArrayInstance * jarray)
{
	if ((jarray->atype != 10 && jarray->atype != 6 && jarray->atype > 3) || index < 0 || index >= jarray->size) {
		fprintf(stderr, "a/i/fastore failed\n");
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		exit(1);
	}
	jarray->data_i[index] = c;
}

signed int iaload(int index, JArrayInstance * jarray)
{
	if ((jarray->atype != 10 && jarray->atype != 6 && jarray->atype > 3) || index < 0 || index >= jarray->size) {
		fprintf(stderr, "a/i/faload failed\n");
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		exit(1);
	}
	return jarray->data_i[index];
}

void bastore(int8_t c, int index, JArrayInstance * jarray)
{
	if (jarray->atype != 8 || index < 0 || index >= jarray->size) {
		fprintf(stderr, "bastore failed\n");
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		exit(1);
	}
	jarray->data_b[index] = c;
}

void aastore(void * c, int index, JArrayInstance * jarray)
{
	if (jarray->atype > 2|| index < 0 || index >= jarray->size) {
		fprintf(stderr, "aastore failed\n");
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
		system("PAUSE");
#endif
		exit(1);
	}
	jarray->data_refence[index] = c;
}
