#include "prng.h"
#include "defs.h"
#include <assert.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <string.h>

static uint8_t marker[256];

BaronyRNG local_rng;
BaronyRNG net_rng;
BaronyRNG map_sequence_rng;

#ifdef __cplusplus
#include "interface/consolecommand.h"
#include "net.h"
static BaronyRNG test_rng;

static ConsoleCommand test_rng_seed(
    "/test_rng_seed",
    "seed test rng",
    [](int argc, const char* argv[]){
    if (argc < 2) {
        test_rng.seedTime();
    } else {
        auto seed = strtol(argv[1], nullptr, 10);
        test_rng.seedBytes(&seed, sizeof(seed));
    }
    });

static ConsoleCommand test_rng_seed_health(
    "/test_rng_seed_health",
    "test rng seed health",
    [](int argc, const char* argv[]){
    test_rng.testSeedHealth();
    });

static ConsoleCommand test_rng_u8(
    "/test_rng_u8",
    "test rng u8",
    [](int argc, const char* argv[]){
    const int i = argc > 1 ? (int)strtol(argv[1], nullptr, 10) : 100000;
    real_t sum = 0.0;
    for (int c = 0; c < i; ++c) {
        auto result = test_rng.getU8();
        sum += result;
    }
    sum /= i;
    messagePlayer(clientnum, MESSAGE_MISC, "mean: %.2f", sum);
    });

static ConsoleCommand test_rng_i8(
    "/test_rng_i8",
    "test rng i8",
    [](int argc, const char* argv[]){
    const int i = argc > 1 ? (int)strtol(argv[1], nullptr, 10) : 100000;
    real_t sum = 0.0;
    for (int c = 0; c < i; ++c) {
        auto result = test_rng.getI8();
        sum += result;
    }
    sum /= i;
    messagePlayer(clientnum, MESSAGE_MISC, "mean: %.2f", sum);
    });

static ConsoleCommand test_rng_f32(
    "/test_rng_f32",
    "test rng f32",
    [](int argc, const char* argv[]){
    const int i = argc > 1 ? (int)strtol(argv[1], nullptr, 10) : 100000;
    real_t sum = 0.0;
    for (int c = 0; c < i; ++c) {
        auto result = test_rng.getF32();
        sum += result;
    }
    sum /= i;
    messagePlayer(clientnum, MESSAGE_MISC, "mean: %.2f", sum);
    });

static ConsoleCommand test_rng_f64(
    "/test_rng_f64",
    "test rng f64",
    [](int argc, const char* argv[]){
    const int i = argc > 1 ? (int)strtol(argv[1], nullptr, 10) : 100000;
    real_t sum = 0.0;
    for (int c = 0; c < i; ++c) {
        auto result = test_rng.getF64();
        sum += result;
    }
    sum /= i;
    messagePlayer(clientnum, MESSAGE_MISC, "mean: %.2f", sum);
    });

static ConsoleCommand test_rng_uniform(
    "/test_rng_uniform",
    "test rng with uniform(a, b, iterations)",
    [](int argc, const char* argv[]){
    const int a = argc > 1 ? (int)strtol(argv[1], nullptr, 10) : -10;
    const int b = argc > 2 ? (int)strtol(argv[2], nullptr, 10) : 10;
    const int i = argc > 3 ? (int)strtol(argv[3], nullptr, 10) : 100000;
    real_t sum = 0.0;
    for (int c = 0; c < i; ++c) {
        int result = test_rng.uniform(a, b);
        sum += result;
    }
    sum /= i;
    messagePlayer(clientnum, MESSAGE_MISC, "mean: %.2f", sum);
    });

static ConsoleCommand test_rng_discrete(
    "/test_rng_discrete",
    "test rng with discrete({chances}, iterations)",
    [](int argc, const char* argv[]){
    if (argc < 3) {
        messagePlayer(clientnum, MESSAGE_MISC, "args: {chances} iterations");
        return;
    }
    std::vector<unsigned int> chances;
    for (int c = 1; c < argc - 1; ++c) {
        unsigned int chance = (int)strtol(argv[c], nullptr, 10);
        chances.push_back(chance);
    }
    const int i = (int)strtol(argv[argc - 1], nullptr, 10);
    real_t sum = 0.0;
    for (int c = 0; c < i; ++c) {
        int result = test_rng.discrete(chances.data(), chances.size());
        sum += result;
    }
    sum /= i;
    messagePlayer(clientnum, MESSAGE_MISC, "mean: %.2f", sum);
    });

