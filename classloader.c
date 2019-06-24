#if defined(_WIN32) || defined(WIN32) || defined(__WIN32)
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include "classloader.h"
#include "endian.h"
#include "encoder.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

JavaClass * loadedClass = 0;
JavaClass * lastLoadedClass = 0;

int get_constant_from_pool(JavaClass * jclass, int offsetOfLast, int * i);
int get_constant_from_pool_by_index(JavaClass * jclass, unsigned short index);

#define get_double(c8,offset) (( ((uint64_t)c8[offset]<<56) | ((uint64_t)c8[offset+1]<<48) | ((uint64_t)c8[offset+2]<<40) | ((uint64_t)c8[offset+3]<<32) | ((uint64_t)c8[offset+4]<<24) | ((uint64_t)c8[offset+5]<<16) | ((uint64_t)c8[offset+6]<<8) | ((uint64_t)c8[offset+7]) ))
#define get_long(c8,offset) (( ((uint64_t)c8[offset+4]<<56) | ((uint64_t)c8[offset+5]<<48) | ((uint64_t)c8[offset+6]<<40) | ((uint64_t)c8[offset+7]<<32) | ((uint64_t)c8[offset]<<24) | ((uint64_t)c8[offset+1]<<16) | ((uint64_t)c8[offset+2]<<8) | ((uint64_t)c8[offset+3]) ))
#define get_int(c8,offset) (( (c8[offset]<<24) | (c8[offset+1]<<16) | (c8[offset+2]<<8) | (c8[offset+3]) ))
#define get_short(c8,offset) (( (c8[offset]<<8) | c8[offset+1] ))

#ifdef __GNUC__
	#define likely(x)       __builtin_expect(!!(x), 1)
	#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
	#define likely(x)  	((x))
	#define unlikely(x) 	((x))
#endif

