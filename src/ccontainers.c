#include "ccontainers.h"
#include <stdio.h>

// ============================================================
// String implementation
// ============================================================

String string_create(void) {
    String s = {0};
    return s;
}

String string_from(const char* s) {
    return string_from_n(s, strlen(s));
}

String string_from_n(const char* s, size_t n) {
    String result = {0};
    result.data = (char*)malloc(n + 1);
    if (result.data) {
        memcpy(result.data, s, n);
        result.data[n] = '\0';
        result.len = n;
        result.cap = n + 1;
    }
    return result;
}

String string_clone(const String* s) {
    return string_from_n(s->data, s->len);
}

void string_destroy(String* s) {
    free(s->data);
    s->data = NULL;
    s->len = 0;
    s->cap = 0;
}

bool string_append(String* s, const char* tail) {
    return string_append_n(s, tail, strlen(tail));
}

bool string_append_n(String* s, const char* tail, size_t n) {
    if (n == 0) return true;
    if (s->len + n + 1 > s->cap) {
        size_t new_cap = s->cap ? s->cap : 32;
        while (new_cap < s->len + n + 1) {
            new_cap += new_cap / 2;
            if (new_cap < 32) new_cap = 32;
        }
        char* new_data = (char*)realloc(s->data, new_cap);
        if (!new_data) return false;
        s->data = new_data;
        s->cap = new_cap;
    }
    memcpy(s->data + s->len, tail, n);
    s->len += n;
    s->data[s->len] = '\0';
    return true;
}

bool string_append_char(String* s, char c) {
    return string_append_n(s, &c, 1);
}

bool string_printf(String* s, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    if (needed < 0) return false;

    size_t needed_len = (size_t)needed;
    if (s->len + needed_len + 1 > s->cap) {
        size_t new_cap = s->cap ? s->cap : 32;
        while (new_cap < s->len + needed_len + 1) {
            new_cap += new_cap / 2;
            if (new_cap < 32) new_cap = 32;
        }
        char* new_data = (char*)realloc(s->data, new_cap);
        if (!new_data) return false;
        s->data = new_data;
        s->cap = new_cap;
    }

    va_start(args, fmt);
    int written = vsnprintf(s->data + s->len, s->cap - s->len, fmt, args);
    va_end(args);
    if (written < 0) return false;
    s->len += (size_t)written;
    return true;
}

void string_clear(String* s) {
    if (s->data) {
        s->data[0] = '\0';
    }
    s->len = 0;
}

char* string_cstr(String* s) {
    if (!s->data) {
        s->data = (char*)malloc(1);
        if (s->data) {
            s->data[0] = '\0';
            s->cap = 1;
        }
    }
    return s->data;
}

int string_cmp(const String* a, const String* b) {
    return strcmp(a->data, b->data);
}

// ============================================================
// Array_base implementation
// ============================================================

void array_init_base(Array_base* arr, size_t elem_size) {
    arr->data = NULL;
    arr->len = 0;
    arr->cap = 0;
    arr->elem_size = elem_size;
}

void array_destroy_base(Array_base* arr) {
    free(arr->data);
    arr->data = NULL;
    arr->len = 0;
    arr->cap = 0;
}

bool array_grow_base(Array_base* arr, size_t min_cap) {
    if (min_cap <= arr->cap) return true;
    size_t new_cap = arr->cap ? arr->cap : 16;
    while (new_cap < min_cap) {
        new_cap += new_cap / 2;
        if (new_cap < 16) new_cap = 16;
    }
    void* new_data = realloc(arr->data, new_cap * arr->elem_size);
    if (!new_data) return false;
    arr->data = new_data;
    arr->cap = new_cap;
    return true;
}

void* array_push_base(Array_base* arr) {
    if (!array_grow_base(arr, arr->len + 1)) return NULL;
    return (char*)arr->data + (arr->len++) * arr->elem_size;
}

void array_pop_base(Array_base* arr) {
    if (arr->len > 0) arr->len--;
}

void* array_get_base(Array_base* arr, size_t i) {
    return (char*)arr->data + i * arr->elem_size;
}

// ============================================================
// Map_base implementation
// ============================================================

void map_init_base(Map_base* m, size_t key_size, size_t val_size, int (*cmp)(const void*, const void*)) {
    m->keys = NULL;
    m->values = NULL;
    m->len = 0;
    m->cap = 0;
    m->key_size = key_size;
    m->val_size = val_size;
    m->cmp = cmp;
}