static ConsoleCommand test_rng_normal(
    "/test_rng_normal",
    "test rng with normal(mean, deviation, iterations)",
    [](int argc, const char* argv[]){
    const int m = argc > 1 ? (int)strtol(argv[1], nullptr, 10) : 0;
    const int d = argc > 2 ? (int)strtol(argv[2], nullptr, 10) : 5;
    const int i = argc > 3 ? (int)strtol(argv[3], nullptr, 10) : 100000;
    std::map<int, int> hist{};
    for (int c = 0; c < i; ++c) {
        int result = test_rng.normal(m, d);
        ++hist[result];
    }
    for (auto p : hist) {
        int value = p.second / 200;
        if (value) {
            std::string s(value, '*');
            messagePlayer(clientnum, MESSAGE_MISC, "%5d %s", p.first, s.c_str());
        }
    }
    });
#endif

static void swap_byte(uint8_t* a, uint8_t* b) {
    uint8_t t = *a;
    *a = *b;
    *b = t;
}

static void seedImpl(BaronyRNG* rng, const void* key, size_t size) {
    assert(key != NULL && size > 0);
    for (int i = 0; i < 256; ++i) {
        rng->buf[i] = (uint8_t)i;
    }

    uint8_t b = 0;
    const uint8_t* bytes = (const uint8_t*)key;
    for (int i = 0; i < 256; ++i) {
        b = b + rng->buf[i] + bytes[i % size];
        swap_byte(&rng->buf[i], &rng->buf[b]);
    }

    memcpy(rng->seed, key, size);
    rng->seed_size = (uint8_t)size;

    rng->i1 = rng->i2 = 0;
    rng->bytes_read = 0;
    rng->seeded = true;
}

void rng_seedBytes(BaronyRNG* rng, const void* key, size_t size) {
    seedImpl(rng, key, size);
}

void rng_seedTime(BaronyRNG* rng) {
    uint32_t t = (uint32_t)getTime();
    seedImpl(rng, &t, sizeof(t));
}

int rng_getSeed(const BaronyRNG* rng, void* out, size_t size) {
    if (!rng->seeded || size < rng->seed_size) {
        assert(0 && "wtf are you doin");
        return -1;
    }
    memcpy(out, rng->seed, rng->seed_size);
    return (int)rng->seed_size;
}

void rng_getBytes(BaronyRNG* rng, void* data_, size_t size) {
    if (!rng->seeded) {
        printlog("rng not seeded, seeding by unix time");
        uint32_t t = (uint32_t)getTime();
        seedImpl(rng, &t, sizeof(t));
    }
    for (uint8_t* data = (uint8_t*)data_; size-- > 0; ++data) {
        rng->i1 = (uint8_t)(((int)rng->i1 + 1) & 255);
        rng->i2 = (uint8_t)(((int)rng->i2 + rng->buf[rng->i1]) & 255);
        swap_byte(&rng->buf[rng->i1], &rng->buf[rng->i2]);
        *data = rng->buf[((int)rng->buf[rng->i1] + (int)rng->buf[rng->i2]) & 255];
        ++rng->bytes_read;
    }
}

uint8_t rng_getU8(BaronyRNG* rng) {
    uint8_t result;
    rng_getBytes(rng, &result, sizeof(result));
    return result;
}

uint16_t rng_getU16(BaronyRNG* rng) {
    uint16_t result;
    rng_getBytes(rng, &result, sizeof(result));
    return result;
}

uint32_t rng_getU32(BaronyRNG* rng) {
    uint32_t result;
    rng_getBytes(rng, &result, sizeof(result));
    return result;
}

uint64_t rng_getU64(BaronyRNG* rng) {
    uint64_t result;
    rng_getBytes(rng, &result, sizeof(result));
    return result;
}

int8_t rng_getI8(BaronyRNG* rng) {
    int8_t result;
    rng_getBytes(rng, &result, sizeof(result));
    return result;
}

int16_t rng_getI16(BaronyRNG* rng) {
    int16_t result;
    rng_getBytes(rng, &result, sizeof(result));
    return result;
}

int32_t rng_getI32(BaronyRNG* rng) {
    int32_t result;
    rng_getBytes(rng, &result, sizeof(result));
    return result;
}

int64_t rng_getI64(BaronyRNG* rng) {
    int64_t result;
    rng_getBytes(rng, &result, sizeof(result));
    return result;
}

float rng_getF32(BaronyRNG* rng) {
    (void)rng;
    uint32_t u32;
    rng_getBytes(rng, &u32, sizeof(u32));
    static const uint64_t div = (uint64_t)1 << 32;
    return (float)u32 / (float)div;
}

double rng_getF64(BaronyRNG* rng) {
    (void)rng;
    uint32_t u32;
    rng_getBytes(rng, &u32, sizeof(u32));
    static const uint64_t div = (uint64_t)1 << 32;
    return (double)u32 / (double)div;
}

