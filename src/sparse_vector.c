#include "sparse_vector.h"
#include <stdlib.h>
#include <stdio.h>
#include "util.h"

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
    if (element_ptr == NULL || count_ptr == NULL) {
        print_ICE("NULL pointer passed to Vector get().\n");
        exit(EXIT_FAILURE);
    }
    if (index >= this->size) {
        print_ICE("Invalid index passed to Vector get().\n");
        exit(EXIT_FAILURE);
    }
    *((const void **)element_ptr) = this->items[index];
    *count_ptr = this->counts[index];
    return 0;
}

int
SparseVector_at(const SparseVector *this, size_t index, void *element_ptr) {
    if (element_ptr == NULL) {
        print_ICE("NULL pointer passed to SparseVector get().\n");
        exit(EXIT_FAILURE);
    }
    size_t curr_index = 0;
    while (this->counts[curr_index] < index) {
        index -= this->counts[curr_index];
        curr_index++;
        if (curr_index >= this->size) {
            print_ICE("Invalid index passed to Vector get().\n");
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

unsigned int
SparseVector_count(const SparseVector *this) {
    return this->count;
}

size_t
SparseVector_capacity(const SparseVector *this) {
    return this->capacity;
}

SparseVector *
new_SparseVector(size_t size) {
    SparseVector *this;

    if (NULL == (this = malloc(sizeof(*this)))) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if (NULL == (this->items = malloc(size * sizeof(void *)))) {
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
    if (NULL == (this->items = malloc(sizeof(*this->items)))) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if (NULL == (this->counts = malloc(sizeof(*this->counts)))) {
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