void map_destroy_base(Map_base* m) {
    free(m->keys);
    free(m->values);
    m->keys = NULL;
    m->values = NULL;
    m->len = 0;
    m->cap = 0;
}

bool map_set_base(Map_base* m, const void* key, const void* val) {
    size_t lo = 0, hi = m->len;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        int c = m->cmp(key, (const char*)m->keys + mid * m->key_size);
        if (c == 0) {
            memcpy((char*)m->values + mid * m->val_size, val, m->val_size);
            return true;
        } else if (c < 0) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }

    if (m->len == m->cap) {
        size_t new_cap = m->cap ? m->cap * 2 : 16;
        void* new_keys = realloc(m->keys, new_cap * m->key_size);
        void* new_vals = realloc(m->values, new_cap * m->val_size);
        if (!new_keys || !new_vals) {
            free(new_keys);
            free(new_vals);
            return false;
        }
        m->keys = new_keys;
        m->values = new_vals;
        m->cap = new_cap;
    }

    if (lo < m->len) {
        memmove((char*)m->keys + (lo + 1) * m->key_size,
                (char*)m->keys + lo * m->key_size,
                (m->len - lo) * m->key_size);
        memmove((char*)m->values + (lo + 1) * m->val_size,
                (char*)m->values + lo * m->val_size,
                (m->len - lo) * m->val_size);
    }
    memcpy((char*)m->keys + lo * m->key_size, key, m->key_size);
    memcpy((char*)m->values + lo * m->val_size, val, m->val_size);
    m->len++;
    return true;
}

bool map_get_base(const Map_base* m, const void* key, void* val_out) {
    if (m->len == 0) return false;
    size_t lo = 0, hi = m->len;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        int c = m->cmp(key, (const char*)m->keys + mid * m->key_size);
        if (c == 0) {
            if (val_out) memcpy(val_out, (const char*)m->values + mid * m->val_size, m->val_size);
            return true;
        } else if (c < 0) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    return false;
}

bool map_has_base(const Map_base* m, const void* key) {
    return map_get_base(m, key, NULL);
}

void map_remove_base(Map_base* m, const void* key) {
    if (m->len == 0) return;
    size_t lo = 0, hi = m->len;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        int c = m->cmp(key, (const char*)m->keys + mid * m->key_size);
        if (c == 0) {
            if (mid + 1 < m->len) {
                memmove((char*)m->keys + mid * m->key_size,
                        (char*)m->keys + (mid + 1) * m->key_size,
                        (m->len - mid - 1) * m->key_size);
                memmove((char*)m->values + mid * m->val_size,
                        (char*)m->values + (mid + 1) * m->val_size,
                        (m->len - mid - 1) * m->val_size);
            }
            m->len--;
            return;
        } else if (c < 0) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
}

void map_clear_base(Map_base* m) {
    m->len = 0;
}

// ============================================================
// Hash functions
// ============================================================

uint64_t hash_fnv1a_64(const unsigned char* data, size_t len) {
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint64_t)data[i];
        h *= 1099511628211ULL;
    }
    return h;
}

uint64_t hash_int64(const void* key) {
    int64_t val;
    memcpy(&val, key, sizeof(val));
    return (uint64_t)(val * 0x9e3779b97f4a7c15ULL);
}

uint64_t hash_string(const void* key) {
    const char* s = *(const char**)key;
    return hash_fnv1a_64((const unsigned char*)s, strlen(s));
}

