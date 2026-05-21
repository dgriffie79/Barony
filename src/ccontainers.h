#pragma once
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

// ============================================================
// String — dynamic char buffer
// ============================================================
typedef struct {
    char* data;
    size_t len;
    size_t cap;
} String;

String string_create(void);
String string_from(const char* s);
String string_from_n(const char* s, size_t n);
String string_clone(const String* s);
void   string_destroy(String* s);
bool   string_append(String* s, const char* tail);
bool   string_append_n(String* s, const char* tail, size_t n);
bool   string_append_char(String* s, char c);
bool   string_printf(String* s, const char* fmt, ...);
void   string_clear(String* s);
char*  string_cstr(String* s);
int    string_cmp(const String* a, const String* b);

// ============================================================
// Array(T) — typed dynamic array via macros
// ============================================================
#define ARRAY_OF(T)             \
    struct {                    \
        T* data;                \
        size_t len;             \
        size_t cap;             \
        size_t elem_size;       \
    }

typedef struct {
    void* data;
    size_t len;
    size_t cap;
    size_t elem_size;
} Array_base;

void  array_init_base(Array_base* arr, size_t elem_size);
void  array_destroy_base(Array_base* arr);
bool  array_grow_base(Array_base* arr, size_t min_cap);
void* array_push_base(Array_base* arr);
void  array_pop_base(Array_base* arr);
void* array_get_base(Array_base* arr, size_t i);

#define array_init(arr)           do { (arr)->data = NULL; (arr)->len = 0; (arr)->cap = 0; (arr)->elem_size = sizeof(*(arr)->data); } while(0)
#define array_destroy(arr)        do { free((arr)->data); (arr)->data = NULL; (arr)->len = 0; (arr)->cap = 0; } while(0)
#define array_len(arr)            ((arr)->len)
#define array_cap(arr)            ((arr)->cap)
#define array_push(arr)           ((arr)->data = (__typeof__((arr)->data))array_push_base((Array_base*)(arr)))
#define array_pop(arr)            (--(arr)->len)
#define array_get(arr, i)         ((arr)->data[(i)])
