#ifndef MAP_H
#define MAP_H
#include <stddef.h>

#define Map() new_Map(16, 0.75)

typedef struct Map Map;
typedef void (*MAP_DELETE_FUNC)(void *);

/*
 * Insert a value into the map at a given key. If data already exists with that
 * key, it is returned through the prev argument if prev is not NULL.
 * Otherwise, prev is pointed to NULL. If an internal error occurs, an error
 * message is printed to stderr and 1 is returned. Otherwise, 0 is returned.
 * Note: if prev is a NULL pointer and there is previous data at the given
 * key, a memory leak is possible if no other pointers to the data exist.
 */
int
Map_put(Map *map, const void *key, size_t key_len, void *value, void **prev);

/*
 * Retrieve a value from the map using a given key. If the key exists in the
 * map 0 is returned and if the value argument is not NULL, the object it
 * points to will be pointed at the key's value. If the key doesn't exist in
 * the map, 1 is returned.
 */
int
Map_get(Map *map, const void *key, size_t key_len, void **value);
#define Map_contains(map, key, len) Map_get(map, key, len, NULL)

void
delete_Map(Map *map, MAP_DELETE_FUNC delete_value);

Map *
new_Map(unsigned int capacity, double load_factor);

#endif