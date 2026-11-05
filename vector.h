#ifndef _VECTOR_H_
#define _VECTOR_H_

#include <stdlib.h>

struct _vector_header
{
	unsigned int size;
	unsigned int capacity;
	unsigned int count;
};

#define vector_init(vector, _count) do{\
	struct _vector_header *header = malloc(sizeof(struct _vector_header) + _count * sizeof(*vector));\
	vector = (char*)header + sizeof(struct _vector_header);\
	header->capacity = (!(_count) ? 1 : _count) * sizeof(*vector);\
	header->size = 0;\
	header->count = 0;\
} while(0)

#define VECTOR_CAPACITY_GROWTH_RATE 2

#define vector_put(pvector, value) do\
{\
	if(!pvector) break;\
	struct _vector_header * header = (char*)pvector - sizeof(struct _vector_header);\
	if (header->capacity < header->size + sizeof(value))\
	{\
		header->capacity *= VECTOR_CAPACITY_GROWTH_RATE;\
		header = realloc(header, sizeof(struct _vector_header) + header->capacity);\
		if(!header) break;\
		pvector = (char*)header + sizeof(struct _vector_header);\
	}\
	pvector[header->size / sizeof(value)] = value;\
	header->size += sizeof(value);\
	header->count += 1;\
} while(0)

#define vector_reset(vector) do\
{\
	if(!vector) break;\
	struct _vector_header * header = (char*)vector - sizeof(struct _vector_header);\
	header->size = 0;\
	header->count = 0;\
} while(0)

#define vector_get_size(vector) ((vector) ? ((struct _vector_header*)((char*)(vector) - sizeof(struct _vector_header)))->size : 0)

#define vector_get_count(vector) ((vector) ? ((struct _vector_header*)((char*)(vector) - sizeof(struct _vector_header)))->count : 0)

#define vector_free(vector) do { \
	if(!vector) break; \
	free((char*)vector - sizeof(struct _vector_header)); \
	vector = 0;\
} while (0)

#define vec_type(type) vec_##type*

#endif // !_VECTOR_H_