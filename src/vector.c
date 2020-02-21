#include "vector.h"
#include <stdlib.h>
#include <stdio.h>
#include "util.h"
#include "safe.h"

#define REALLOC_SIZE 8

struct Vector {
    void **items;
    size_t capacity;
    size_t size;
};

Vector *
Vector_append(Vector *this, void *element) {
    if (this->size == this->capacity) {
        size_t new_capacity = this->capacity + REALLOC_SIZE;
        this->items = safe_realloc(this->items, new_capacity * sizeof(void *));
        this->capacity = new_capacity;
    }
    this->items[this->size++] = element;
    return this;
}

void *
Vector_get(const Vector *this, size_t index) {
    if (index >= this->size) {
        print_ICE("Invalid index passed to Vector get().\n");
        exit(EXIT_FAILURE);
    }
    return this->items[index];
}

size_t
Vector_size(const Vector *this) {
    return this->size;
}

size_t
Vector_capacity(const Vector *this) {
    return this->capacity;
}

void
Vector_clear(Vector *this, VEC_DELETE_FUNC delete_value) {
    if (NULL != delete_value) {
        for (size_t i = 0; i < this->size; i++) {
            delete_value(this->items[i]);
        }
    }
    this->size = 0;
}

Vector *
new_Vector(size_t size) {
    Vector *this;

    this = safe_malloc(sizeof(*this));
    *this = (Vector){
        safe_malloc(size * sizeof(void *)), size, 0
    };
    return this;
}

Vector *
init_Vector(void *element) {
    Vector *this;

    this = safe_malloc(sizeof(*this));
    this->items = malloc(sizeof(*this->items));
    this->items[0] = element;
    this->capacity = 1;
    this->size = 1;
    return this;
}

void
sort_Vector(Vector *vec, VEC_COMPARATOR comparator) {
    qsort(vec->items, vec->size, sizeof(*vec->items), comparator);
}

Vector *
copy_Vector(const Vector *vec, VEC_COPY_FUNC copy_value) {
    Vector *new_vec;
    void **new_items;

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
    for (size_t i = 0; i < vec->size; i++) {
        new_items[i] = copy_value(vec->items[i]);
    }
    *new_vec = (Vector){
        new_items, vec->capacity, vec->size
    };
    return new_vec;
}

void
delete_Vector(Vector *this, void (*delete_value)(void *)) {
    if (NULL != delete_value) {
        for (size_t i = 0; i < this->size; i++) {
            delete_value(this->items[i]);
        }
    }
    free(this->items);
    free(this);
}