int cmp_int64(const void* a, const void* b) {
    int64_t va, vb;
    memcpy(&va, a, sizeof(va));
    memcpy(&vb, b, sizeof(vb));
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

int cmp_string(const void* a, const void* b) {
    const char* sa = *(const char**)a;
    const char* sb = *(const char**)b;
    return strcmp(sa, sb);
}

// ============================================================
// HashMap_base implementation
// ============================================================

void hashmap_init_base(HashMap_base* m, size_t key_size, size_t val_size,
                       uint64_t (*hash)(const void*), int (*cmp)(const void*, const void*)) {
    m->metadata = NULL;
    m->keys = NULL;
    m->values = NULL;
    m->cap = 0;
    m->len = 0;
    m->key_size = key_size;
    m->val_size = val_size;
    m->hash = hash;
    m->cmp = cmp;
}

void hashmap_destroy_base(HashMap_base* m) {
    free(m->metadata);
    free(m->keys);
    free(m->values);
    m->metadata = NULL;
    m->keys = NULL;
    m->values = NULL;
    m->cap = 0;
    m->len = 0;
}

static bool hashmap_grow(HashMap_base* m) {
    size_t old_cap = m->cap;
    size_t new_cap = old_cap ? old_cap * 2 : 16;

    uint8_t* new_meta = (uint8_t*)calloc(new_cap, 1);
    void* new_keys = calloc(new_cap, m->key_size);
    void* new_vals = calloc(new_cap, m->val_size);
    if (!new_meta || !new_keys || !new_vals) {
        free(new_meta);
        free(new_keys);
        free(new_vals);
        return false;
    }

    for (size_t i = 0; i < old_cap; i++) {
        if (m->metadata[i] == 1) {
            const void* key = (const char*)m->keys + i * m->key_size;
            const void* val = (const char*)m->values + i * m->val_size;

            uint64_t h = m->hash(key);
            size_t idx = h & (new_cap - 1);
            while (new_meta[idx]) {
                idx = (idx + 1) & (new_cap - 1);
            }
            new_meta[idx] = 1;
            memcpy((char*)new_keys + idx * m->key_size, key, m->key_size);
            memcpy((char*)new_vals + idx * m->val_size, val, m->val_size);
        }
    }

    free(m->metadata);
    free(m->keys);
    free(m->values);
    m->metadata = new_meta;
    m->keys = new_keys;
    m->values = new_vals;
    m->cap = new_cap;
    return true;
}

bool hashmap_set_base(HashMap_base* m, const void* key, const void* val) {
    if (m->cap == 0) {
        if (!hashmap_grow(m)) return false;
    } else if (m->len * 4 >= m->cap * 3) {
        if (!hashmap_grow(m)) return false;
    }

    uint64_t h = m->hash(key);
    size_t idx = h & (m->cap - 1);

    for (;;) {
        if (m->metadata[idx] == 0) {
            m->metadata[idx] = 1;
            memcpy((char*)m->keys + idx * m->key_size, key, m->key_size);
            memcpy((char*)m->values + idx * m->val_size, val, m->val_size);
            m->len++;
            return true;
        }
        if (m->metadata[idx] == 1 &&
            m->cmp(key, (const char*)m->keys + idx * m->key_size) == 0) {
            memcpy((char*)m->values + idx * m->val_size, val, m->val_size);
            return true;
        }
        idx = (idx + 1) & (m->cap - 1);
    }
}

bool hashmap_get_base(const HashMap_base* m, const void* key, void* val_out) {
    if (m->cap == 0) return false;

    uint64_t h = m->hash(key);
    size_t idx = h & (m->cap - 1);

    for (size_t i = 0; i < m->cap; i++) {
        if (m->metadata[idx] == 0) return false;
        if (m->metadata[idx] == 1 &&
            m->cmp(key, (const char*)m->keys + idx * m->key_size) == 0) {
            if (val_out) memcpy(val_out, (const char*)m->values + idx * m->val_size, m->val_size);
            return true;
        }
        idx = (idx + 1) & (m->cap - 1);
    }
    return false;
}

bool hashmap_has_base(const HashMap_base* m, const void* key) {
    return hashmap_get_base(m, key, NULL);
}

void hashmap_remove_base(HashMap_base* m, const void* key) {
    if (m->cap == 0) return;

    uint64_t h = m->hash(key);
    size_t idx = h & (m->cap - 1);

    for (size_t i = 0; i < m->cap; i++) {
        if (m->metadata[idx] == 0) return;
        if (m->metadata[idx] == 1 &&
            m->cmp(key, (const char*)m->keys + idx * m->key_size) == 0) {
            m->metadata[idx] = 2;
            m->len--;
            return;
        }
        idx = (idx + 1) & (m->cap - 1);
    }
}

// ============================================================
// List implementation
// ============================================================

List list_create(void) {
    List l = {0};
    return l;
}

void list_destroy(List* l) {
    ListNode* cur = l->first;
    while (cur) {
        ListNode* next = cur->next;
        if (cur->deleter && cur->data) cur->deleter(cur->data);
        free(cur);
        cur = next;
    }
    l->first = NULL;
    l->last = NULL;
    l->len = 0;
}

ListNode* list_push_front(List* l, void* data, void (*deleter)(void*)) {
    ListNode* node = (ListNode*)malloc(sizeof(ListNode));
    if (!node) return NULL;
    node->data = data;
    node->deleter = deleter;
    node->prev = NULL;
    node->next = l->first;
    if (l->first) {
        l->first->prev = node;
    } else {
        l->last = node;
    }
    l->first = node;
    l->len++;
    return node;
}

ListNode* list_push_back(List* l, void* data, void (*deleter)(void*)) {
    ListNode* node = (ListNode*)malloc(sizeof(ListNode));
    if (!node) return NULL;
    node->data = data;
    node->deleter = deleter;
    node->next = NULL;
    node->prev = l->last;
    if (l->last) {
        l->last->next = node;
    } else {
        l->first = node;
    }
    l->last = node;
    l->len++;
    return node;
}

void* list_pop_front(List* l) {
    if (!l->first) return NULL;
    ListNode* node = l->first;
    void* data = node->data;
    l->first = node->next;
    if (l->first) {
        l->first->prev = NULL;
    } else {
        l->last = NULL;
    }
    l->len--;
    free(node);
    return data;
}

void* list_pop_back(List* l) {
    if (!l->last) return NULL;
    ListNode* node = l->last;
    void* data = node->data;
    l->last = node->prev;
    if (l->last) {
        l->last->next = NULL;
    } else {
        l->first = NULL;
    }
    l->len--;
    free(node);
    return data;
}

void list_remove(List* l, ListNode* node) {
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        l->first = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    } else {
        l->last = node->prev;
    }
    if (node->deleter && node->data) node->deleter(node->data);
    free(node);
    l->len--;
}

