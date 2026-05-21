#include "shader.h"
#include "defs.h"
#include <stdlib.h>
#include <string.h>

static unsigned int currentActiveShader = 0;

void shader_init(Shader* sh, const char* name) {
    if (sh->program != 0) {
        return;
    }
    sh->name = name;
    sh->program = GL_CHECK_ERR_RET(glCreateProgram());
    if (sh->program) {
        printlog("initialized shader program '%s' successfully", name);
    } else {
        printlog("failed to initialize shader program '%s'", name);
    }
}

void shader_destroy(Shader* sh) {
    if (sh->program) {
        for (int i = 0; i < sh->shaders_count; ++i) {
            if (sh->shaders[i]) {
                GL_CHECK_ERR(glDetachShader(sh->program, sh->shaders[i]));
                GL_CHECK_ERR(glDeleteShader(sh->shaders[i]));
            }
        }
        GL_CHECK_ERR(glDeleteProgram(sh->program));
        free(sh->shaders);
        sh->shaders = NULL;
        sh->shaders_count = 0;
        sh->shaders_capacity = 0;
        free(sh->uniforms);
        sh->uniforms = NULL;
        sh->uniforms_count = 0;
        sh->uniforms_capacity = 0;
        sh->program = 0;
    }
}

bool shader_bind(Shader* sh) {
    if (currentActiveShader != sh->program) {
        GL_CHECK_ERR(glUseProgram(sh->program));
        currentActiveShader = sh->program;
    }
    return sh->program != 0;
}

void shader_unbind(void) {
    if (currentActiveShader) {
        GL_CHECK_ERR(glUseProgram(0));
        currentActiveShader = 0;
    }
}

bool shader_isInitialized(const Shader* sh) {
    return sh->program != 0;
}

int shader_uniform(Shader* sh, const char* name) {
    for (int i = 0; i < sh->uniforms_count; ++i) {
        if (strcmp(sh->uniforms[i].name, name) == 0) {
            return sh->uniforms[i].location;
        }
    }
    int handle = GL_CHECK_ERR_RET(glGetUniformLocation(sh->program, (GLchar*)name));
    if (handle == -1) {
        printlog("uniform %s not found!", name);
    }
    if (sh->uniforms_count >= sh->uniforms_capacity) {
        int newcap = sh->uniforms_capacity ? sh->uniforms_capacity * 2 : 8;
        UniformEntry* newu = (UniformEntry*)realloc(sh->uniforms, (size_t)newcap * sizeof(UniformEntry));
        if (!newu) return handle;
        sh->uniforms = newu;
        sh->uniforms_capacity = newcap;
    }
    UniformEntry* entry = &sh->uniforms[sh->uniforms_count++];
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    entry->name[sizeof(entry->name) - 1] = '\0';
    entry->location = handle;
    return handle;
}

void shader_bindAttribLocation(Shader* sh, const char* attribute, int location) {
    GL_CHECK_ERR(glBindAttribLocation(sh->program, location, attribute));
}

bool shader_compile(Shader* sh, const char* source, size_t len, int type) {
    GLenum glType;
    switch (type) {
    default: return false;
    case SHADER_VERTEX: glType = GL_VERTEX_SHADER; break;
    case SHADER_GEOMETRY: glType = GL_GEOMETRY_SHADER; break;
    case SHADER_FRAGMENT: glType = GL_FRAGMENT_SHADER; break;
    }

    const char version[] = "#version 150 core\n";
    const char* sources[2] = {version, source};
    const int lens[2] = {(int)sizeof(version) - 1, (int)len};

    unsigned int shaderObj = GL_CHECK_ERR_RET(glCreateShader(glType));
    GL_CHECK_ERR(glShaderSource(shaderObj, 2, sources, lens));
    GL_CHECK_ERR(glCompileShader(shaderObj));

    GLint status;
    GL_CHECK_ERR(glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &status));
    if (status) {
        GL_CHECK_ERR(glAttachShader(sh->program, shaderObj));
        if (sh->shaders_count >= sh->shaders_capacity) {
            int newcap = sh->shaders_capacity ? sh->shaders_capacity * 2 : 8;
            unsigned int* news = (unsigned int*)realloc(sh->shaders, (size_t)newcap * sizeof(unsigned int));
            if (!news) {
                GL_CHECK_ERR(glDeleteShader(shaderObj));
                return false;
            }
            sh->shaders = news;
            sh->shaders_capacity = newcap;
        }
        sh->shaders[sh->shaders_count++] = shaderObj;
        printlog("compiled shader %d successfully", sh->shaders_count);
        return true;
    } else {
        char log[1024];
        GL_CHECK_ERR(glGetShaderInfoLog(shaderObj, (GLint)sizeof(log), NULL, (GLchar*)log));
        printlog("failed to compile shader: %s", log);
        GL_CHECK_ERR(glDeleteShader(shaderObj));
        return false;
    }
}

bool shader_link(Shader* sh) {
    sh->uniforms_count = 0;
    GL_CHECK_ERR(glLinkProgram(sh->program));

    GLint status;
    GL_CHECK_ERR(glGetProgramiv(sh->program, GL_LINK_STATUS, &status));
    if (status) {
        printlog("linked shader program '%s' successfully", sh->name);
        return true;
    } else {
        char log[1024];
        GL_CHECK_ERR(glGetProgramInfoLog(sh->program, sizeof(log), NULL, (GLchar*)log));
        printlog("failed to link shaders for '%s': %s", sh->name, log);
        return false;
    }
}

#ifdef __cplusplus
void Shader::init(const char* name) { shader_init(this, name); }
void Shader::destroy() { shader_destroy(this); }
bool Shader::bind() { return shader_bind(this); }
void Shader::unbind() { shader_unbind(); }
int Shader::uniform(const char* name) { return shader_uniform(this, name); }
void Shader::bindAttribLocation(const char* attribute, int location) { shader_bindAttribLocation(this, attribute, location); }
bool Shader::compile(const char* source, size_t len, Type type) { return shader_compile(this, source, len, (int)type); }
bool Shader::link() { return shader_link(this); }
#endif