JavaClass * load_class(const char * filepath)
{
	JavaClass * jclass = 0;

	FILE * file = fopen(filepath, "rb");
	if (unlikely(!file))
		return 0;

	jclass = malloc(sizeof(JavaClass));
	memset(jclass, 0, sizeof(JavaClass));

	fseek(file, 0, SEEK_END);
	long int fileSz = ftell(file);
	if (unlikely(fileSz == -1))
	{
		fclose(file);
		return 0;
	}
	fseek(file, 0, SEEK_SET);

	jclass->loaded_data = malloc(fileSz);

	fread((char *)jclass->loaded_data, fileSz, 1, file);
	fclose(file);

	uint32_t * c32 = (uint32_t *)jclass->loaded_data;
	uint16_t * c16 = (uint16_t *)jclass->loaded_data;
	uint8_t  * c8 = (uint8_t *)jclass->loaded_data;

	if (unlikely(c32[0] != 0xbebafeca)) {
		fprintf(stderr, "Magic incorrect. Value (big endian) = 0x%x\n", c32[0]);
		return 0;
	}

	if (unlikely(!c16[4])) {
		fputs("constant_pool_count is 0!\n", stderr);
		return 0;
	}
	c16[4] = SwapEndiannessS(c16[4]);
	c16[4]--;

	int offset = 10;
	for (int i = 0; i < c16[4]; i++) {
		offset = get_constant_from_pool(jclass, offset, &i);
		if (unlikely(offset < 0)) return 0;
	}

	jclass->access_flags = &c8[offset];

	offset += 2;
	offset += 2;
	offset += 2;

	int interfacesCount = get_short(c8, offset);
	offset += 2;
	offset += interfacesCount * 2;

	int fieldsCount = get_short(c8, offset);
	offset += 2;
	int fieldInstanceIndex = 0;
	for (int i = 0; i < fieldsCount; i++) {
		int flags = get_short(c8, offset);
		offset += 2;
		int nameIndex = get_short(c8, offset)-1;
		offset += 2;
		int descriptorIndex = get_short(c8, offset)-1;
		offset += 2;

		Field * field = malloc(sizeof(Field));

		int nameOffset = get_constant_from_pool_by_index(jclass, nameIndex) + 1;
		field->name = java_class_utf8_to_c_string(&c8[nameOffset]);
		int descOffset = get_constant_from_pool_by_index(jclass, descriptorIndex) + 1;
		field->desc = java_class_utf8_to_c_string(&c8[descOffset]);
		field->data_64 = 0;
		field->flags = flags;

		if (!jclass->fields) {
			jclass->fields = field;
			field->next = 0;
			field->prev = 0;
			jclass->last_field = jclass->fields;
		}
		else {
			jclass->last_field->next = field;
			field->prev = jclass->last_field;
			field->next = 0;
			jclass->last_field = field;
		}

		if (!(flags & 8)) { /* not static */
			field->instanceIndex = fieldInstanceIndex++;
		}

		int attributesCount = get_short(c8, offset);
		offset += 2;
		for (int j = 0; j < attributesCount; j++) {
			offset += 2;
			int attributesLength = get_int(c8, offset);
			offset += 4;
			offset += attributesLength;
		}
	}
	jclass->intanceFieldsCount = fieldInstanceIndex;

	jclass->methods_count = &c8[offset];

	/* validate methods */

	int methodsCount = get_short(c8, offset);
	offset += 2;
	for (int i = 0; i < methodsCount; i++) {
		offset += 2;
		short nameIndex = get_short(c8, offset);
		if (unlikely(c8[get_constant_from_pool_by_index(jclass, nameIndex - 1)] != 1)) {
			fprintf(stderr, "1) tag is %u\n", c8[get_constant_from_pool_by_index(jclass, nameIndex - 1)]);
			return 0;
		}
		offset += 2;

		short descriptorIndex = get_short(c8, offset);
		if (unlikely(c8[get_constant_from_pool_by_index(jclass, descriptorIndex - 1)] != 1)) {
			fprintf(stderr, "2) tag is %u\n", c8[get_constant_from_pool_by_index(jclass, descriptorIndex - 1)]);
			return 0;
		}
		offset += 2;
		int attributesCount = get_short(c8, offset);
		offset += 2;
		for (int j = 0; j < attributesCount; j++) {
			offset += 2;
			int attributesLength = get_int(c8, offset);
			offset += 4;
			offset += attributesLength;
		}
	}

	int thisclass = get_constant_from_pool_by_index(jclass, get_short(jclass->access_flags, 2) - 1);
	int nameIndex = get_short(c8, thisclass + 1);
	int className = get_constant_from_pool_by_index(jclass, nameIndex - 1);
	const int len = get_short(c8, className + 1);
	
	char * s = malloc(len + 1);
	memcpy(s, &c8[className + 3], len);
	s[len] = 0;
	jclass->class_name = s;

	if (!loadedClass) {
		loadedClass = jclass;
		jclass->next = 0;
		jclass->prev = 0;
		lastLoadedClass = loadedClass;
	}
	else {
		lastLoadedClass->next = jclass;
		jclass->prev = lastLoadedClass;
		jclass->next = 0;
		lastLoadedClass = jclass;
	}
	
	s = malloc(strlen(jclass->class_name) + 15);
	memcpy(s, jclass->class_name, strlen(jclass->class_name));
	memcpy(&s[strlen(jclass->class_name)], ".constants.txt", 15);
	char * s1 = s;
	while (*s1) {
		if (*s1 == '/')
			*s1 = '.';
		s1++;
	}

	file = fopen(s, "w");
	if (unlikely(!file)) {
		fprintf(stderr, "Could not load file '%s' for writing\n", s);
		return jclass;
	}
	free(s);

	offset = 10;
	for (int i = 0; i < c16[4]; i++) {
		if (c8[offset] == 1) {
			fprintf(file, "(%u): UTF-8: ", i);
			for (int i = 0; i < get_short(c8, offset + 1); i++) {
				fputc(c8[offset + 3 + i], file);
			}
			fputc('\n', file);
		}
		else if (c8[offset] == 3) {
			fprintf(file, "(%u): Integer: %i (0x%x)\n", i, get_int(c8, offset+1), get_int(c8, offset + 1));
		}
		else if (c8[offset] == 4) {
			uint32_t v = get_int(c8, offset + 1);
			float f = ((float *)&v)[0];
			fprintf(file, "(%u): Float: %f (0x%x) offset %u\n", i, f, v, offset);
		}
		else if (c8[offset] == 7) {
			fprintf(file, "(%u): Class: name index %u\n", i, get_short(c8, offset + 1)-1);
		}
		else if (c8[offset] == 9) {
			fprintf(file, "(%u): Field Reference: class index %u, name and type index %u\n", i, get_short(c8, offset + 1) - 1, get_short(c8, offset + 3) - 1);
		}
		else if (c8[offset] == 10) {
			fprintf(file, "(%u): Method Reference: class index %u, name and type index %u\n", i, get_short(c8, offset + 1) - 1, get_short(c8, offset + 3) - 1);
		}
		else if (c8[offset] == 11) {
			fprintf(file, "(%u): Interface Method Reference: class index %u, name and type index %u\n", i, get_short(c8, offset + 1) - 1, get_short(c8, offset + 3) - 1);
		}
		else if (c8[offset] == 8) {
			fprintf(file, "(%u): Constant String: utf-8 index %u\n", i, get_short(c8, offset + 1) - 1);
		}
		else if (c8[offset] == 5) {
			fprintf(file, "(%u): Long: %I64d (0x%I64x)\n", i, get_long(c8, offset + 1), get_long(c8, offset + 1));
		}
		else if (c8[offset] == 6) {
			int64_t d = get_double(c8, offset + 1);
			fprintf(file, "(%u): Double: %f\n", i, *((double *)&d));
		}
		else if (c8[offset] == 12) {
			fprintf(file, "(%u): Name and type: name %u type %u\n", i, get_short(c8, offset + 1)-1, get_short(c8, offset + 3)-1);
		}
		else {
			fprintf(file, "(%u): Other\n", i);
		}
		offset = get_constant_from_pool(jclass, offset, &i);
		if (unlikely(offset < 0)) return 0;
	}

	fclose(file);
	
	return jclass;
}

