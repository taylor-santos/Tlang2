#ifndef ITERATOR_H
#define ITERATOR_H

typedef struct Iterator Iterator;

struct Iterator {
    struct IteratorData *data;
    int (*hasNext)(Iterator *this);
    void *(*next)(Iterator *this);
    void (*delete)(Iterator *this);
};

#endif
