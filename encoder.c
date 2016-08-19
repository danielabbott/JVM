#if defined(_WIN32) || defined(WIN32) || defined(__WIN32)
#define _CRT_SECURE_NO_DEPRECATE
#endif

#include "encoder.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "runtime.h"
#include "native.h"

void * alloc_executable_memory(size_t size);
void * resize_executable_memory(void * a, size_t size);

#define get_double(c8,offset) (( ((uint64_t)c8[offset]<<56) | ((uint64_t)c8[offset+1]<<48) | ((uint64_t)c8[offset+2]<<40) | ((uint64_t)c8[offset+3]<<32) | ((uint64_t)c8[offset+4]<<24) | ((uint64_t)c8[offset+5]<<16) | ((uint64_t)c8[offset+6]<<8) | ((uint64_t)c8[offset+7]) ))
#define get_long(c8,offset) (( ((uint64_t)c8[offset+4]<<56) | ((uint64_t)c8[offset+5]<<48) | ((uint64_t)c8[offset+6]<<40) | ((uint64_t)c8[offset+7]<<32) | ((uint64_t)c8[offset]<<24) | ((uint64_t)c8[offset+1]<<16) | ((uint64_t)c8[offset+2]<<8) | ((uint64_t)c8[offset+3]) ))
#define get_int(c8,offset) (( (c8[offset]<<24) | (c8[offset+1]<<16) | (c8[offset+2]<<8) | (c8[offset+3]) ))
#define get_short(c8,offset) (( (c8[offset]<<8) | c8[offset+1] ))

static inline int add_byte(int data_offset, int * dataSize, uint8_t * data, uint8_t b)
{
	if (data_offset >= *dataSize) {
		(*dataSize) += 4096;
		data = resize_executable_memory(data, *dataSize);
	}
	data[data_offset] = b;
	return data_offset + 1;
}

struct AddressReplacement;
typedef struct AddressReplacement{
	struct AddressReplacement * next;
	struct AddressReplacement * prev;
	unsigned int offsetOfReplacement; // (uint32_t)
	unsigned int newOffset;
} AddressReplacement;

extern JavaClass * stringJclass;