// declared in jvm.c
extern char * run_dir;

JavaClass * load_class_(const char * filename)
{
	int len = strlen(filename);
	char * s1 = malloc(len + 5);
	memcpy(s1, "jre/", 4);
	memcpy(&s1[4], filename, len);
	s1[len + 4] = 0;

	JavaClass * c = load_class(s1);
	if (c) return c;

	if (run_dir) {
		int run_dir_len = strlen(run_dir);
		if (run_dir_len > 4) {
			s1 = realloc(s1, len + run_dir_len + 1);
		}
		memcpy(s1, run_dir, run_dir_len);
		memcpy(&s1[run_dir_len], filename, len);
		s1[len + run_dir_len] = 0;

		JavaClass * c = load_class(s1);
		free(s1);
		return c;
	} else {
		memcpy(s1, filename, len);
		s1[len] = 0;

		JavaClass * c = load_class(s1);
		free(s1);
		return c;
	}
}

JavaClass * get_or_load_class_from_class_constant(JavaClass * jclass, int offset)
{
	int nameIndex = get_short(jclass->loaded_data, offset + 1) - 1;
	offset = get_constant_from_pool_by_index(jclass, nameIndex);
	char * className = java_class_utf8_to_c_string(&jclass->loaded_data[offset + 1]);
	JavaClass * c = get_class(className);
	if (c)
		free(className);
	else {
		int len = strlen(className);
		char * s = malloc(len + 7);
		memcpy(s, className, len);
		memcpy(&s[len], ".class", 6);
		s[len + 6] = 0;
		c = load_class_(s);
		free(s);
		if (!c) {
			fprintf(stderr, "Class '%s' not found\n", className);
			return 0;
		}
		free(className);
	}
	return c;
}

typedef void(*function) (int v3, int v2, int v1, int v0);