int rng_rand(BaronyRNG* rng) {
    int i;
    rng_getBytes(rng, &i, sizeof(i));
    return i & 0x7fffffff;
}

int rng_uniform(BaronyRNG* rng, int a, int b) {
    (void)rng;
    if (a == b) {
        return a;
    }
    int min = (a < b ? a : b);
    int max = (a > b ? a : b);
    int diff = (max - min) + 1;
    int choice = (int)(rng_getF64(rng) * (double)diff);
    return min + choice;
}

int rng_discrete(BaronyRNG* rng, const unsigned int* chances, int size) {
    (void)rng;
    if (size <= 0) {
        assert(0 && "BaronyRNG::discrete() list is less-or-equal than 0");
        return 0;
    }

    unsigned int total = 0;
    for (int c = 0; c < size; ++c) {
        total += chances[c];
    }
    if (total == 0) {
        assert(0 && "BaronyRNG::discrete() chances of picking anything are 0");
        return 0;
    }

    unsigned int choice = (unsigned int)(rng_getF64(rng) * (double)total);
    for (int c = 0; c < size; ++c) {
        if (chances[c] > choice) {
            return c;
        } else {
            choice -= chances[c];
        }
    }

    assert(0 && "BaronyRNG::discrete() nothing was picked. this should never happen");
    return 0;
}

int rng_normal(BaronyRNG* rng, int mean, int deviation) {
    (void)rng;
    const real_t m = (real_t)mean;
    const real_t d = (real_t)deviation;
    const real_t f1 = rng_getF64(rng);
    const real_t f2 = rng_getF64(rng);
    const real_t norm = cos(2.0 * PI * f2) * sqrt(-2.0 * log(f1));
    return (int)round(norm * d + m);
}

void rng_setMarker(const BaronyRNG* rng) {
#ifndef NDEBUG
    memcpy(marker, rng->buf, sizeof(marker));
#endif
    (void)rng;
}

void rng_checkMarker(const BaronyRNG* rng) {
#ifndef NDEBUG
    if (!memcmp(marker, rng->buf, sizeof(marker))) {
        printlog("reached marker");
    }
#endif
    (void)rng;
}

size_t rng_bytesRead(const BaronyRNG* rng) {
    return rng->bytes_read;
}

bool rng_isSeeded(const BaronyRNG* rng) {
    return rng->seeded;
}

void rng_testSeedHealth(const BaronyRNG* rng) {
    real_t sum = 0.0;
    for (int c = 0; c < 256; ++c) {
        for (int b = 0; b < 8; ++b) {
            if (rng->buf[c] & (1 << b)) {
                sum += 1.0;
            }
        }
    }
    sum /= 2048.0;
    printlog("rng seed bits are %.2f%% on", sum * 100.0);
}

#ifdef __cplusplus
void BaronyRNG::seedImpl(const void* key, size_t size) { ::seedImpl(this, key, size); }
void BaronyRNG::seedBytes(const void* key, size_t size) { rng_seedBytes(this, key, size); }
void BaronyRNG::seedTime() { rng_seedTime(this); }
int BaronyRNG::getSeed(void* out, size_t size) const { return rng_getSeed(this, out, size); }
void BaronyRNG::getBytes(void* data_, size_t size) { rng_getBytes(this, data_, size); }
uint8_t BaronyRNG::getU8() { return rng_getU8(this); }
uint16_t BaronyRNG::getU16() { return rng_getU16(this); }
uint32_t BaronyRNG::getU32() { return rng_getU32(this); }
uint64_t BaronyRNG::getU64() { return rng_getU64(this); }
int8_t BaronyRNG::getI8() { return rng_getI8(this); }
int16_t BaronyRNG::getI16() { return rng_getI16(this); }
int32_t BaronyRNG::getI32() { return rng_getI32(this); }
int64_t BaronyRNG::getI64() { return rng_getI64(this); }
float BaronyRNG::getF32() { return rng_getF32(this); }
double BaronyRNG::getF64() { return rng_getF64(this); }
int BaronyRNG::rand() { return rng_rand(this); }
int BaronyRNG::uniform(int a, int b) { return rng_uniform(this, a, b); }
int BaronyRNG::discrete(const unsigned int* chances, int size) { return rng_discrete(this, chances, size); }
int BaronyRNG::normal(int mean, int deviation) { return rng_normal(this, mean, deviation); }
void BaronyRNG::setMarker() const { rng_setMarker(this); }
void BaronyRNG::checkMarker() const { rng_checkMarker(this); }
size_t BaronyRNG::bytesRead() const { return rng_bytesRead(this); }
void BaronyRNG::testSeedHealth() const { rng_testSeedHealth(this); }
#endif
