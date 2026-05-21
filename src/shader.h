#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
#include <vector>
#include <unordered_map>
#include <string>
#endif

#define SHADER_VERTEX 0
#define SHADER_GEOMETRY 1
#define SHADER_FRAGMENT 2

typedef struct UniformEntry {
    char name[128];
    int location;
} UniformEntry;

// Forward declaration for C function declarations
#ifdef __cplusplus
class Shader;
extern "C" {
#else
typedef struct Shader Shader;
#endif

void shader_init(Shader* sh, const char* name);
void shader_destroy(Shader* sh);
bool shader_bind(Shader* sh);
void shader_unbind(void);
int shader_uniform(Shader* sh, const char* name);
bool shader_isInitialized(const Shader* sh);
void shader_bindAttribLocation(Shader* sh, const char* attribute, int location);
bool shader_compile(Shader* sh, const char* source, size_t len, int type);
bool shader_link(Shader* sh);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class Shader {
public:
    Shader() { memset(this, 0, sizeof(*this)); name = "untitled"; }

    void init(const char* n) { shader_init(this, n); }
    void destroy() { shader_destroy(this); }
    bool bind() { return shader_bind(this); }
    static void unbind() { shader_unbind(); }
    int uniform(const char* n) { return shader_uniform(this, n); }
    bool isInitialized() const { return program != 0; }

    enum class Type {
        Vertex = SHADER_VERTEX,
        Geometry = SHADER_GEOMETRY,
        Fragment = SHADER_FRAGMENT,
    };

    void bindAttribLocation(const char* attribute, int location) { shader_bindAttribLocation(this, attribute, location); }
    bool compile(const char* source, size_t len, Type type) { return shader_compile(this, source, len, (int)type); }
    bool link() { return shader_link(this); }

    bool operator==(const Shader& rhs) const {
        return program == rhs.program;
    }

private:
    const char* name;
    unsigned int program;
    unsigned int* shaders;
    int shaders_count;
    int shaders_capacity;
    UniformEntry* uniforms;
    int uniforms_count;
    int uniforms_capacity;
};

#else

typedef struct Shader {
    const char* name;
    unsigned int program;
    unsigned int* shaders;
    int shaders_count;
    int shaders_capacity;
    UniformEntry* uniforms;
    int uniforms_count;
    int uniforms_capacity;
} Shader;

#endif