int run_static_constructor(JavaClass * jclass)
{
	Method * m = load_method_in_class(jclass, "<clinit>", "()V");
	if (m) {
		unsigned char * data = convert_bytecode(m);
		if (!data) {
			printf("Failed to decode static constructor bytecode in class %s\n", jclass->class_name);
			return 1;
		}
		function f = (function)data;
		f(0,0,0,0);
	}
	return 0;
}


/**
*	Returns an offset into the loaded class file in ram. Used for traversing all constants in the pool.
*/
int get_constant_from_pool (JavaClass * jclass, int offsetOfLast, int * i)
{
	uint8_t * c8 = (uint8_t *) jclass->loaded_data;

	int j = offsetOfLast;
	unsigned char tag = c8[j++];
	switch (tag) {
		case 0: {
			printf("Tag 0 in constant pool!\n");
			j--;
			break;
		}
		case 1:
		{ // utf-8
			uint16_t len = get_short(c8, j);
			j += 2;
			j += len;
			break;
		}
		case 3: // float
		case 4:
		{ // int
			j += 4;
			break;
		}
		case 5: // Double
		case 6:
		{ // long
			j += 8;
			(*i)++;
			break;
		}
		case 7:
		{ // class
			j += 2;
			break;
		}
		case 8:
		{ // string
			j += 2;
			break;
		}
		case 9:
		case 10:
		case 11:
		{ // field ref
			j += 4;
			break;
		}
		case 12:
		{ // name and type
			j += 4;
			break;
		}
		case 15:
		{ // method handle
			j += 3;
			break;
		}
		case 16:
		{ // method type
			j += 2;
			break;
		}
		case 18:
		{ // invoke dynamic
			j += 4;
			break;
		}
		default:
			fprintf(stderr, "Unrecognised tag: %u\n", tag);
			return -1;
	}
	return j;
}

/**
*	Returns an offset into the loaded class file in ram
*/
int get_constant_from_pool_by_index(JavaClass * jclass, unsigned short index)
{
	uint16_t * c16 = (uint16_t *)jclass->loaded_data;

	int offset = 10;
	for (int i = 0; i < c16[4]; i++) {
		if (i == index)
			return offset;
		offset = get_constant_from_pool(jclass, offset, &i);
		if (offset < 0) return -1;
	}
	return -1;
}

void print_constant_pool_utf8(JavaClass * jclass, unsigned short offset)
{
	uint16_t * c16 = (uint16_t *)jclass->loaded_data;
	uint8_t * c8 = (uint8_t *)jclass->loaded_data;

	if (c8[offset] == 1) {
		for (int i = 0; i < get_short(c8, offset + 1); i++) {
			putc(c8[offset + 3 + i], stdout);
		}
		putc('\n', stdout);
	} else {
		printf("not utf8!\n");
	}
	return;
		
}

/**
*	Where len is the known length (in bytes) of utf2
*	utf1 must be null terminated
*/
bool utf8_compare(char * utf1, char * utf2, int len) {
	if(strlen(utf1) != len)
		return false;
	return memcmp(utf1, utf2, len) == 0;
}

Method * loadedMethod = 0;
Method * lastLoadedMethod = 0;

