#include "sparse_vector.h"
#include <stdlib.h>
#include <stdio.h>
#include "util.h"
#include "vector.h"

#define REALLOC_SIZE 8

struct SparseVector {
    void **items;
    ull *counts;
    size_t capacity;
    size_t size;
    ull count;
};

SparseVector *
SparseVector_append(SparseVector *this, void *element, ull count) {
    if (this->size == this->capacity) {
        size_t new_capacity = this->capacity + REALLOC_SIZE;
        if (NULL == (this->items =
            realloc(this->items, new_capacity * sizeof(*this->items)))) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        if (NULL == (this->counts =
            realloc(this->counts, new_capacity * sizeof(*this->counts)))) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        this->capacity = new_capacity;
    }
    this->counts[this->size] = count;
    this->items[this->size++] = element;
    this->count += count;
    return this;
}

int
SparseVector_get(const SparseVector *this,
    size_t index,
    void *element_ptr,
    ull *count_ptr) {
    if (index >= this->size) {
        print_ICE("Invalid index passed to SparseVector get().\n");
        exit(EXIT_FAILURE);
    }
    if (NULL != element_ptr) {
        *((const void **)element_ptr) = this->items[index];
    }
    if (NULL != count_ptr) {
        *count_ptr = this->counts[index];
    }
    return 0;
}

int
SparseVector_at(const SparseVector *this, ull index, void *element_ptr) {
    if (element_ptr == NULL) {
        print_ICE("NULL pointer passed to SparseVector get().\n");
        exit(EXIT_FAILURE);
    }
    size_t curr_index = 0;
    while (this->counts[curr_index] < index) {
        index -= this->counts[curr_index];
        curr_index++;
        if (curr_index >= this->size) {
            print_ICE("Invalid index passed to SparseVector get().\n");
            exit(EXIT_FAILURE);
        }
    }
    *((const void **)element_ptr) = this->items[curr_index];
    return 0;
}

size_t
SparseVector_size(const SparseVector *this) {
    return this->size;
}

unsigned long long int
SparseVector_count(const SparseVector *this) {
    return this->count;
}

size_t
SparseVector_capacity(const SparseVector *this) {
    return this->capacity;
}

void
SparseVector_reduce(SparseVector *this,
    SVEC_COMPARE_FUNC comp,
    const void *data,
    SVEC_DELETE_FUNC delete) {
    size_t currIndex = 0;
    for (size_t i = 1; i < this->size; i++) {
        if (comp(this->items[currIndex], this->items[i], data)) {
            // i and currIndex are different
            currIndex++;
            this->items[currIndex] = this->items[i];
            this->counts[currIndex] = this->counts[i];
        } else {
            // i and currIndex are the same
            this->counts[currIndex] += this->counts[i];
            if (NULL != delete) {
                delete(this->items[i]);
            }
        }
    }
    this->size = currIndex + 1;
}

SparseVector *
new_SparseVector(size_t size) {
    SparseVector *this;

    if (NULL == (this = malloc(sizeof(*this)))) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if (NULL == (this->items = malloc(size * sizeof(*this->items)))) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if (NULL == (this->counts = malloc(size * sizeof(*this->counts)))) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    this->capacity = size;
    this->size = 0;
    this->count = 0;
    return this;
}

SparseVector *
init_SparseVector(void *element, ull count) {
    SparseVector *this;

    if (NULL == (this = malloc(sizeof(*this)))) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if (NULL == (this->items = malloc(1 * sizeof(*this->items)))) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if (NULL == (this->counts = malloc(1 * sizeof(*this->counts)))) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    this->items[0] = element;
    this->counts[0] = count;
    this->capacity = 1;
    this->size = 1;
    this->count = count;
    return this;
}

SparseVector *
copy_SparseVector(SparseVector *vec, SVEC_COPY_FUNC copy_value) {
    SparseVector *new_vec;
    void **new_items;
    ull *new_counts;

    if (NULL == (new_vec = malloc(sizeof(*new_vec)))) {
        print_ICE("");
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if (NULL == (new_items = malloc(vec->capacity * sizeof(*new_items)))) {
        print_ICE("");
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if (NULL == (new_counts = malloc(vec->capacity * sizeof(*new_counts)))) {
        print_ICE("");
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < vec->size; i++) {
        new_items[i] = copy_value(vec->items[i]);
        new_counts[i] = vec->counts[i];
    }
    *new_vec = (SparseVector){
        new_items, new_counts, vec->capacity, vec->size, vec->count
    };
    return new_vec;
}

void
delete_SparseVector(SparseVector *this, void (*delete_value)(void *)) {
    if (NULL != delete_value) {
        for (size_t i = 0; i < this->size; i++) {
            delete_value(this->items[i]);
        }
    }
    free(this->items);
    free(this->counts);
    free(this);
}