uint8_t * convert_bytecode(Method * method)
{
	if (method->machineCode)
		return method->machineCode;

	if (!method->bytecode || !method->bytecodeLength) {
		printf("method %s has no bytecode!\n", method->name);
		return 0;
	}

	if (method->maxLocals > 8) {
		printf("max locals is currently limited to 8. method->maxLocals = %u", method->maxLocals);
		return 0;
	}

	int len;
	int len2;
	char * s;
	FILE * file;
	if (method->name) {
		len = strlen(method->name);
		len2 = strlen(method->jclass->class_name);
		s = malloc(len2 + 1 + len + 10);
		strcpy(s, method->jclass->class_name);
		s[len2] = '.';
		strcpy(&s[len2 + 1], method->name);
		memcpy(&s[len2+1+len], ".bytecode", 10);

		char * s1 = s;
		while (*s1) {
			if (*s1 == '<' || *s1 == '>')
				*s1 = '_';
			if (*s1 == '/' || *s1 == '\\')
				*s1 = '.';
			s1++;
		}

		file = fopen(s, "w");
		free(s);
		if (file) {
			fwrite(method->bytecode, method->bytecodeLength, 1, file);
			fclose(file);
		}
	}

	uint32_t * byteCodeMachineCodeOffsets = malloc(method->bytecodeLength * 4);
	if(!byteCodeMachineCodeOffsets) {
		fprintf(stderr, "Out of memory converting method %s %s bytecode!\n", method->name, method->descriptor);
		return 0;
	}
	memset(byteCodeMachineCodeOffsets, 0xff, method->bytecodeLength * 4);

	AddressReplacement * addressReplacement = 0;
	AddressReplacement * lastAddressReplacement = 0;

	int dataSize = 4096;
	int bytecode_offset = 0;

	uint8_t * data = (uint8_t *)alloc_executable_memory(dataSize);
	method->machineCode = data;

	bytecode_offset = 0;
	int data_offset = 0;

	// push ebp
	data[data_offset++] = 0x55;

	// mov ebp, esp
	data[data_offset++] = 0x89;
	data[data_offset++] = 0xe5;
	
	/*

	Stack layout

	: local variables (parameters stored in here)
		variable 0 (this)
		variable 1
		variable 2 (maybe)
		variable 3 (maybe)

	return address
	ebp: esp: ebp

	*/

#define add_byte(x) (add_byte(data_offset, &dataSize, data, x))

#define add_dword(x)  \
		{data_offset = add_byte(((uint32_t)x) & 0xff); \
		data_offset = add_byte((((uint32_t)x) >> 8) & 0xff); \
		data_offset = add_byte((((uint32_t)x) >> 16) & 0xff); \
		data_offset = add_byte((((uint32_t)x) >> 24) & 0xff);}
	
#define push_dword(x) \
	{data_offset = add_byte(0x68); \
	add_dword(x);}

#define push_byte(x) \
	{data_offset = add_byte(0x6a); \
	data_offset = add_byte(x);}

	while (1) {
		uint8_t op = method->bytecode[bytecode_offset];
		byteCodeMachineCodeOffsets[bytecode_offset] = data_offset;
		bytecode_offset++;

		switch (op) {
		case 0: { /* nop */
			printf("nop\n");
			break;
		}
		case 0x01: { /* push null */
			push_byte(0);
			break;
		}
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		case 0x08: { /* push int constant to operand stack */
			push_byte((int)op - 3);
			break;
		}
		case 0x10: { /* push byte */
			char i = method->bytecode[bytecode_offset++];
			push_byte((int)i);
			break;
		}
		case 0x11: { /* push short */
			short i = ((method->bytecode[bytecode_offset] << 8) | method->bytecode[bytecode_offset + 1]);
			bytecode_offset += 2;
			push_dword(i);
			break;
		}
		case 0x12: /* push item fron constant pool */
		case 0x13: /* push item fron constant pool (wide index) */
		case 0x14: /* push long or double fron constant pool (wide index) */
		{
			unsigned short index;
			if (op == 0x12)
				index = (method->bytecode[bytecode_offset++]) - 1;
			else {
				index = ((method->bytecode[bytecode_offset] << 8) | method->bytecode[bytecode_offset + 1]) - 1;
				bytecode_offset += 2;
			}
			int offset = get_constant_from_pool_by_index(method->jclass, index);
			if (op < 0x14) {
				if (method->jclass->loaded_data[offset] == 3 || method->jclass->loaded_data[offset] == 4) {
					int i = get_int(method->jclass->loaded_data, offset + 1);
					push_dword(i);
				}
				else if (method->jclass->loaded_data[offset] == 8) { // constant string
					index = get_short(method->jclass->loaded_data, offset + 1) - 1;
					offset = get_constant_from_pool_by_index(method->jclass, index);

					JavaClass * jclass = get_class("java/lang/String");
					// void * eax = new_jclass(string);

					// push jclass
					push_dword((uintptr_t)jclass);

					// mov eax, new_jclass
					data_offset = add_byte(0xb8);
					add_dword(new_jclass);

					// call eax
					data_offset = add_byte(0xff);
					data_offset = add_byte(0xd0);

					// add esp, 4
					data_offset = add_byte(0x83);
					data_offset = add_byte(0xc4);
					data_offset = add_byte(4);

					// push eax
					data_offset = add_byte(0x50);

					/* call constructor */

					int length = get_short(method->jclass->loaded_data, offset + 1);
					JArrayInstance * a = new_array(5, length);
					for (int i = 0; i < length; i++) {
						a->data_c[i] = (uint16_t)method->jclass->loaded_data[offset + 3 + i];
					}

					Method * m = load_method_in_class(stringJclass, "<init>", "([C)V");

					push_dword(m);

					// mov eax, get_ptr_for_method
					data_offset = add_byte(0xb8);
					add_dword(get_ptr_for_method);

					// call eax
					data_offset = add_byte(0xff);
					data_offset = add_byte(0xd0);

					// add esp, 4
					data_offset = add_byte(0x83);
					data_offset = add_byte(0xc4);
					data_offset = add_byte(4);

					push_dword(a);

					int toPush = (m->maxLocals-2);
					for (int i = 0; i < toPush; i++)
						push_byte(0);

					// call eax
					data_offset = add_byte(0xff);
					data_offset = add_byte(0xd0);

					int sz = (m->maxLocals-1) * 4;

					// add esp, sz

					if (sz) {
						if (sz < 128) {
							data_offset = add_byte(0x83);
							data_offset = add_byte(0xc4);
							data_offset = add_byte(sz);
						}
						else {
							data_offset = add_byte(0x81);
							data_offset = add_byte(0xc4);
							data_offset = add_byte(sz & 0xff);
							data_offset = add_byte((sz >> 8) & 0xff);
							data_offset = add_byte((sz >> 16) & 0xff);
							data_offset = add_byte((sz >> 24) & 0xff);
						}
					}
				}
			}
			else {
				int64_t i = 0;
				if (method->jclass->loaded_data[offset] == 5) {
					i = get_long(method->jclass->loaded_data, offset + 1);
				}
				else if (method->jclass->loaded_data[offset] == 6) {
					i = get_double(method->jclass->loaded_data, offset + 1);
				}

				push_dword((uint32_t)(i >> 32));
				push_dword(i);
			}
			break;
		}

		case 0x19: /* aload */
		case 0x15: { /* iload */
			unsigned char index = method->bytecode[bytecode_offset++];

			// push DWORD[ebp + (method->maxLocals - i + 1)*4]
			data_offset = add_byte(0xff);
			data_offset = add_byte(0x75);
			data_offset = add_byte((method->maxLocals - index + 1) * 4);
			break;
		}

		case 0x1e:
		case 0x1f:
		case 0x20:
		case 0x21: /* lload */
		case 0x18:
		case 0x26:
		case 0x27:
		case 0x28:
		case 0x29: { /* dload */
			int i;
			if (op >= 0x26)
				i = op - 0x26;
			else if (op >= 0x1e)
				i = op - 0x1e;
			else
				i = method->bytecode[bytecode_offset++];

			// push DWORD[ebp + (method->maxLocals - i + 1)*4]
			data_offset = add_byte(0xff);
			data_offset = add_byte(0x75);
			data_offset = add_byte((method->maxLocals - i + 1)*4);

			// push DWORD[ebp + (method->maxLocals - (i+1) + 1)*4]
			data_offset = add_byte(0xff);
			data_offset = add_byte(0x75);
			data_offset = add_byte((method->maxLocals - i + 1)*4);
			break;
		}

		case 0x1a:
		case 0x1b:
		case 0x1c:
		case 0x1d: /* load int from local variable */

		case 0x22:
		case 0x23:
		case 0x24:
		case 0x25: /* Load float from local variable */

		case 0x2a:
		case 0x2b:
		case 0x2c:
		case 0x2d: {  /* Load reference from local variable */
			int i;
			if(op >= 0x2a)
				i = op - 0x2a;
			else if (op >= 0x22)
				i = op - 0x22;
			else
				i = op - 0x1a;

			// push DWORD PTR [ebp + (method->maxLocals - i + 1)*4]
			data_offset = add_byte(0xff);
			data_offset = add_byte(0x75);
			data_offset = add_byte((method->maxLocals - i + 1)*4);
			break;
		}

		case 0x34: { /* caload */
			// mov eax, caload
			data_offset = add_byte(0xb8);
			add_dword(caload);

			// call eax
			data_offset = add_byte(0xff);
			data_offset = add_byte(0xd0);

			// add esp, 8
			data_offset = add_byte(0x83);
			data_offset = add_byte(0xc4);
			data_offset = add_byte(8);

			// push eax
			data_offset = add_byte(0x50);
			break;
		}

		case 0x3a: /* astore */
		case 0x36: { /* istore */
			unsigned char index = method->bytecode[bytecode_offset++];

			// pop DWORD[ebp + (method->maxLocals - i + 1)*4]
			data_offset = add_byte(0x8f);
			data_offset = add_byte(0x45);
			data_offset = add_byte((method->maxLocals - index + 1) * 4);
			break;
		}

		case 0x39:
		case 0x47:
		case 0x48:
		case 0x49:
		case 0x4a: { /* Store double into local variable */
			int i;
			if (op >= 0x47)
				i = op - 0x47;
			else
				i = method->bytecode[bytecode_offset++];

			// pop eax
			data_offset = add_byte(0x58);

			// mov DWORD[ebp + (method->maxLocals - (i+1) + 1)*4], eax
			data_offset = add_byte(0x89);
			data_offset = add_byte(0x45);
			data_offset = add_byte((method->maxLocals - (i+1) + 1)*4);

			// pop DWORD[ebp + (method->maxLocals - i + 1)*4]
			data_offset = add_byte(0x8f);
			data_offset = add_byte(0x45);
			data_offset = add_byte((method->maxLocals - i + 1) * 4);
			break;
		}

		case 0x3b:
		case 0x3c:
		case 0x3d:
		case 0x3e: /* Store int into local variable */

		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46: /* Store float into local variable */

		case 0x4b:
		case 0x4c:
		case 0x4d:
		case 0x4e: { /* Store reference into local variable */
			int i;
			if(op >= 0x4b)
				i = op - 0x4b;
			else if (op >= 0x43)
				i = op - 0x43;
			else
				i = op - 0x3b;

			// pop DWORD[ebp + (method->maxLocals - i + 1)*4]
			data_offset = add_byte(0x8f);
			data_offset = add_byte(0x45);
			data_offset = add_byte((method->maxLocals - i + 1)*4);
			break;
		}

		case 0x4f: { /* iastore Store into int array */
			// mov eax, iastore
			data_offset = add_byte(0xb8);
			add_dword(iastore);

			// call eax
			data_offset = add_byte(0xff);
			data_offset = add_byte(0xd0);

			// add esp, 12
			data_offset = add_byte(0x83);
			data_offset = add_byte(0xc4);
			data_offset = add_byte(12);
			break;
		}
		case 0x53: { /* aastore Store into reference array */
			// mov eax, aastore
			data_offset = add_byte(0xb8);
			add_dword(aastore);

			// call eax
			data_offset = add_byte(0xff);
			data_offset = add_byte(0xd0);

			// add esp, 12
			data_offset = add_byte(0x83);
			data_offset = add_byte(0xc4);
			data_offset = add_byte(12);
			break;
		}
		case 0x54: { /* bastore Store into byte array */
			// mov eax, bastore
			data_offset = add_byte(0xb8);
			add_dword(bastore);

			// call eax
			data_offset = add_byte(0xff);
			data_offset = add_byte(0xd0);

			// add esp, 12
			data_offset = add_byte(0x83);
			data_offset = add_byte(0xc4);
			data_offset = add_byte(12);
			break;
		}
		case 0x55: { /* castore Store into char array */
			// mov eax, castore
			data_offset = add_byte(0xb8);
			add_dword(castore);

			// call eax
			data_offset = add_byte(0xff);
			data_offset = add_byte(0xd0);

			// add esp, 12
			data_offset = add_byte(0x83);
			data_offset = add_byte(0xc4);
			data_offset = add_byte(12);
			break;
		}
		case 0x57: { /* pop */
			// add esp, 4
			data_offset = add_byte(0x83);
			data_offset = add_byte(0xc4);
			data_offset = add_byte(4);
			break;
		}
		case 0x59: { /* Duplicate the top operand stack value */
			// push [esp]
			data_offset = add_byte(0xff);
			data_offset = add_byte(0x34);
			data_offset = add_byte(0x24);
			break;
		}

		case 0x5c: { /* dup2 */
			// push dword ptr [esp+4]
			data_offset = add_byte(0xff);
			data_offset = add_byte(0x74);
			data_offset = add_byte(0x24);
			data_offset = add_byte(0x04);

			// push dword ptr [esp+4]
			data_offset = add_byte(0xff);
			data_offset = add_byte(0x74);
			data_offset = add_byte(0x24);
			data_offset = add_byte(0x04);
			break;
		}

		case 0x60: { /* iadd */
			// pop eax
			data_offset = add_byte(0x58);

			// add dword ptr [esp], eax
			data_offset = add_byte(0x01);
			data_offset = add_byte(0x04);
			data_offset = add_byte(0x24);

			break;
		}

		case 0x61: { /* ladd */
			// pop edx (low dword)
			data_offset = add_byte(0x5a);

			// pop eax (high dword)
			data_offset = add_byte(0x58);

			// add edx, dword ptr [esp]
			data_offset = add_byte(0x03);
			data_offset = add_byte(0x14);
			data_offset = add_byte(0x24);

			// jno no_overflow
			data_offset = add_byte(0x71);
			data_offset = add_byte(1);

			// inc eax
			data_offset = add_byte(0x40);

			// no_overflow:

			// add eax, dword ptr [esp+4]
			data_offset = add_byte(0x03);
			data_offset = add_byte(0x44);
			data_offset = add_byte(0x24);
			data_offset = add_byte(0x04);

			// mov dword ptr [esp], eax
			data_offset = add_byte(0x89);
			data_offset = add_byte(0x04);
			data_offset = add_byte(0x24);

			// mov dword ptr [esp+4], edx
			data_offset = add_byte(0x89);
			data_offset = add_byte(0x54);
			data_offset = add_byte(0x24);
			data_offset = add_byte(0x04);

			break;
		}

		case 0x62: { /* fadd */
			// fld dword ptr [esp+4]
			data_offset = add_byte(0xd9);
			data_offset = add_byte(0x44);
			data_offset = add_byte(0x24);
			data_offset = add_byte(0x04);

			// fadd dword ptr [esp]
			data_offset = add_byte(0xd8);
			data_offset = add_byte(0x04);
			data_offset = add_byte(0x24);

			// add esp, 4
			data_offset = add_byte(0x83);
			data_offset = add_byte(0xc4);
			data_offset = add_byte(0x04);

			// fst dword ptr [esp]
			data_offset = add_byte(0xd9);
			data_offset = add_byte(0x14);
			data_offset = add_byte(0x24);

			// fwait
			data_offset = add_byte(0x9b);
			break;
		}
		case 0x63: { /* dadd */
			// fld qword ptr [esp+4]
			data_offset = add_byte(0xdd);
			data_offset = add_byte(0x44);
			data_offset = add_byte(0x24);
			data_offset = add_byte(0x04);

			// fadd qword ptr [esp]
			data_offset = add_byte(0xdc);
			data_offset = add_byte(0x04);
			data_offset = add_byte(0x24);

			// add esp, 4
			data_offset = add_byte(0x83);
			data_offset = add_byte(0xc4);
			data_offset = add_byte(0x04);

			// fst qword ptr [esp]
			data_offset = add_byte(0xdd);
			data_offset = add_byte(0x14);
			data_offset = add_byte(0x24);

			// fwait
			data_offset = add_byte(0x9b);
			break;
		}
		case 0x64: { /* isub */
			// pop eax
			data_offset = add_byte(0x58);

			// sub dword ptr [esp], eax
			data_offset = add_byte(0x29);
			data_offset = add_byte(0x04);
			data_offset = add_byte(0x24);

			break;
		}
		case 0x66: { /* fsub */
			// fld dword ptr [esp+4]
			data_offset = add_byte(0xd9);
			data_offset = add_byte(0x44);
			data_offset = add_byte(0x24);
			data_offset = add_byte(0x04);

			// fsub dword ptr [esp]
			data_offset = add_byte(0xd8);
			data_offset = add_byte(0x24);
			data_offset = add_byte(0x24);

			// add esp, 4
			data_offset = add_byte(0x83);
			data_offset = add_byte(0xc4);
			data_offset = add_byte(0x04);

			// fst dword ptr [esp]
			data_offset = add_byte(0xd9);
			data_offset = add_byte(0x14);
			data_offset = add_byte(0x24);

			// fwait
			data_offset = add_byte(0x9b);
			break;
		}
		case 0x67: { /* dsub */
			// fld qword ptr [esp+8]
			data_offset = add_byte(0xdd);
			data_offset = add_byte(0x44);
			data_offset = add_byte(0x24);
			data_offset = add_byte(0x08);

			// fsub qword ptr [esp]
			data_offset = add_byte(0xdc);
			data_offset = add_byte(0x24);
			data_offset = add_byte(0x24);

			// add esp, 8
			data_offset = add_byte(0x83);
			data_offset = add_byte(0xc4);
			data_offset = add_byte(0x08);

			// fst qword ptr [esp]
			data_offset = add_byte(0xdd);
			data_offset = add_byte(0x14);
			data_offset = add_byte(0x24);

			// fwait
			data_offset = add_byte(0x9b);
			break;
		}
		case 0x68: { /* imul */
			// pop eax
			data_offset = add_byte(0x58);

			// pop edx
			data_offset = add_byte(0x5a);

			// imul edx
			data_offset = add_byte(0xf7);
			data_offset = add_byte(0xea);

			// push eax
			data_offset = add_byte(0x50);
			break;
		}
		case 0x6b: { /* dmul */
			// fld qword ptr [esp+8]
			data_offset = add_byte(0xdd);
			data_offset = add_byte(0x44);
			data_offset = add_byte(0x24);
			data_offset = add_byte(0x08);

			// fmul qword ptr [esp]
			data_offset = add_byte(0xdc);
			data_offset = add_byte(0x0c);
			data_offset = add_byte(0x24);

			// add esp, 8
			data_offset = add_byte(0x83);
			data_offset = add_byte(0xc4);
			data_offset = add_byte(0x08);

			// fst qword ptr [esp]
			data_offset = add_byte(0xdd);
			data_offset = add_byte(0x14);
			data_offset = add_byte(0x24);

			// fwait
			data_offset = add_byte(0x9b);
			break;
		}
		case 0x6c: { /* idiv */
			// pop ecx
			data_offset = add_byte(0x59);

			// xor edx, edx
			data_offset = add_byte(0x31);
			data_offset = add_byte(0xd2);

			// pop eax
			data_offset = add_byte(0x58);

			// idiv ecx
			data_offset = add_byte(0xf7);
			data_offset = add_byte(0xf9);

			// push eax
			data_offset = add_byte(0x50);
			break;
		}
		case 0x6a: { /* fmul */
			// fld dword ptr [esp+4]
			data_offset = add_byte(0xd9);
			data_offset = add_byte(0x44);
			data_offset = add_byte(0x24);
			data_offset = add_byte(0x04);

			// fmul dword ptr [esp]
			data_offset = add_byte(0xd8);
			data_offset = add_byte(0x0c);
			data_offset = add_byte(0x24);

			// add esp, 4
			data_offset = add_byte(0x83);
			data_offset = add_byte(0xc4);
			data_offset = add_byte(0x04);

			// fst dword ptr [esp]
			data_offset = add_byte(0xd9);
			data_offset = add_byte(0x14);
			data_offset = add_byte(0x24);

			// fwait
			data_offset = add_byte(0x9b);
			break;
		}
		case 0x6e: { /* fdiv */
			// fld dword ptr [esp+4]
			data_offset = add_byte(0xd9);
			data_offset = add_byte(0x44);
			data_offset = add_byte(0x24);
			data_offset = add_byte(0x04);

			// fdiv dword ptr [esp]
			data_offset = add_byte(0xd8);
			data_offset = add_byte(0x34);
			data_offset = add_byte(0x24);

			// add esp, 4
			data_offset = add_byte(0x83);
			data_offset = add_byte(0xc4);
			data_offset = add_byte(0x04);

			// fst dword ptr [esp]
			data_offset = add_byte(0xd9);
			data_offset = add_byte(0x14);
			data_offset = add_byte(0x24);

			// fwait
			data_offset = add_byte(0x9b);
			break;
		}
		case 0x6f: { /* ddiv */
			// fld qword ptr [esp+8]
			data_offset = add_byte(0xdd);
			data_offset = add_byte(0x44);
			data_offset = add_byte(0x24);
			data_offset = add_byte(0x08);

			// fdiv qword ptr [esp]
			data_offset = add_byte(0xdc);
			data_offset = add_byte(0x34);
			data_offset = add_byte(0x24);

			// add esp, 8
			data_offset = add_byte(0x83);
			data_offset = add_byte(0xc4);
			data_offset = add_byte(0x08);

			// fst qword ptr [esp]
			data_offset = add_byte(0xdd);
			data_offset = add_byte(0x14);
			data_offset = add_byte(0x24);

			// fwait
			data_offset = add_byte(0x9b);
			break;
		}

		case 0x70: { /* remainder */
			// pop ecx
			data_offset = add_byte(0x59);

			// xor edx, edx
			data_offset = add_byte(0x31);
			data_offset = add_byte(0xd2);

			// pop eax
			data_offset = add_byte(0x58);

			// idiv ecx
			data_offset = add_byte(0xf7);
			data_offset = add_byte(0xf9);

			// push edx
			data_offset = add_byte(0x52);

			break;
		}

		case 0x74: { /* make int negative */
			// not dword ptr [esp]
			data_offset = add_byte(0xf7);
			data_offset = add_byte(0x14);
			data_offset = add_byte(0x24);

			// inc dword ptr [esp]
			data_offset = add_byte(0xff);
			data_offset = add_byte(0x04);
			data_offset = add_byte(0x24);

			break;
		}

		case 0x78: { /* shift left */
			// pop ecx
			data_offset = add_byte(0x59);

			// and ecx, 0x1f
			data_offset = add_byte(0x83);
			data_offset = add_byte(0xe1);
			data_offset = add_byte(0x1f);

			// pop eax
			data_offset = add_byte(0x58);

			// shl eax, cl
			data_offset = add_byte(0xd3);
			data_offset = add_byte(0xe0);

			// push eax
			data_offset = add_byte(0x50);
			break;
		}

		case 0x84: { /* iinc */
			unsigned char index = method->bytecode[bytecode_offset++];
			unsigned char constant = method->bytecode[bytecode_offset++];

			// add DWORD[ebp + (method->maxLocals - i + 1)*4], const
			data_offset = add_byte(0x83);
			data_offset = add_byte(0x45);
			data_offset = add_byte((method->maxLocals - index + 1) * 4);
			data_offset = add_byte(constant);
			break;
		}

		case 0x85: { /* i2l  (int to long) */
			// pop eax
			data_offset = add_byte(0x58);

			// cdq
			data_offset = add_byte(0x99);

			// push edx
			data_offset = add_byte(0x52);
			// push eax
			data_offset = add_byte(0x50);
			break;
		}

		case 0x8d: { /* f2d */
			// fld dword ptr [esp]
			data_offset = add_byte(0xd9);
			data_offset = add_byte(0x04);
			data_offset = add_byte(0x24);

			// sub esp, 4
			data_offset = add_byte(0x83);
			data_offset = add_byte(0xec);
			data_offset = add_byte(0x04);

			// fst qword ptr [esp]
			data_offset = add_byte(0xdd);
			data_offset = add_byte(0x14);
			data_offset = add_byte(0x24);

			// fwait
			data_offset = add_byte(0x9b);
			break;
		}

		case 0x92: { /* int to char */
			// and dword ptr [esp], 0xffff
			data_offset = add_byte(0x81);
			data_offset = add_byte(0x24);
			data_offset = add_byte(0x24);
			data_offset = add_byte(0xff);
			data_offset = add_byte(0xff);
			data_offset = add_byte(0);
			data_offset = add_byte(0);

			break;
		}

		case 0x99: /* if i == 0 */
		case 0x9a: /* if i != 0 */
		case 0x9b: /* if i < 0 */
		case 0x9c: /* if i >= 0 */
		case 0x9d: /* if i > 0 */
		case 0x9e: { /* if i <= 0 */
			short offset = (short) ((method->bytecode[bytecode_offset] << 8) | method->bytecode[bytecode_offset + 1]);

			int newOffset = bytecode_offset-1 + offset;

			bytecode_offset += 2;

			// pop eax
			data_offset = add_byte(0x58);

			if (op == 0x99 || op == 0x9a) {
				// test eax, eax
				data_offset = add_byte(0x85);
				data_offset = add_byte(0xc0);

				if (op == 0x99) {
					// je
					data_offset = add_byte(0x0f);
					data_offset = add_byte(0x84);
				}
				else if (op == 0x9a) {
					// jnz
					data_offset = add_byte(0x0f);
					data_offset = add_byte(0x85);
				}
			}
			else {
				// cmp eax, 0
				data_offset = add_byte(0x83);
				data_offset = add_byte(0xf8);
				data_offset = add_byte(0);

				if (op == 0x9b) {
					// jl
					data_offset = add_byte(0x0f);
					data_offset = add_byte(0x8c);
				} else if (op == 0x9c) {
					// jge
					data_offset = add_byte(0x0f);
					data_offset = add_byte(0x8d);
				}
				else if (op == 0x9d) {
					// jg
					data_offset = add_byte(0x0f);
					data_offset = add_byte(0x8f);
				}
				else if (op == 0x9e) {
					// jle
					data_offset = add_byte(0x0f);
					data_offset = add_byte(0x8e);
				}
			}

			if (offset < 0) {
				int difference = -(int)(data_offset + 4 - byteCodeMachineCodeOffsets[newOffset]);
				add_dword(difference);
			}
			else {
				AddressReplacement * replace = malloc(sizeof(AddressReplacement));
				replace->offsetOfReplacement = data_offset;
				replace->newOffset = newOffset;
				replace->next = 0;
				if (addressReplacement) {
					replace->prev = lastAddressReplacement;
					lastAddressReplacement->next = replace;
					lastAddressReplacement = replace;
				}
				else {
					addressReplacement = lastAddressReplacement = replace;
					replace->prev = 0;
				}
				add_dword(0xffffffff);
			}

			break;
		}

		case 0x9f: /* if_icmpeq */
		case 0xa0: /* if_icmpne */
		case 0xa1: /* if_icmplt */
		case 0xa2: /* if_icmpge */
		case 0xa3: /* if_icmpgt */
		case 0xa4: { /* if_icmple */
			short offset = (short)((method->bytecode[bytecode_offset] << 8) | method->bytecode[bytecode_offset + 1]);

			int newOffset = bytecode_offset - 1 + offset;

			bytecode_offset += 2;

			// pop edx
			data_offset = add_byte(0x5a);

			// pop eax
			data_offset = add_byte(0x58);
			 
			// cmp eax, edx
			data_offset = add_byte(0x39);
			data_offset = add_byte(0xd0);

			if (op == 0x9f) {
				// je
				data_offset = add_byte(0x0f);
				data_offset = add_byte(0x84);
			}
			else if (op == 0xa0) {
				// jne
				data_offset = add_byte(0x0f);
				data_offset = add_byte(0x85);
			}
			else if (op == 0xa1) {
				// jl
				data_offset = add_byte(0x0f);
				data_offset = add_byte(0x8c);
			}
			else if (op == 0xa2) {
				// jge
				data_offset = add_byte(0x0f);
				data_offset = add_byte(0x8d);
			}
			else if (op == 0xa3) {
				// jg
				data_offset = add_byte(0x0f);
				data_offset = add_byte(0x8f);
			}
			else if (op == 0xa4) {
				// jle
				data_offset = add_byte(0x0f);
				data_offset = add_byte(0x8e);
			}
			

			if (offset < 0) {
				int difference = -(int)(data_offset + 4 - byteCodeMachineCodeOffsets[newOffset]);
				add_dword(difference);
			}
			else {
				AddressReplacement * replace = malloc(sizeof(AddressReplacement));
				replace->offsetOfReplacement = data_offset;
				replace->newOffset = newOffset;
				replace->next = 0;
				if (addressReplacement) {
					replace->prev = lastAddressReplacement;
					lastAddressReplacement->next = replace;
					lastAddressReplacement = replace;
				}
				else {
					addressReplacement = lastAddressReplacement = replace;
					replace->prev = 0;
				}
				add_dword(0xffffffff);
			}

			break;
		}

		case 0xa7: { /* goto */
			short offset = (short)((method->bytecode[bytecode_offset] << 8) | method->bytecode[bytecode_offset + 1]);

			int newOffset = bytecode_offset - 1 + offset;

			bytecode_offset += 2;

			// jmp
			data_offset = add_byte(0xe9);			

			if (offset < 0) {
				int difference = -(int)(data_offset + 4 - byteCodeMachineCodeOffsets[newOffset]);
				add_dword(difference);
			}
			else {
				AddressReplacement * replace = malloc(sizeof(AddressReplacement));
				replace->offsetOfReplacement = data_offset;
				replace->newOffset = newOffset;
				replace->next = 0;
				if (addressReplacement) {
					replace->prev = lastAddressReplacement;
					lastAddressReplacement->next = replace;
					lastAddressReplacement = replace;
				}
				else {
					addressReplacement = lastAddressReplacement = replace;
					replace->next = 0;
				}
				add_dword(0xffffffff);
			}

			break;
		}

		case 0xac: /* ireturn */
		case 0xad: /* lreturn */
		case 0xb0: /* areturn */
		case 0xb1: { /* return */
			if (op == 0xac || op == 0xad || op == 0xb0) {
				// pop eax
				data_offset = add_byte(0x58);
			}

			if (op == 0xad) {
				// pop edx
				data_offset = add_byte(0x5a);
			}

			// mov esp, ebp
			data_offset = add_byte(0x89);
			data_offset = add_byte(0xec);

			// pop ebp
			data_offset = add_byte(0x5d);

			// ret
			data_offset = add_byte(0xc3);
			break;
		}
		case 0xb2: { /* get static field in class */
			unsigned index = ((method->bytecode[bytecode_offset] << 8) | method->bytecode[bytecode_offset + 1]) - 1;
			bytecode_offset += 2;

			Field * field = get_field(method->jclass, index);
			bool bits64 = field->desc[0] == 'J' || field->desc[0] == 'D';

			if (bits64) {
				// push dword ptr [&field->data + 4]
				data_offset = add_byte(0xff);
				data_offset = add_byte(0x35);
				add_dword((uintptr_t)&field->data + 4);
			}

			// push dword ptr [&field->data]
			data_offset = add_byte(0xff);
			data_offset = add_byte(0x35);
			add_dword((uintptr_t)&field->data);
			break;
		}
		case 0xb3: { /* Set static field in class */
			unsigned index = ((method->bytecode[bytecode_offset] << 8) | method->bytecode[bytecode_offset + 1]) - 1;
			bytecode_offset += 2;

			Field * field = get_field(method->jclass, index);
			bool bits64 = field->desc[0] == 'J' || field->desc[0] == 'D';

			// pop dword ptr [&field->data]
			data_offset = add_byte(0x8f);
			data_offset = add_byte(0x05);
			add_dword((uintptr_t)&field->data);

			if (bits64) {
				// pop dword ptr [&field->data + 4]
				data_offset = add_byte(0x8f);
				data_offset = add_byte(0x05);
				add_dword((uintptr_t)&field->data + 4);
			}

			break;
		}
		case 0xb4: { /* getfield  - get field in object */
			unsigned index = ((method->bytecode[bytecode_offset] << 8) | method->bytecode[bytecode_offset + 1]) - 1;
			bytecode_offset += 2;
			Field * field = get_field(method->jclass, index);
			if (field->flags & 8) {
				fprintf(stderr, "Using wrong opcode for setting a static field. Use 0xb3 not 0xb5");
				free(byteCodeMachineCodeOffsets);
				return 0;
			}
			bool bits64 = field->desc[0] == 'J' || field->desc[0] == 'D';

			// pop eax (objectref)
			data_offset = add_byte(0x58);

			// mov eax, [eax+4]
			data_offset = add_byte(0x8b);
			data_offset = add_byte(0x40);
			data_offset = add_byte(0x04);

			if(bits64) {
				//  push [eax+index*8+4]
				data_offset = add_byte(0xff);
				data_offset = add_byte(0xb0);
				add_dword(field->instanceIndex * 8 + 4);
			}

			//  push [eax+index*8]
			data_offset = add_byte(0xff);
			data_offset = add_byte(0xb0);
			add_dword(field->instanceIndex * 8);

			break;
		}
		case 0xb5: { /* putfield - Set field in object */
			unsigned index = ((method->bytecode[bytecode_offset] << 8) | method->bytecode[bytecode_offset + 1]) - 1;
			bytecode_offset += 2;
			Field * field = get_field(method->jclass, index);
			if (field->flags & 8) {
				fprintf(stderr, "Using wrong opcode for setting a static field. Use 0xb3 not 0xb5");
				free(byteCodeMachineCodeOffsets);
				return 0;
			}
			bool bits64 = field->desc[0] == 'J' || field->desc[0] == 'D';

			// pop edx (value)
			data_offset = add_byte(0x5a);

			if (field->desc[0] == 'J' || field->desc[0] == 'D') {
				// pop ecx (value)
				data_offset = add_byte(0x59);
			}

			// pop eax (objectref)
			data_offset = add_byte(0x58);

			// mov eax, [eax+4]
			data_offset = add_byte(0x8b);
			data_offset = add_byte(0x40);
			data_offset = add_byte(0x04);


			// mov [eax+index*4], edx
			int o = field->instanceIndex*4;
			if (o > 127) {
				data_offset = add_byte(0x89);
				data_offset = add_byte(0x90);
				add_dword(o);
			}
			else {
				data_offset = add_byte(0x89);
				data_offset = add_byte(0x50);
				data_offset = add_byte(o);
			}

			if (field->desc[0] == 'J' || field->desc[0] == 'D') {
				// mov [eax+index*8+4], ecx
				int o = field->instanceIndex * 8 + 4;
				if (o > 127) {
					data_offset = add_byte(0x89);
					data_offset = add_byte(0x88);
					add_dword(o);
				}
				else {
					data_offset = add_byte(0x89);
					data_offset = add_byte(0x48);
					data_offset = add_byte(o);
				}
			}

			break;
		}
		case 0xb6: /* invoke method */
		case 0xb7: /* invoke special method */
		case 0xb8: { /* run static method */
			unsigned index = ((method->bytecode[bytecode_offset] << 8) | method->bytecode[bytecode_offset + 1]) - 1;

			Method * m = load_method_in_class_by_index(method->jclass, index);
			if (!m) {
				printf("Could not load static method (index %i) referred to in class %s, called statically in %s\n", index, method->jclass->class_name, method->name);
				free(byteCodeMachineCodeOffsets);
				return 0;
			}

			bytecode_offset += 2;
			if (!(m->flags & 0x100)) { // not native
				if (m != method) {

					// push m
					push_dword(m);

					// mov eax, get_ptr_for_method
					data_offset = add_byte(0xb8);
					add_dword(get_ptr_for_method);

					// call eax
					data_offset = add_byte(0xff);
					data_offset = add_byte(0xd0);

					// add esp, 4
					data_offset = add_byte(0x83);
					data_offset = add_byte(0xc4);
					data_offset = add_byte(4);
				}

				int toPush = m->maxLocals - m->parametersSize / 4;
				for(int i = 0; i < toPush; i++)
					push_byte(0);

				if (m != method) {
					// call eax
					data_offset = add_byte(0xff);
					data_offset = add_byte(0xd0);
				}
				else {
					// call xxx
					int32_t difference = -((int32_t)(data_offset + 5));

					data_offset = add_byte(0xe8);
					add_dword(difference);
				}

				int sz = m->maxLocals * 4;

				// add esp, sz

				if (sz < 128) {
					data_offset = add_byte(0x83);
					data_offset = add_byte(0xc4);
					data_offset = add_byte(sz);
				}
				else {
					data_offset = add_byte(0x81);
					data_offset = add_byte(0xc4);
					data_offset = add_byte(sz & 0xff);
					data_offset = add_byte((sz >> 8) & 0xff);
					data_offset = add_byte((sz >> 16) & 0xff);
					data_offset = add_byte((sz >> 24) & 0xff);
				}

				const char * s1 = m->descriptor;
				while (*s1) s1++;
				if (s1[-1] != 'V') {
					bool bits64 = s1[-1] == 'D' || s1[-1] == 'J';
					if (bits64) {
						// push edx
						data_offset = add_byte(0x52);
					}
					// push eax
					data_offset = add_byte(0x50);
				}
			}
			else { /* native function */
				// TODO native instance methods - push local variable 0

				// push m->jclass
				if (op == 0xb8) { /* static method */
					push_dword((uintptr_t)m->jclass);
				} 
				else {
					// push DWORD[ebp + (method->maxLocals - 0 + 1)*4]
					data_offset = add_byte(0xff);
					data_offset = add_byte(0x75);
					data_offset = add_byte((method->maxLocals + 1) * 4);
				}

				NativeFunction * n = get_native_function(m->name, m->descriptor, op != 0xb8);
				if (!n) {
					printf("Native function %s %s does not exist! Called from %s in %s\n", m->name, m->descriptor, method->name, method->jclass->class_name);
					free(byteCodeMachineCodeOffsets);
					return 0;
				}

				// mov eax, n->function
				data_offset = add_byte(0xb8);
				add_dword(n->function);

				// call eax
				data_offset = add_byte(0xff);
				data_offset = add_byte(0xd0);

				const char * s = &m->descriptor[1];
				int sz = 4;
				while (*s != ')') {
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
					}
					s++;
				}

				// add esp, sz

				if (sz < 128) {
					data_offset = add_byte(0x83);
					data_offset = add_byte(0xc4);
					data_offset = add_byte(sz);
				}
				else {
					data_offset = add_byte(0x81);
					data_offset = add_byte(0xc4);
					data_offset = add_byte(sz & 0xff);
					data_offset = add_byte((sz >> 8) & 0xff);
					data_offset = add_byte((sz >> 16) & 0xff);
					data_offset = add_byte((sz >> 24) & 0xff);
				}

			}

			break;
		}
		case 0xbb: { /* new */
			unsigned index = ((method->bytecode[bytecode_offset] << 8) | method->bytecode[bytecode_offset + 1]) - 1;
			bytecode_offset += 2;

			int offset = get_constant_from_pool_by_index(method->jclass, index);
			uint8_t tag = method->jclass->loaded_data[offset];
			if (tag == 7) { /* class */
				JavaClass * jclass = get_or_load_class_from_class_constant(method->jclass, offset);
				if (!jclass) {
					free(byteCodeMachineCodeOffsets);
					return 0;
				}
				// void * eax = new_jclass(string);

				// push jclass
				push_dword((uintptr_t)jclass);

				// mov eax, new_jclass
				data_offset = add_byte(0xb8);
				add_dword(new_jclass);

				// call eax
				data_offset = add_byte(0xff);
				data_offset = add_byte(0xd0);

				// add esp, 4
				data_offset = add_byte(0x83);
				data_offset = add_byte(0xc4);
				data_offset = add_byte(4);

				// push eax
				data_offset = add_byte(0x50);

				/* call constructor */

				Method * m = load_method_in_class(stringJclass, "<init>", "()V");
				if (m) {
					push_dword(m);

					// mov eax, get_ptr_for_method
					data_offset = add_byte(0xb8);
					add_dword(get_ptr_for_method);

					// call eax
					data_offset = add_byte(0xff);
					data_offset = add_byte(0xd0);

					// add esp, 4
					data_offset = add_byte(0x83);
					data_offset = add_byte(0xc4);
					data_offset = add_byte(4);

					int toPush = (m->maxLocals - 1);
					if (toPush > 0) {
						for (int i = 0; i < toPush; i++)
							push_byte(0);
					}

					// call eax
					data_offset = add_byte(0xff);
					data_offset = add_byte(0xd0);

					int sz = (m->maxLocals-1) * 4;

					// add esp, sz

					if (sz < 128) {
						data_offset = add_byte(0x83);
						data_offset = add_byte(0xc4);
						data_offset = add_byte(sz);
					}
					else {
						data_offset = add_byte(0x81);
						data_offset = add_byte(0xc4);
						data_offset = add_byte(sz & 0xff);
						data_offset = add_byte((sz >> 8) & 0xff);
						data_offset = add_byte((sz >> 16) & 0xff);
						data_offset = add_byte((sz >> 24) & 0xff);
					}
				}
			}
			else {
				printf("'new' opcode using constant with tag %u\n", tag);
			}

			break;
		}
		case 0xbc: { /* new array */
			char atype = method->bytecode[bytecode_offset++];
			if (atype < 4 || atype > 11) {
				fprintf(stderr, "atype invalid creating new array\n");
				free(byteCodeMachineCodeOffsets);
				return 0;
			}

			// count is already on the stack
			// push atype
			push_byte(atype);

			// mov eax, new_array
			data_offset = add_byte(0xb8);
			add_dword(new_array);

			// call eax
			data_offset = add_byte(0xff);
			data_offset = add_byte(0xd0);

			// add esp, 8
			data_offset = add_byte(0x83);
			data_offset = add_byte(0xc4);
			data_offset = add_byte(8);

			// push eax
			data_offset = add_byte(0x50);

			break;
		}
		case 0xbd: { /* new array of references */
			unsigned index = ((method->bytecode[bytecode_offset] << 8) | method->bytecode[bytecode_offset + 1]) - 1;
			bytecode_offset += 2;

			int offset = get_constant_from_pool_by_index(method->jclass, index);
			uint8_t tag = method->jclass->loaded_data[offset];
			if (tag == 7) { /* class */
				JavaClass * jclass = get_or_load_class_from_class_constant(method->jclass, offset);
				if (!jclass) {
					free(byteCodeMachineCodeOffsets);
					return 0;
				}

				// JArrayInstance * a = new_array(0, size);
				push_dword(0);

				// mov eax, new_array
				data_offset = add_byte(0xb8);
				add_dword(new_array);

				// call eax
				data_offset = add_byte(0xff);
				data_offset = add_byte(0xd0);

				// add esp, 8
				data_offset = add_byte(0x83);
				data_offset = add_byte(0xc4);
				data_offset = add_byte(8);

				// push eax
				data_offset = add_byte(0x50);
			}
			else {
				printf("new array of refences uses tag %u\n", tag);
				free(byteCodeMachineCodeOffsets);
				return 0;
			}

			break;
		}
		case 0xbe: { /* arraylength */
			// pop eax
			data_offset = add_byte(0x58);

			// mov eax, dword ptr [eax+12]
			data_offset = add_byte(0x8b);
			data_offset = add_byte(0x40);
			data_offset = add_byte(12);

			// push eax
			data_offset = add_byte(0x50);
			break;
		}
		default: {
			fprintf(stderr, "Unrecognised bytecode: 0x%x in method '%s'\n", op, method->name);
			free(byteCodeMachineCodeOffsets);
			return 0;
		}
		}

		if (bytecode_offset >= method->bytecodeLength)
			break;
	}

