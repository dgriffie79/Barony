#ifndef SET_H
#define SET_H

#ifdef __cplusplus
extern "C" {
#endif

void* set_create(void);
void set_destroy(void* set);
int set_find(void* set, int key);
void set_insert(void* set, int key);
void set_clear(void* set);

#ifdef __cplusplus
}
#endif

#endif
