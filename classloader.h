#ifndef _classloader_h_
#define _classloader_h_

#include <string.h>
#include <stdint.h>

struct _Field;
typedef struct _Field
{
	struct _Field * prev;
	struct _Field * next;
	char * name;
	char * desc;
	// flags & 0x8 = static
	int flags;
	union {
		uint8_t data_8;
		uint16_t data_16;
		uint32_t data;
		uint64_t data_64;
		int instanceIndex; /* Index into JClassInstance->fields */
	};
} Field;

struct _JavaClass;
typedef struct _JavaClass
{
	struct _JavaClass * prev;
	struct _JavaClass * next;
	unsigned char * loaded_data;
	const char * class_name;
	unsigned char * access_flags;
	unsigned char * methods_count;
	Field * fields;
	Field * last_field;
	int intanceFieldsCount;
} JavaClass;

JavaClass * load_class (const char * filepath);

/*
	Searhes the JRE then searches the directory the main class was run in
*/
JavaClass * load_class_(const char * filename);

JavaClass * get_class(char * name);

// jclass->loaded_data[offset] (tag) should be 7
JavaClass * get_or_load_class_from_class_constant(JavaClass * jclass, int offset);
char * java_class_utf8_to_c_string(uint8_t * utf8_len);
int run_static_constructor(JavaClass * jclass);

struct _Method;
typedef struct _Method
{
	struct _Method * next;
	struct _Method * prev;

	const char * name;
	const char * descriptor;

	int bytecodeLength;
	uint8_t * bytecode;

	int machineCodeLength;
	uint8_t * machineCode;

	int maxLocals;
	/* Size in bytes of paramters, including 'this' parameter */
	int parametersSize;
	JavaClass * jclass;

	uint16_t flags;
} Method;

Method * load_method_in_class(JavaClass * jclass, char * name, char * descriptor);
Method * load_method_in_class_by_index(JavaClass * jclass, unsigned short constant_pool_index);
int get_constant_from_pool_by_index(JavaClass * jclass, unsigned short index);
Field * get_field(JavaClass * jclass, int index);


#endif