#undef push
#undef call

	method->machineCodeLength = data_offset;

	AddressReplacement * replace = addressReplacement;
	while (replace) {
		int difference = byteCodeMachineCodeOffsets[replace->newOffset] - (replace->offsetOfReplacement + 4);

		data_offset = replace->offsetOfReplacement;

		data[data_offset] = difference & 0xff;
		data[data_offset+1] = (difference>>8) & 0xff;
		data[data_offset+2] = (difference>>16) & 0xff;
		data[data_offset+3] = (difference>>24) & 0xff;

		AddressReplacement * next = replace->next;
		free(replace);
		replace = next;
	}

	data_offset = 0;

	if (method->name) {
		len = strlen(method->name);
		len2 = strlen(method->jclass->class_name);
		s = malloc(len2 + 1 + len + 13);
		strcpy(s, method->jclass->class_name);
		s[len2] = '.';
		strcpy(&s[len2 + 1], method->name);
		memcpy(&s[len2 + 1 + len], ".machinecode", 13);

		char * s1 = s;
		while (*s1) {
			if (*s1 == '<' || *s1 == '>')
				*s1 = '_';
			if (*s1 == '/' || *s1 == '\\')
				*s1 = '.';
			s1++;
		}

		file = fopen(s, "w");
		free(s);
		if (file) {
			fwrite(data, method->machineCodeLength, 1, file);
			fclose(file);
		}
	}
	
	free(byteCodeMachineCodeOffsets);

	return data;
}


#ifdef __linux__

#include <sys/mman.h>

void * alloc_executable_memory(size_t size)
{
	void * ptr = mmap(0, size,
		PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ptr == (void*)-1) {
		fputs("Error allocating executable memory", stderr);
		return 0;
	}
	return ptr;
}

void * resize_executable_memory(void * a, size_t size)
{
	void * ptr = mmap(a, size,
		PROT_READ | PROT_WRITE | PROT_EXEC,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ptr == (void*)-1) {
		fputs("Error allocating executable memory", stderr);
		return 0;
	}
	return ptr;
}

#else

#include <Windows.h>

void * alloc_executable_memory(size_t size)
{
	return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
}

void * resize_executable_memory(void * a, size_t size)
{
	return VirtualAlloc(a, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
}

#endif
