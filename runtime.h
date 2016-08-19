#ifndef _runtime_h_
#define _runtime_h_

#include <stdint.h>
#include <stdbool.h>
#include "classloader.h"

typedef struct _JClassInstance
{
	JavaClass * jclass;

	/* non-static fields */
	uint64_t * fields; // length = jclass->intanceFieldsCount
} JClassInstance;

struct _JArrayInstance;
typedef struct _JArrayInstance
{
	struct _JArrayInstance * next;
	struct _JArrayInstance * prev;

	/*
		T_REFERENCE_CLASS 0
		T_REFERENCE_ARRAY 1
		T_REFERENCE_INTERFACE 2

		T_BOOLEAN	4
		T_CHAR		5
		T_FLOAT		6
		T_DOUBLE	7
		T_BYTE		8
		T_SHORT		9
		T_INT		10
		T_LONG		11
	*/
	int atype;
	int size;

	// N.B. JIT code depends on these variables (^) being at the start and in that order

	union {
		void * data;
		bool * data_bool;
		uint16_t * data_c;
		float * data_f;
		double * data_d;
		int8_t * data_b;
		int16_t * data_s;
		int32_t * data_i;
		int64_t * data_l;
		void ** data_refence;
	};
	union {
		JavaClass * jclass;
		struct _JArrayInstance * jarray;
		// TODO Interfaces
	};
} JArrayInstance;

// void * get_method(JavaClass * jclass, int index);
void * get_ptr_for_method(Method * m);
JClassInstance * new_jclass(JavaClass * jclass);
JArrayInstance * new_array(char atype, int size);

void castore(uint16_t c, int index, JArrayInstance * jarray);
void iastore(int32_t c, int index, JArrayInstance * jarray);
void bastore(int8_t c, int index, JArrayInstance * jarray);
void aastore(void * c, int index, JArrayInstance * jarray);

unsigned short caload(int index, JArrayInstance * jarray);

#endif
