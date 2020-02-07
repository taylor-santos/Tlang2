#include "map.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

struct Entry {
    struct Entry *next;
    void *key;
    size_t len;
    void *value;
    unsigned long hash;
};
#define Entry(key, len, value, h) new_Entry(key, len, value, h)

struct Map {
    unsigned int size;
    unsigned int capacity;
    double load_factor;
    struct Entry **entries;
};

static struct Entry *
new_Entry(const void *key, size_t len, void *value, unsigned int h) {
    struct Entry *new_entry;
    void *new_key;

    if (NULL == (new_entry = malloc(sizeof(*new_entry)))) {
        print_ICE("");
        perror("malloc");
        return NULL;
    }
    if (NULL == (new_key = malloc(len))) {
        free(new_entry);
        print_ICE("");
        perror("malloc");
        return NULL;
    }
    memcpy(new_key, key, len);
    *new_entry = (struct Entry){
        NULL, new_key, len, value, h
    };
    return new_entry;
}

static int
keycmp(const void *key1, size_t len1, const void *key2, size_t len2) {
    const char *k1 = key1, *k2 = key2;
    for (; len1 && len2; k1++, len1--, k2++, len2--) {
        if (*k1 != *k2) {
            return *k1 - *k2;
        }
    }
    if (len1 != len2) {
        return (int)len1 - (int)len2;
    }
    return 0;
}

static unsigned long
hash(const void *key, size_t len) {
    /* djb2 hash function
     * http://www.cse.yorku.ca/~oz/hash.html
     */
    unsigned long hash = 5381;
    size_t i;
    const char *k = key;
    for (i = 0; i < len; i++) {
        hash = ((hash << 5u) + hash) + *k++;
    }
    return hash;
}

static int
resize(Map *map, unsigned int new_capacity) {
    struct Entry **new_entries, *curr_entry, **new_entry;

    if (NULL == (new_entries = calloc(new_capacity, sizeof(*new_entries)))) {
        print_ICE("");
        perror("malloc");
        return 1;
    }
    for (unsigned int i = 0; i < map->capacity; i++) {
        for (curr_entry = map->entries[i];
            NULL != curr_entry;
            curr_entry = curr_entry->next) {
            unsigned long h = curr_entry->hash % new_capacity;
            for (new_entry = &new_entries[h];
                NULL != *new_entry;
                new_entries = &(*new_entry)->next) {
                if (keycmp(curr_entry->key,
                    curr_entry->len,
                    (*new_entry)->key,
                    (*new_entry)->len) <= 0) {
                    break;
                }
            }
            struct Entry *tmp = curr_entry->next;
            curr_entry->next = *new_entry;
            *new_entries = curr_entry;
            curr_entry = tmp;
        }
    }
    free(map->entries);
    map->entries = new_entries;
    map->capacity = new_capacity;
    return 0;
}

int
Map_put(Map *map, const void *key, size_t key_len, void *value, void **prev) {
    unsigned long h;
    struct Entry **curr_entry, *new_entry;

    h = hash(key, key_len);
    for (curr_entry = &map->entries[h % map->capacity];
        NULL != *curr_entry;
        curr_entry = &(*curr_entry)->next) {
        int cmp = keycmp(key, key_len, (*curr_entry)->key, (*curr_entry)->len);
        if (0 == cmp) {
            if (NULL != prev) {
                *prev = (*curr_entry)->value;
            }
            (*curr_entry)->value = value;
            return 0;
        } else if (0 > cmp) {
            if (NULL == (new_entry = Entry(key, key_len, value, h))) {
                return 1;
            }
            new_entry->next = *curr_entry;
            *curr_entry = new_entry;
            map->size++;
            if ((double)map->size / map->capacity >= map->load_factor) {
                return resize(map, 2 * map->capacity);
            }
            return 0;
        }
    }
    if (NULL == (new_entry = Entry(key, key_len, value, h))) {
        return 1;
    }
    *curr_entry = new_entry;
    map->size++;
    if ((double)map->size / map->capacity >= map->load_factor) {
        return resize(map, 2 * map->capacity);
    }
    return 0;
}

int
Map_get(Map *map, const void *key, size_t key_len, void **value) {
    struct Entry *curr_entry;
    unsigned long h;

    h = hash(key, key_len);
    for (curr_entry = map->entries[h % map->capacity];
        NULL != curr_entry;
        curr_entry = curr_entry->next) {
        int cmp = keycmp(key, key_len, curr_entry->key, curr_entry->len);
        if (0 == cmp) {
            if (NULL != value) {
                *value = curr_entry->value;
            }
            return 0;
        } else if (0 > cmp) {
            return 1;
        }
    }
    return 1;
}

void
delete_Map(Map *this, MAP_DELETE_FUNC delete_value) {
    for (size_t i = 0; i < this->capacity; i++) {
        struct Entry *curr_entry = this->entries[i];
        while (curr_entry != NULL) {
            struct Entry *next = curr_entry->next;
            if (NULL != delete_value) {
                delete_value(curr_entry->value);
            }
            free(curr_entry->key);
            free(curr_entry);
            curr_entry = next;
        }
    }
    free(this->entries);
    free(this);
}

Map *
new_Map(unsigned int capacity, double load_factor) {
    Map *map;
    struct Entry **entries;

    if (capacity == 0) {
        print_ICE("cannot create Map with 0 capacity\n");
        exit(EXIT_FAILURE);
    }
    if (NULL == (map = malloc(sizeof(*map)))) {
        print_ICE("");
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    if (NULL == (entries = calloc(capacity, sizeof(*entries)))) {
        print_ICE("");
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    *map = (Map){
        0, capacity, load_factor, entries
    };
    return map;
}
