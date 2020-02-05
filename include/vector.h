#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>

#define Vector() new_Vector(0)

enum VEC_ERROR_MODE {
    VEC_EXIT_ON_FAIL, VEC_RETURN_ERROR_CODE
};

typedef struct Vector Vector;
typedef void (*VEC_DELETE_TYPE)(void *);

/*
 * Append element to the end of the vector, expanding its capacity if needed.
 * If a malloc failure occurs and ERROR_MODE is VEC_EXIT_ON_FAIL, an error
 * message is printed to stderr and the program exits. Otherwise if an error
 * occurs and ERROR_MODE is VEC_
 */
Vector *
Vector_append(Vector *this, void *element);

/*
 * If index is within the bounds of the vector, points element_ptr to the
 * element at that index. If VEC_EXIT_ON_FAIL is defined, an index error or
 * NULL element_ptr will print an error message
 */
int
Vector_get(const Vector *this, size_t index, void *element_ptr);

/*
 * Returns the number of elements in the vector.
 */
size_t
Vector_size(const Vector *this);

/*
 * Returns the capacity of the vector. This is the maximum number of
 * elements it can contain before adding an additional element will cause a
 * reallocation.
 */
size_t
Vector_capacity(const Vector *this);

Vector *
new_Vector(size_t size);

Vector *
init_Vector(void *element);

void
delete_Vector(Vector *this, void (*delete_value)(void *));

void
Vector_set_error_mode(enum VEC_ERROR_MODE mode);

#endif