Method * load_method_in_class(JavaClass * jclass, char * name, char * descriptor)
{
	Method * method;
	
	if (loadedMethod) {
		method = loadedMethod;
		while (method) {
			if (method->jclass == jclass && strcmp(method->name, name) == 0 && strcmp(method->descriptor, descriptor) == 0)
				return method;
			method = method->next;
		}
	}

	method = 0;

	uint8_t * c8 = jclass->methods_count;
	uint8_t * c8_ = jclass->loaded_data;
	int offset = 0;
	int methodsCount = get_short(c8, offset);
	offset += 2;
	for (int i = 0; i < methodsCount; i++) {
		unsigned short flags = get_short(c8, offset);
		offset += 2;
		short nameIndex = get_short(c8, offset);
		int nameOffset = get_constant_from_pool_by_index(jclass, nameIndex-1) + 1;
		bool couldbe;
		if (utf8_compare(name, (char*)&c8_[nameOffset + 2], get_short(c8_, nameOffset)))
			couldbe = true;
		else
			couldbe = false;
		offset += 2;

		if (couldbe) {
			short descriptorIndex = get_short(c8, offset);
			int descriptorOffset = get_constant_from_pool_by_index(jclass, descriptorIndex-1)+1;
			char * c = (char*)&c8_[descriptorOffset + 2];
			if (!utf8_compare(descriptor, c, get_short(c8_, descriptorOffset))) {
				couldbe = false;
			}
		}
		offset += 2;

		if(couldbe) {
			method = malloc(sizeof(Method));
			method->jclass = jclass;
			method->flags = flags;
			method->bytecode = 0;
			method->bytecodeLength = 0;
		}

		int attributesCount = get_short(c8, offset);
		offset += 2;
		for (int j = 0; j < attributesCount; j++) {
			int attributesLength = 0;
			if(method) {
				nameIndex = get_short(c8, offset);
				nameOffset = get_constant_from_pool_by_index(jclass, nameIndex-1) + 1;
				if (get_short(c8_, nameOffset) && utf8_compare("Code", &c8_[nameOffset + 2], 4)) {
					offset += 2;
					int attributesLength = get_int(c8, offset);
					offset += 4;

					method->maxLocals = get_short(c8, offset + 2);

					method->bytecodeLength = get_int(c8, offset+4);

					if(!method->bytecodeLength) {
						fprintf(stderr, "Method %s %s has no bytecode!\n", name, descriptor);
						return 0;
					}

					method->bytecode = &c8[offset+8];
					
					offset += attributesLength;
				}
			}

			if (!attributesLength) {
				offset += 2;
				int attributesLength = get_int(c8, offset);
				offset += 4;
				offset += attributesLength;
			}
		}
		if (method)
			break;
	}
	if (method) {
		int len = strlen(name)+1;
		method->name = malloc(len);
		memcpy((char *)method->name, name, len);

		len = strlen(descriptor) + 1;
		method->descriptor = malloc(len);
		memcpy((char *)method->descriptor, descriptor, len);

		method->machineCode = 0;
		method->machineCodeLength = 0;

		const char * s = &method->descriptor[1];
		int sz = 0;
		while (*s != ')' && *s) {
			if (*s == 'I' || *s == 'F' || *s == 'B' || *s == 'C'
				|| *s == 'S' || *s == 'Z')
				sz += 4;
			else if (*s == 'D' || *s == 'J')
				sz += 8;
			else if (*s == 'L') {
				sz += 4;
				while (*s != ';' && *s != ')')
					s++;
			}
			else if (*s == '[') {
				sz += 4;
				while (*s == '[') s++;
				s++;
			}
			s++;
		}
		if(!(method->flags & 8)) { /* not static */
			sz += 4; // for 'this'
		}
		method->parametersSize = sz;

		if (!loadedMethod) {
			loadedMethod = method;
			method->next = 0;
			method->prev = 0;
			lastLoadedMethod = loadedMethod;
		}
		else {
			lastLoadedMethod->next = method;
			method->prev = lastLoadedMethod;
			method->next = 0;
			lastLoadedMethod = method;
		}

		return method;
	}
	return 0;
}

JavaClass * get_class(char * name)
{
	JavaClass * c = loadedClass;
	while (c) {
		if (strcmp(c->class_name, name) == 0)
			return c;
		c = c->next;
	}
	return 0;
}

char * java_class_utf8_to_c_string(uint8_t * utf8_len)
{
	int len = get_short(utf8_len, 0);
	char * s = malloc(len + 1);
	memcpy(s, &utf8_len[2], len);
	s[len] = 0;
	return s;
}

