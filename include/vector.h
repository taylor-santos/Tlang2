#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>
#include <stdio.h>

#define Vector() new_Vector(0)

typedef struct Vector Vector;
typedef void (*VEC_DELETE_FUNC)(void *);
typedef void *(*VEC_COPY_FUNC)(const void *);
typedef int (*VEC_COMPARATOR)(const void *, const void *);

/*
 * Append element to the end of the vector, expanding its capacity if needed.
 * If a malloc failure occurs and ERROR_MODE is VEC_EXIT_ON_FAIL, an error
 * message is printed to stderr and the program exits. Otherwise if an error
 * occurs and ERROR_MODE is VEC_
 */
Vector *
Vector_append(Vector *this, void *element);

/*
 * If index is within the bounds of the vector, returns the element at that
 * index. Otherwise prints an error and exits the program.
 */
void *
Vector_get(const Vector *this, size_t index);

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
sort_Vector(Vector *vec, VEC_COMPARATOR comparator);

Vector *
copy_Vector(const Vector *vec, VEC_COPY_FUNC copy_value);

void
delete_Vector(Vector *this, VEC_DELETE_FUNC delete_value);

#endif
