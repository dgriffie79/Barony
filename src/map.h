#ifndef MAP_H
#define MAP_H

#ifdef __cplusplus
extern "C" {
#endif

// IntMap: map<int, Uint32> — for tileAttributes
void* intmap_create(void);
void intmap_destroy(void* map);
int intmap_find(void* map, int key);
unsigned int intmap_get(void* map, int key);
void intmap_set(void* map, int key, unsigned int value);
void intmap_erase(void* map, int key);
void intmap_clear(void* map);

// PtrMap: map<Sint32, void*> — for entities_map
void* ptrmap_create(void);
void ptrmap_destroy(void* map);
int ptrmap_find(void* map, int key);
void* ptrmap_get(void* map, int key);
void ptrmap_set(void* map, int key, void* value);
void ptrmap_erase(void* map, int key);
void ptrmap_clear(void* map);

#ifdef __cplusplus
}
#endif

#endif
