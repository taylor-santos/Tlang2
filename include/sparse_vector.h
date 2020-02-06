#ifndef SPARSE_VECTOR_H
#define SPARSE_VECTOR_H

#include <stddef.h>

#define SparseVector() new_SparseVector(0)
#define ull unsigned long long int

typedef struct SparseVector SparseVector;

SparseVector *
SparseVector_append(SparseVector *this, void *element, ull count);

int
SparseVector_get(const SparseVector *this,
    size_t index,
    void *element_ptr,
    ull *count_ptr);

int
SparseVector_at(const SparseVector *this, size_t index, void *element_ptr);

size_t
SparseVector_size(const SparseVector *this);

unsigned int
SparseVector_count(const SparseVector *this);

size_t
SparseVector_capacity(const SparseVector *this);

SparseVector *
new_SparseVector(size_t size);

SparseVector *
init_SparseVector(void *element, ull count);

void
delete_SparseVector(SparseVector *this, void (*delete_value)(void *));

#endif
