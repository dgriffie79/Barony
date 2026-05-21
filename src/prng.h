#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Forward declaration
#ifdef __cplusplus
class BaronyRNG;
extern "C" {
#else
typedef struct BaronyRNG BaronyRNG;
#endif

void rng_seedTime(BaronyRNG* rng);
void rng_seedBytes(BaronyRNG* rng, const void* key, size_t size);
void rng_getBytes(BaronyRNG* rng, void* data_, size_t size);
int rng_getSeed(const BaronyRNG* rng, void* out, size_t size);
void rng_testSeedHealth(const BaronyRNG* rng);
void rng_setMarker(const BaronyRNG* rng);
void rng_checkMarker(const BaronyRNG* rng);
size_t rng_bytesRead(const BaronyRNG* rng);
bool rng_isSeeded(const BaronyRNG* rng);
uint8_t rng_getU8(BaronyRNG* rng);
uint16_t rng_getU16(BaronyRNG* rng);
uint32_t rng_getU32(BaronyRNG* rng);
uint64_t rng_getU64(BaronyRNG* rng);
int8_t rng_getI8(BaronyRNG* rng);
int16_t rng_getI16(BaronyRNG* rng);
int32_t rng_getI32(BaronyRNG* rng);
int64_t rng_getI64(BaronyRNG* rng);
float rng_getF32(BaronyRNG* rng);
double rng_getF64(BaronyRNG* rng);
int rng_rand(BaronyRNG* rng);
int rng_uniform(BaronyRNG* rng, int a, int b);
int rng_discrete(BaronyRNG* rng, const unsigned int* chances, int size);
int rng_normal(BaronyRNG* rng, int mean, int deviation);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class BaronyRNG {
public:
    BaronyRNG() { memset(this, 0, sizeof(*this)); }

    void seedTime() { rng_seedTime(this); }
    void seedBytes(const void* k, size_t s) { rng_seedBytes(this, k, s); }
    void getBytes(void* d, size_t s) { rng_getBytes(this, d, s); }
    int getSeed(void* o, size_t s) const { return rng_getSeed(this, o, s); }
    void testSeedHealth() const { rng_testSeedHealth(this); }
    void setMarker() const { rng_setMarker(this); }
    void checkMarker() const { rng_checkMarker(this); }
    size_t bytesRead() const { return rng_bytesRead(this); }
    bool isSeeded() const { return seeded; };

    uint8_t  getU8() { return rng_getU8(this); }
    uint16_t getU16() { return rng_getU16(this); }
    uint32_t getU32() { return rng_getU32(this); }
    uint64_t getU64() { return rng_getU64(this); }
    int8_t   getI8() { return rng_getI8(this); }
    int16_t  getI16() { return rng_getI16(this); }
    int32_t  getI32() { return rng_getI32(this); }
    int64_t  getI64() { return rng_getI64(this); }
    float    getF32() { return rng_getF32(this); }
    double   getF64() { return rng_getF64(this); }

    int rand() { return rng_rand(this); }
    int uniform(int a, int b) { return rng_uniform(this, a, b); }
    int discrete(const unsigned int* c, int s) { return rng_discrete(this, c, s); }
    int normal(int m, int d) { return rng_normal(this, m, d); }

private:
    bool seeded;
    uint8_t seed[256];
    uint8_t seed_size;
    uint8_t buf[256];
    uint8_t i1;
    uint8_t i2;
    size_t bytes_read;

    void seedImpl(const void*, size_t) { /* not needed - uses rng_seedBytes */ }
};

extern BaronyRNG local_rng;
extern BaronyRNG net_rng;
extern BaronyRNG map_rng;
extern BaronyRNG map_server_rng;
extern BaronyRNG map_sequence_rng;

#else

typedef struct BaronyRNG {
    bool seeded;
    uint8_t seed[256];
    uint8_t seed_size;
    uint8_t buf[256];
    uint8_t i1;
    uint8_t i2;
    size_t bytes_read;
} BaronyRNG;

extern BaronyRNG local_rng;
extern BaronyRNG net_rng;
extern BaronyRNG map_rng;
extern BaronyRNG map_server_rng;
extern BaronyRNG map_sequence_rng;

#endif