void list_clear(List* l) {
    ListNode* cur = l->first;
    while (cur) {
        ListNode* next = cur->next;
        if (cur->deleter && cur->data) cur->deleter(cur->data);
        free(cur);
        cur = next;
    }
    l->first = NULL;
    l->last = NULL;
    l->len = 0;
}

size_t list_len(const List* l) {
    return l->len;
}

ListNode* list_at(const List* l, size_t index) {
    if (index >= l->len) return NULL;
    ListNode* cur = l->first;
    for (size_t i = 0; i < index; i++) cur = cur->next;
    return cur;
}

// ============================================================
// Queue implementation
// ============================================================

void queue_init(Queue* q, size_t elem_size) {
    q->data = NULL;
    q->elem_size = elem_size;
    q->head = 0;
    q->tail = 0;
    q->cap = 0;
}

void queue_destroy(Queue* q) {
    free(q->data);
    q->data = NULL;
    q->head = 0;
    q->tail = 0;
    q->cap = 0;
}

bool queue_push(Queue* q, const void* item) {
    if (q->cap == 0) {
        q->cap = 16;
        q->data = malloc(q->cap * q->elem_size);
        if (!q->data) return false;
    } else if (q->tail - q->head == q->cap) {
        size_t new_cap = q->cap * 2;
        void* new_data = malloc(new_cap * q->elem_size);
        if (!new_data) return false;

        size_t count = q->tail - q->head;
        size_t head_mod = q->head & (q->cap - 1);
        size_t first_chunk = q->cap - head_mod;
        if (first_chunk > count) first_chunk = count;

        memcpy(new_data, (char*)q->data + head_mod * q->elem_size, first_chunk * q->elem_size);
        if (first_chunk < count) {
            memcpy((char*)new_data + first_chunk * q->elem_size, q->data,
                   (count - first_chunk) * q->elem_size);
        }

        free(q->data);
        q->data = new_data;
        q->head = 0;
        q->tail = count;
        q->cap = new_cap;
    }

    size_t idx = q->tail & (q->cap - 1);
    memcpy((char*)q->data + idx * q->elem_size, item, q->elem_size);
    q->tail++;
    return true;
}

bool queue_pop(Queue* q, void* item_out) {
    if (q->head == q->tail) return false;
    size_t idx = q->head & (q->cap - 1);
    if (item_out) memcpy(item_out, (char*)q->data + idx * q->elem_size, q->elem_size);
    q->head++;
    return true;
}

void* queue_front(Queue* q) {
    if (q->head == q->tail) return NULL;
    size_t idx = q->head & (q->cap - 1);
    return (char*)q->data + idx * q->elem_size;
}

void* queue_back(Queue* q) {
    if (q->head == q->tail) return NULL;
    size_t idx = (q->tail - 1) & (q->cap - 1);
    return (char*)q->data + idx * q->elem_size;
}

size_t queue_len(const Queue* q) {
    return q->tail - q->head;
}

bool queue_empty(const Queue* q) {
    return q->head == q->tail;
}
