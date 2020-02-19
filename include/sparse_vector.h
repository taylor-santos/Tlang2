#ifndef SPARSE_VECTOR_H
#define SPARSE_VECTOR_H

#include <stddef.h>

#define SparseVector() new_SparseVector(0)
#define ull unsigned long long int

typedef struct SparseVector SparseVector;
typedef void (*SVEC_DELETE_FUNC)(void *);
typedef void *(*SVEC_COPY_FUNC)(const void *);
typedef int (*SVEC_COMPARE_FUNC)(const void *, const void *, const void *data);

SparseVector *
SparseVector_append(SparseVector *this, void *element, ull count);

int
SparseVector_get(const SparseVector *this,
    size_t index,
    void *element_ptr,
    ull *count_ptr);

int
SparseVector_at(const SparseVector *this, ull index, void *element_ptr);

size_t
SparseVector_size(const SparseVector *this);

unsigned long long int
SparseVector_count(const SparseVector *this);

size_t
SparseVector_capacity(const SparseVector *this);

/*
 * Given a comparison function that returns 0 if two elements are equal,
 * reduces the sparse vector such that adjacent cells with equal elements
 * are added together. A pointer to arbitrary data is passed in as the
 * third element of the comparison function. Elements that get reduced may
 * optionally be passed to a delete function to free their memory.
 */
void
SparseVector_reduce(SparseVector *this,
    SVEC_COMPARE_FUNC comp,
    const void *data,
    SVEC_DELETE_FUNC delete);

SparseVector *
new_SparseVector(size_t size);

SparseVector *
init_SparseVector(void *element, ull count);

SparseVector *
copy_SparseVector(SparseVector *vec, SVEC_COPY_FUNC copy_value);

void
delete_SparseVector(SparseVector *this, SVEC_DELETE_FUNC delete_value);

#endif