/* Finds the method in the given classes constant pool */
/* The method is not necessarily in the given jclass */
Method * load_method_in_class_by_index(JavaClass * jclass, unsigned short constant_pool_index)
{
	Method * method = 0;

	int offset = get_constant_from_pool_by_index(jclass, constant_pool_index);
	uint8_t * c8 = (uint8_t *)jclass->loaded_data;

	if (c8[offset] != 10)
		return 0;

	unsigned short classIndex = get_short(c8, offset + 1)-1;
	unsigned short nameAndTypeIndex = get_short(c8, offset + 3)-1;

	int classIndexOffset = get_constant_from_pool_by_index(jclass, classIndex);

	if (c8[classIndexOffset] != 7)
		return 0;

	unsigned short classNameIndex = get_short(c8, classIndexOffset + 1) - 1;
	int classNameIndexOffset = get_constant_from_pool_by_index(jclass, classNameIndex);
	
	char * s = java_class_utf8_to_c_string(&c8[classNameIndexOffset + 1]);

	JavaClass * c = get_class(s);
	if (!c) {
		int len = get_short(c8, classNameIndexOffset + 1);
		char * s1 = malloc(len + 6 + 1);
		memcpy(s1, s, len);
		memcpy(&s1[len], ".class", 7);
		c = load_class_(s1);
		if (!c)
			fprintf(stderr, "Could not load class file %s\n", s1);
		free(s1);
		if (!c)
			return 0;
	}
	free(s);

	int nameAndTypeOffset = get_constant_from_pool_by_index(jclass, nameAndTypeIndex);

	if (c8[nameAndTypeOffset] != 12)
		return 0;

	int nameIndex = get_short(c8, nameAndTypeOffset + 1) - 1;
	int descriptorIndex = get_short(c8, nameAndTypeOffset + 3) - 1;
	int nameOffset = get_constant_from_pool_by_index(jclass, nameIndex);
	int descriptorOffset = get_constant_from_pool_by_index(jclass, descriptorIndex);

	char * name = java_class_utf8_to_c_string(&c8[nameOffset + 1]);
	char * descriptor = java_class_utf8_to_c_string(&c8[descriptorOffset + 1]);
		
	Method * m = load_method_in_class(c, name, descriptor);
	free(name);
	free(descriptor);
	return m;
}

Field * get_field(JavaClass * jclass, int index)
{
	int offset = get_constant_from_pool_by_index(jclass, index);

	unsigned classIndex = get_short(jclass->loaded_data, offset + 1) - 1;
	unsigned nameAndTypeIndex = get_short(jclass->loaded_data, offset + 3) - 1;

	int classOffset = get_constant_from_pool_by_index(jclass, classIndex);
	unsigned classNameIndex = get_short(jclass->loaded_data, classOffset + 1) - 1;
	int classNameOffset = get_constant_from_pool_by_index(jclass, classNameIndex);

	char * s = java_class_utf8_to_c_string(&jclass->loaded_data[classNameOffset + 1]);
	JavaClass * c = get_class(s);
	if (!c) {
		int len = get_short(jclass->loaded_data, classNameOffset + 1);
		char * s1 = malloc(len + 6 + 1);
		memcpy(s1, s, len);
		memcpy(&s1[len], ".class", 7);
		c = load_class_(s1);
		free(s1);
	}

	int nameAndTypeOffset = get_constant_from_pool_by_index(jclass, nameAndTypeIndex);

	unsigned nameIndex = get_short(jclass->loaded_data, nameAndTypeOffset + 1) - 1;
	unsigned typeIndex = get_short(jclass->loaded_data, nameAndTypeOffset + 3) - 1;

	int nameOffset = get_constant_from_pool_by_index(jclass, nameIndex);
	int typeOffset = get_constant_from_pool_by_index(jclass, typeIndex);

	char * fname = java_class_utf8_to_c_string(&jclass->loaded_data[nameOffset + 1]);
	char * ftype = java_class_utf8_to_c_string(&jclass->loaded_data[typeOffset + 1]);

	Field * f = c->fields;
	while (f) {
		if (strcmp(f->name, fname) == 0 && strcmp(f->desc, ftype) == 0) {
			return f;
		}

		f = f->next;
	}

	return f;
}
