#include "vector.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "util.h"

#define REALLOC_SIZE 8

static enum VEC_ERROR_MODE ERROR_MODE = VEC_EXIT_ON_FAIL;

struct Vector {
    void **items;
    size_t capacity;
    size_t size;
};

int
Vector_append(Vector *this, void *element) {
    if (this->size == this->capacity) {
        size_t new_capacity = this->capacity + REALLOC_SIZE;
        if (NULL == (this->items =
            realloc(this->items, new_capacity * sizeof(void *)))) {
            if (VEC_EXIT_ON_FAIL == ERROR_MODE) {
                perror("realloc");
                exit(EXIT_FAILURE);
            } else {
                return 1;
            }
        }
        this->capacity = new_capacity;
    }
    this->items[this->size++] = element;
    return 0;
}

int
Vector_get(const Vector *this, size_t index, void *element_ptr) {
    if (element_ptr == NULL) {
        if (VEC_EXIT_ON_FAIL == ERROR_MODE) {
            print_ICE("NULL pointer passed to Vector get().\n");
            exit(EXIT_FAILURE);
        } else {
            return 1;
        }
    }
    if (index >= this->size) {
        if (VEC_EXIT_ON_FAIL == ERROR_MODE) {
            print_ICE("Invalid index passed to Vector get().\n");
            exit(EXIT_FAILURE);
        } else {
            return 1;
        }
    }
    *((const void **)element_ptr) = this->items[index];
    return 0;
}

size_t
Vector_size(const Vector *this) {
    return this->size;
}

size_t
Vector_capacity(const Vector *this) {
    return this->capacity;
}

Vector *
new_Vector(size_t size) {
    Vector *this;

    if (NULL == (this = malloc(sizeof(*this)))) {
        if (VEC_EXIT_ON_FAIL == ERROR_MODE) {
            perror("malloc");
            exit(EXIT_FAILURE);
        } else {
            return NULL;
        }
    }
    if (NULL == (this->items = malloc(size * sizeof(void *)))) {
        if (VEC_EXIT_ON_FAIL == ERROR_MODE) {
            perror("malloc");
            exit(EXIT_FAILURE);
        } else {
            free(this);
            return NULL;
        }
    }
    this->capacity = size;
    this->size = 0;
    return this;
}

Vector *
init_Vector(void *element) {
    Vector *this;

    if (NULL == (this = malloc(sizeof(*this)))) {
        if (VEC_EXIT_ON_FAIL == ERROR_MODE) {
            perror("malloc");
            exit(EXIT_FAILURE);
        } else {
            return NULL;
        }
    }
    if (NULL == (this->items = malloc(sizeof(*this->items)))) {
        if (VEC_EXIT_ON_FAIL == ERROR_MODE) {
            perror("malloc");
            exit(EXIT_FAILURE);
        } else {
            free(this);
            return NULL;
        }
    }
    this->items[0] = element;
    this->capacity = 1;
    this->size = 1;
    return this;
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

void
Vector_set_error_mode(enum VEC_ERROR_MODE mode) {
    assert(VEC_EXIT_ON_FAIL == mode || VEC_RETURN_ERROR_CODE == mode);
    ERROR_MODE = mode;
}
