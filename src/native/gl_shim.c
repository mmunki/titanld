/*
 * TitanGL GL front-end.
 *
 * Only the functions that need translation, emulation or a fake desktop
 * query are implemented here.  Everything return by eglGetProcAddress that is
 * not in this file comes straight from the system GLES driver.
 */
#include "titangl.h"
#include "glsl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef GL_MAJOR_VERSION
#define GL_MAJOR_VERSION 0x821B
#endif
#ifndef GL_MINOR_VERSION
#define GL_MINOR_VERSION 0x821C
#endif
#ifndef GL_CONTEXT_PROFILE_MASK
#define GL_CONTEXT_PROFILE_MASK 0x9126
#endif
#ifndef GL_CONTEXT_FLAGS
#define GL_CONTEXT_FLAGS 0x821E
#endif
#ifndef GL_CONTEXT_CORE_PROFILE_BIT
#define GL_CONTEXT_CORE_PROFILE_BIT 0x00000001
#endif
#ifndef GL_NUM_EXTENSIONS
#define GL_NUM_EXTENSIONS 0x821D
#endif
#ifndef GL_READ_ONLY
#define GL_READ_ONLY 0x88B8
#endif
#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY 0x88B9
#endif
#ifndef GL_READ_WRITE
#define GL_READ_WRITE 0x88BA
#endif
#ifndef GL_TEXTURE_1D
#define GL_TEXTURE_1D 0x0DE0
#endif
#ifndef GL_TEXTURE_2D
#define GL_TEXTURE_2D 0x0DE1
#endif
#ifndef GL_ALPHA_TEST
#define GL_ALPHA_TEST 0x0BC0
#endif
#ifndef GL_COLOR_MATERIAL
#define GL_COLOR_MATERIAL 0x0B57
#endif
#ifndef GL_FOG
#define GL_FOG 0x0B60
#endif
#ifndef GL_LIGHTING
#define GL_LIGHTING 0x0B50
#endif
#ifndef GL_POLYGON_SMOOTH
#define GL_POLYGON_SMOOTH 0x0B41
#endif
#ifndef GL_LINE_STIPPLE
#define GL_LINE_STIPPLE 0x0B24
#endif
#ifndef GL_POLYGON_STIPPLE
#define GL_POLYGON_STIPPLE 0x0B42
#endif
#ifndef GL_NORMALIZE
#define GL_NORMALIZE 0x0BA1
#endif
#ifndef GL_AUTO_NORMAL
#define GL_AUTO_NORMAL 0x0D80
#endif
#ifndef GL_TEXTURE_GEN_S
#define GL_TEXTURE_GEN_S 0x0C60
#endif
#ifndef GL_TEXTURE_GEN_T
#define GL_TEXTURE_GEN_T 0x0C61
#endif
#ifndef GL_TEXTURE_GEN_R
#define GL_TEXTURE_GEN_R 0x0C62
#endif
#ifndef GL_TEXTURE_GEN_Q
#define GL_TEXTURE_GEN_Q 0x0C63
#endif

typedef double GLdouble;

#define DECL(name) static __typeof__(&name) real_##name

DECL(glShaderSource);
DECL(glCompileShader);
DECL(glCreateShader);
DECL(glDeleteShader);
DECL(glGetShaderiv);
DECL(glGetShaderInfoLog);
DECL(glAttachShader);
DECL(glDetachShader);
DECL(glCreateProgram);
DECL(glDeleteProgram);
DECL(glLinkProgram);
DECL(glGetProgramiv);
DECL(glGetProgramInfoLog);
DECL(glBindAttribLocation);
DECL(glGetAttribLocation);
DECL(glGetUniformLocation);

DECL(glGetString);
DECL(glGetStringi);
DECL(glGetIntegerv);
DECL(glGetFloatv);
DECL(glGetBooleanv);
DECL(glGetError);
DECL(glEnable);
DECL(glDisable);
DECL(glClearDepthf);
DECL(glDepthRangef);
DECL(glLineWidth);
DECL(glMapBufferRange);
DECL(glUnmapBuffer);
DECL(glFlushMappedBufferRange);
DECL(glGetBufferParameteriv);
DECL(glBindBuffer);
DECL(glBufferData);
DECL(glGenBuffers);
DECL(glDeleteBuffers);
DECL(glDrawArrays);
DECL(glDrawElements);
DECL(glDrawArraysInstanced);
DECL(glDrawElementsInstanced);
DECL(glBlendFunc);
DECL(glBlendFuncSeparate);
DECL(glBlendEquation);
DECL(glBlendColor);
DECL(glDepthFunc);
DECL(glDepthMask);
DECL(glColorMask);
DECL(glCullFace);
DECL(glFrontFace);
DECL(glPixelStorei);
DECL(glPolygonOffset);
DECL(glClear);
DECL(glClearColor);
DECL(glClearStencil);
DECL(glViewport);
DECL(glScissor);
DECL(glActiveTexture);
DECL(glBindTexture);
DECL(glGenTextures);
DECL(glDeleteTextures);
DECL(glTexImage2D);
DECL(glTexSubImage2D);
DECL(glTexParameteri);
DECL(glTexParameterf);
DECL(glGenerateMipmap);
DECL(glUseProgram);
DECL(glUniform1i);
DECL(glUniform1f);
DECL(glUniform2f);
DECL(glUniform3f);
DECL(glUniform4f);
DECL(glUniform1iv);
DECL(glUniform1fv);
DECL(glUniform2fv);
DECL(glUniform3fv);
DECL(glUniform4fv);
DECL(glUniformMatrix3fv);
DECL(glUniformMatrix4fv);
DECL(glEnableVertexAttribArray);
DECL(glDisableVertexAttribArray);
DECL(glVertexAttribPointer);
DECL(glVertexAttribIPointer);
DECL(glVertexAttribDivisor);
DECL(glGenVertexArrays);
DECL(glBindVertexArray);
DECL(glDeleteVertexArrays);
DECL(glBindFramebuffer);
DECL(glGenFramebuffers);
DECL(glDeleteFramebuffers);
DECL(glFramebufferTexture2D);
DECL(glFramebufferRenderbuffer);
DECL(glCheckFramebufferStatus);
DECL(glGenRenderbuffers);
DECL(glDeleteRenderbuffers);
DECL(glBindRenderbuffer);
DECL(glRenderbufferStorage);
DECL(glReadPixels);
DECL(glDrawBuffers);
DECL(glFinish);
DECL(glFlush);
DECL(glStencilFunc);
DECL(glStencilOp);
DECL(glStencilMask);
DECL(glTexStorage2D);
DECL(glInvalidateFramebuffer);
DECL(glFenceSync);
DECL(glClientWaitSync);
DECL(glDeleteSync);
DECL(glWaitSync);
DECL(glGetSynciv);
DECL(glCopyBufferSubData);
DECL(glGetActiveUniform);
DECL(glGetActiveAttrib);
DECL(glGetUniformBlockIndex);
DECL(glUniformBlockBinding);
DECL(glGetActiveUniformBlockiv);
DECL(glGetActiveUniformBlockName);

typedef void (*PFN_glBindFragDataLocationEXT)(GLuint program, GLuint colorNumber,
                                             const GLchar *name);
static PFN_glBindFragDataLocationEXT real_glBindFragDataLocationEXT;

#define LOAD_REAL(name)                                                        \
    do {                                                                       \
        if (!real_##name) {                                                     \
            real_##name = (__typeof__(real_##name))titangl_get_proc(#name);     \
        }                                                                      \
    } while (0)

#define FWD_VOID(name, params, args)                                           \
    void name params                                                           \
    {                                                                          \
        LOAD_REAL(name);                                                        \
        if (real_##name) {                                                      \
            real_##name args;                                                   \
        }                                                                      \
    }

#define FWD(ret, name, params, args)                                           \
    ret name params                                                            \
    {                                                                          \
        LOAD_REAL(name);                                                        \
        if (!real_##name) {                                                     \
            return (ret)0;                                                      \
        }                                                                      \
        return real_##name args;                                                \
    }

/* ----------------------------------------------------------- shader state */

#define TITANGL_MAX_SHADERS 2048
#define TITANGL_MAX_PROGRAMS 512
#define TITANGL_MAX_PROGRAM_SHADERS 16

typedef struct {
    GLuint id;
    int used;
    int is_vertex;
    int translated_ok;
    titangl_shader translated;
} shader_slot;

typedef struct {
    GLuint id;
    int used;
    GLuint shaders[TITANGL_MAX_PROGRAM_SHADERS];
    int shader_count;
} program_slot;

static shader_slot g_shaders[TITANGL_MAX_SHADERS];
static program_slot g_programs[TITANGL_MAX_PROGRAMS];
static pthread_mutex_t g_state_lock = PTHREAD_MUTEX_INITIALIZER;

static shader_slot *shader_slot_locked(GLuint id, int create)
{
    int free_slot = -1;
    for (int i = 0; i < TITANGL_MAX_SHADERS; i++) {
        if (g_shaders[i].used && g_shaders[i].id == id) {
            return &g_shaders[i];
        }
        if (!g_shaders[i].used && free_slot < 0) {
            free_slot = i;
        }
    }
    if (!create || free_slot < 0) {
        return NULL;
    }
    shader_slot *s = &g_shaders[free_slot];
    memset(s, 0, sizeof(*s));
    s->used = 1;
    s->id = id;
    return s;
}

static program_slot *program_slot_locked(GLuint id, int create)
{
    int free_slot = -1;
    for (int i = 0; i < TITANGL_MAX_PROGRAMS; i++) {
        if (g_programs[i].used && g_programs[i].id == id) {
            return &g_programs[i];
        }
        if (!g_programs[i].used && free_slot < 0) {
            free_slot = i;
        }
    }
    if (!create || free_slot < 0) {
        return NULL;
    }
    program_slot *p = &g_programs[free_slot];
    memset(p, 0, sizeof(*p));
    p->used = 1;
    p->id = id;
    return p;
}

static int shader_type_is_vertex(GLuint shader)
{
    int is_vertex = -1;
    pthread_mutex_lock(&g_state_lock);
    shader_slot *s = shader_slot_locked(shader, 0);
    if (s) {
        is_vertex = s->is_vertex;
    }
    pthread_mutex_unlock(&g_state_lock);
    if (is_vertex >= 0) {
        return is_vertex;
    }
    LOAD_REAL(glGetShaderiv);
    if (real_glGetShaderiv) {
        GLint type = 0;
        real_glGetShaderiv(shader, GL_SHADER_TYPE, &type);
        is_vertex = (type == GL_VERTEX_SHADER);
        pthread_mutex_lock(&g_state_lock);
        s = shader_slot_locked(shader, 1);
        if (s) {
            s->is_vertex = is_vertex;
        }
        pthread_mutex_unlock(&g_state_lock);
    }
    return is_vertex > 0;
}

/* --------------------------------------------------------- shader entry */

GLuint glCreateShader(GLenum type)
{
    LOAD_REAL(glCreateShader);
    if (!real_glCreateShader) {
        return 0;
    }
    GLuint id = real_glCreateShader(type);
    if (id) {
        pthread_mutex_lock(&g_state_lock);
        shader_slot *s = shader_slot_locked(id, 1);
        if (s) {
            s->is_vertex = (type == GL_VERTEX_SHADER);
        }
        pthread_mutex_unlock(&g_state_lock);
    }
    return id;
}

void glDeleteShader(GLuint shader)
{
    LOAD_REAL(glDeleteShader);
    pthread_mutex_lock(&g_state_lock);
    shader_slot *s = shader_slot_locked(shader, 0);
    if (s) {
        titangl_shader_free(&s->translated);
        memset(s, 0, sizeof(*s));
    }
    pthread_mutex_unlock(&g_state_lock);
    if (real_glDeleteShader) {
        real_glDeleteShader(shader);
    }
}

void glShaderSource(GLuint shader, GLsizei count, const GLchar *const *string,
                    const GLint *length)
{
    LOAD_REAL(glShaderSource);
    if (!real_glShaderSource || count <= 0 || !string) {
        if (real_glShaderSource) {
            real_glShaderSource(shader, count, string, length);
        }
        return;
    }

    size_t total = 0;
    for (GLsizei i = 0; i < count; i++) {
        if (!string[i]) {
            continue;
        }
        total += (length && length[i] >= 0) ? (size_t)length[i] : strlen(string[i]);
        total += 1;
    }
    char *joined = (char *)malloc(total + 1);
    if (!joined) {
        real_glShaderSource(shader, count, string, length);
        return;
    }
    size_t at = 0;
    for (GLsizei i = 0; i < count; i++) {
        if (!string[i]) {
            continue;
        }
        size_t n = (length && length[i] >= 0) ? (size_t)length[i] : strlen(string[i]);
        memcpy(joined + at, string[i], n);
        at += n;
        joined[at++] = '\n';
    }
    joined[at] = '\0';

    int is_vertex = shader_type_is_vertex(shader);
    titangl_shader translated;
    memset(&translated, 0, sizeof(translated));
    int ok = titangl_translate_shader(joined, at, is_vertex, &translated);
    free(joined);

    if (ok != 0) {
        TLOG(TITANGL_WARN, "shader %u: translation failed (%s), compiling the original source",
             shader, translated.error);
        real_glShaderSource(shader, count, string, length);
        titangl_shader_free(&translated);
        return;
    }

    pthread_mutex_lock(&g_state_lock);
    shader_slot *s = shader_slot_locked(shader, 1);
    const GLchar *stable = NULL;
    if (s) {
        titangl_shader_free(&s->translated);
        s->translated = translated;
        s->translated_ok = 1;
        s->is_vertex = is_vertex;
        stable = s->translated.text;
    } else {
        titangl_shader_free(&translated);
    }
    pthread_mutex_unlock(&g_state_lock);

    if (stable) {
        real_glShaderSource(shader, 1, &stable, NULL);
    } else {
        real_glShaderSource(shader, count, string, length);
    }
}

void glCompileShader(GLuint shader)
{
    LOAD_REAL(glCompileShader);
    if (!real_glCompileShader) {
        return;
    }
    real_glCompileShader(shader);
    LOAD_REAL(glGetShaderiv);
    LOAD_REAL(glGetShaderInfoLog);
    if (real_glGetShaderiv && real_glGetShaderInfoLog) {
        GLint compiled = 0;
        real_glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            char log[4096];
            GLsizei written = 0;
            real_glGetShaderInfoLog(shader, sizeof(log) - 1, &written, log);
            if (written < 0) {
                written = 0;
            }
            log[written < (GLsizei)sizeof(log) ? written : (GLsizei)sizeof(log) - 1] = '\0';
            TLOG(TITANGL_ERROR, "shader %u failed to compile:\n%s", shader, log);
        }
    }
}

void glAttachShader(GLuint program, GLuint shader)
{
    LOAD_REAL(glAttachShader);
    pthread_mutex_lock(&g_state_lock);
    program_slot *p = program_slot_locked(program, 1);
    if (p) {
        int found = 0;
        for (int i = 0; i < p->shader_count; i++) {
            if (p->shaders[i] == shader) {
                found = 1;
            }
        }
        if (!found && p->shader_count < TITANGL_MAX_PROGRAM_SHADERS) {
            p->shaders[p->shader_count++] = shader;
        }
    }
    pthread_mutex_unlock(&g_state_lock);
    if (real_glAttachShader) {
        real_glAttachShader(program, shader);
    }
}

void glDetachShader(GLuint program, GLuint shader)
{
    LOAD_REAL(glDetachShader);
    pthread_mutex_lock(&g_state_lock);
    program_slot *p = program_slot_locked(program, 0);
    if (p) {
        for (int i = 0; i < p->shader_count; i++) {
            if (p->shaders[i] == shader) {
                for (int j = i; j + 1 < p->shader_count; j++) {
                    p->shaders[j] = p->shaders[j + 1];
                }
                p->shader_count--;
                break;
            }
        }
    }
    pthread_mutex_unlock(&g_state_lock);
    if (real_glDetachShader) {
        real_glDetachShader(program, shader);
    }
}

GLuint glCreateProgram(void)
{
    LOAD_REAL(glCreateProgram);
    if (!real_glCreateProgram) {
        return 0;
    }
    GLuint id = real_glCreateProgram();
    if (id) {
        pthread_mutex_lock(&g_state_lock);
        program_slot_locked(id, 1);
        pthread_mutex_unlock(&g_state_lock);
    }
    return id;
}

void glDeleteProgram(GLuint program)
{
    LOAD_REAL(glDeleteProgram);
    pthread_mutex_lock(&g_state_lock);
    program_slot *p = program_slot_locked(program, 0);
    if (p) {
        memset(p, 0, sizeof(*p));
    }
    pthread_mutex_unlock(&g_state_lock);
    if (real_glDeleteProgram) {
        real_glDeleteProgram(program);
    }
}

void glLinkProgram(GLuint program)
{
    LOAD_REAL(glLinkProgram);
    LOAD_REAL(glBindAttribLocation);
    LOAD_REAL(glGetProgramiv);
    LOAD_REAL(glGetProgramInfoLog);
    if (!real_glLinkProgram) {
        return;
    }

    titangl_attribute attrs[TITANGL_MAX_ATTRIBUTES];
    int attr_count = 0;
    pthread_mutex_lock(&g_state_lock);
    program_slot *p = program_slot_locked(program, 0);
    if (p && real_glBindAttribLocation) {
        for (int i = 0; i < p->shader_count && attr_count < TITANGL_MAX_ATTRIBUTES; i++) {
            shader_slot *s = shader_slot_locked(p->shaders[i], 0);
            if (!s || !s->translated_ok || !s->is_vertex) {
                continue;
            }
            for (int a = 0; a < s->translated.attribute_count &&
                            attr_count < TITANGL_MAX_ATTRIBUTES; a++) {
                attrs[attr_count++] = s->translated.attributes[a];
            }
        }
    }
    pthread_mutex_unlock(&g_state_lock);

    for (int i = 0; i < attr_count; i++) {
        real_glBindAttribLocation(program, (GLuint)attrs[i].location, attrs[i].name);
    }
    real_glLinkProgram(program);

    if (real_glGetProgramiv) {
        GLint linked = 0;
        real_glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked && real_glGetProgramInfoLog) {
            char log[4096];
            GLsizei written = 0;
            real_glGetProgramInfoLog(program, sizeof(log) - 1, &written, log);
            if (written < 0) {
                written = 0;
            }
            log[written < (GLsizei)sizeof(log) ? written : (GLsizei)sizeof(log) - 1] = '\0';
            TLOG(TITANGL_ERROR, "program %u failed to link:\n%s", program, log);
        }
    }
}

/* ------------------------------------------------------------ GL queries */

static char g_version_string[64];
static char g_glsl_string[64];
static char g_renderer_string[128];
static char g_vendor_string[32] = "TitanLD";
static char g_extensions[16384];
static int g_extensions_ready;

static void build_extension_string(void)
{
    LOAD_REAL(glGetString);
    LOAD_REAL(glGetStringi);
    LOAD_REAL(glGetIntegerv);
    if (g_extensions_ready) {
        return;
    }
    g_extensions[0] = '\0';
    size_t at = 0;
    if (real_glGetIntegerv && real_glGetStringi) {
        GLint count = 0;
        real_glGetIntegerv(GL_NUM_EXTENSIONS, &count);
        for (GLint i = 0; i < count && at + 1 < sizeof(g_extensions); i++) {
            const char *ext = (const char *)real_glGetStringi(GL_EXTENSIONS, (GLuint)i);
            if (!ext) {
                continue;
            }
            size_t n = strlen(ext);
            if (at + n + 2 >= sizeof(g_extensions)) {
                break;
            }
            if (at) {
                g_extensions[at++] = ' ';
            }
            memcpy(g_extensions + at, ext, n);
            at += n;
            g_extensions[at] = '\0';
        }
    } else if (real_glGetString) {
        const char *ext = (const char *)real_glGetString(GL_EXTENSIONS);
        if (ext) {
            snprintf(g_extensions, sizeof(g_extensions), "%s", ext);
            at = strlen(g_extensions);
        }
    }
    (void)at;
    g_extensions_ready = 1;
}

const GLubyte *glGetString(GLenum name)
{
    LOAD_REAL(glGetString);
    switch (name) {
    case GL_VERSION:
        snprintf(g_version_string, sizeof(g_version_string), "%d.%d.0 TitanGL",
                 titangl_config_state.fake_major, titangl_config_state.fake_minor);
        return (const GLubyte *)g_version_string;
    case GL_SHADING_LANGUAGE_VERSION:
        snprintf(g_glsl_string, sizeof(g_glsl_string), "4.60 TitanGL");
        return (const GLubyte *)g_glsl_string;
    case GL_RENDERER:
        snprintf(g_renderer_string, sizeof(g_renderer_string),
                 "TitanGL (TitanLD custom renderer)");
        return (const GLubyte *)g_renderer_string;
    case GL_VENDOR:
        return (const GLubyte *)g_vendor_string;
    case GL_EXTENSIONS:
        build_extension_string();
        return (const GLubyte *)g_extensions;
    default:
        return real_glGetString ? real_glGetString(name) : NULL;
    }
}

const GLubyte *glGetStringi(GLenum name, GLuint index)
{
    LOAD_REAL(glGetStringi);
    if (name == GL_VERSION) {
        snprintf(g_version_string, sizeof(g_version_string), "%d.%d.0 TitanGL",
                 titangl_config_state.fake_major, titangl_config_state.fake_minor);
        return (const GLubyte *)g_version_string;
    }
    return real_glGetStringi ? real_glGetStringi(name, index) : NULL;
}

void glGetIntegerv(GLenum pname, GLint *params)
{
    if (!params) {
        return;
    }
    switch (pname) {
    case GL_MAJOR_VERSION:
        *params = titangl_config_state.fake_major;
        return;
    case GL_MINOR_VERSION:
        *params = titangl_config_state.fake_minor;
        return;
    case GL_CONTEXT_PROFILE_MASK:
        *params = GL_CONTEXT_CORE_PROFILE_BIT;
        return;
    case GL_CONTEXT_FLAGS:
        *params = 0;
        return;
    default:
        break;
    }
    LOAD_REAL(glGetIntegerv);
    if (real_glGetIntegerv) {
        real_glGetIntegerv(pname, params);
    } else {
        *params = 0;
    }
}

static int pname_float_count(GLenum pname)
{
    switch (pname) {
    case GL_VIEWPORT:
    case GL_SCISSOR_BOX:
    case GL_COLOR_CLEAR_VALUE:
    case GL_BLEND_COLOR:
    case GL_COLOR_WRITEMASK:
        return 4;
    case GL_DEPTH_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
    case GL_MAX_VIEWPORT_DIMS:
        return 2;
    default:
        return 1;
    }
}

void glGetDoublev(GLenum pname, GLdouble *params)
{
    if (!params) {
        return;
    }
    int count = pname_float_count(pname);
    GLfloat values[16];
    memset(values, 0, sizeof(values));
    LOAD_REAL(glGetFloatv);
    if (real_glGetFloatv) {
        real_glGetFloatv(pname, values);
    } else {
        GLint ints[16] = {0};
        LOAD_REAL(glGetIntegerv);
        if (real_glGetIntegerv) {
            real_glGetIntegerv(pname, ints);
        }
        for (int i = 0; i < 16; i++) {
            values[i] = (GLfloat)ints[i];
        }
    }
    for (int i = 0; i < count; i++) {
        params[i] = (GLdouble)values[i];
    }
}

/* ------------------------------------------------------- state filtering */

static int is_desktop_only_cap(GLenum cap)
{
    switch (cap) {
    case GL_TEXTURE_1D:
    case GL_TEXTURE_2D:
    case GL_ALPHA_TEST:
    case GL_COLOR_MATERIAL:
    case GL_FOG:
    case GL_LIGHTING:
    case GL_POLYGON_SMOOTH:
    case GL_LINE_STIPPLE:
    case GL_POLYGON_STIPPLE:
    case GL_NORMALIZE:
    case GL_AUTO_NORMAL:
    case GL_TEXTURE_GEN_S:
    case GL_TEXTURE_GEN_T:
    case GL_TEXTURE_GEN_R:
    case GL_TEXTURE_GEN_Q:
        return 1;
    default:
        return 0;
    }
}

void glEnable(GLenum cap)
{
    if (is_desktop_only_cap(cap)) {
        return;
    }
    LOAD_REAL(glEnable);
    if (real_glEnable) {
        real_glEnable(cap);
    }
}

void glDisable(GLenum cap)
{
    if (is_desktop_only_cap(cap)) {
        return;
    }
    LOAD_REAL(glDisable);
    if (real_glDisable) {
        real_glDisable(cap);
    }
}

void glClearDepth(GLdouble depth)
{
    LOAD_REAL(glClearDepthf);
    if (real_glClearDepthf) {
        real_glClearDepthf((GLfloat)depth);
    }
}

void glDepthRange(GLdouble nearVal, GLdouble farVal)
{
    LOAD_REAL(glDepthRangef);
    if (real_glDepthRangef) {
        real_glDepthRangef((GLfloat)nearVal, (GLfloat)farVal);
    }
}

void glPointSize(GLfloat size)
{
    (void)size;
}

void glPolygonMode(GLenum face, GLenum mode)
{
    (void)face;
    (void)mode;
}

void glDrawBuffer(GLenum mode)
{
    (void)mode;
}

void glLineWidth(GLfloat width)
{
    LOAD_REAL(glLineWidth);
    if (real_glLineWidth) {
        real_glLineWidth(width > 1.0f ? 1.0f : width);
    }
}

/* ---------------------------------------------------------- buffer calls */

static GLenum map_access_to_flags(GLenum access)
{
    switch (access) {
    case GL_READ_ONLY:
        return GL_MAP_READ_BIT;
    case GL_WRITE_ONLY:
        return GL_MAP_WRITE_BIT;
    case GL_READ_WRITE:
        return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
    default:
        return GL_MAP_READ_BIT;
    }
}

void *glMapBuffer(GLenum target, GLenum access)
{
    LOAD_REAL(glMapBufferRange);
    LOAD_REAL(glGetBufferParameteriv);
    if (!real_glMapBufferRange || !real_glGetBufferParameteriv) {
        return NULL;
    }
    GLint size = 0;
    real_glGetBufferParameteriv(target, GL_BUFFER_SIZE, &size);
    if (size <= 0) {
        return NULL;
    }
    return real_glMapBufferRange(target, 0, size, map_access_to_flags(access));
}

void glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void *data)
{
    LOAD_REAL(glMapBufferRange);
    LOAD_REAL(glUnmapBuffer);
    LOAD_REAL(glGetBufferParameteriv);
    if (!data || size <= 0 || !real_glMapBufferRange || !real_glUnmapBuffer ||
        !real_glGetBufferParameteriv) {
        return;
    }
    GLint buffer_size = 0;
    real_glGetBufferParameteriv(target, GL_BUFFER_SIZE, &buffer_size);
    if (offset < 0 || size > buffer_size || offset > buffer_size - size) {
        return;
    }
    void *mapped = real_glMapBufferRange(target, 0, buffer_size, GL_MAP_READ_BIT);
    if (!mapped) {
        return;
    }
    memcpy(data, (const char *)mapped + offset, (size_t)size);
    real_glUnmapBuffer(target);
}

static size_t index_type_size(GLenum type)
{
    switch (type) {
    case GL_UNSIGNED_BYTE:
        return 1;
    case GL_UNSIGNED_SHORT:
        return 2;
    case GL_UNSIGNED_INT:
        return 4;
    default:
        return 0;
    }
}

static GLuint rebase_indices(GLenum type, GLintptr offset, GLsizei count,
                             GLint basevertex, int *ok)
{
    *ok = 0;
    size_t elem = index_type_size(type);
    if (!elem || count <= 0) {
        return 0;
    }
    LOAD_REAL(glMapBufferRange);
    LOAD_REAL(glUnmapBuffer);
    LOAD_REAL(glGetBufferParameteriv);
    LOAD_REAL(glGenBuffers);
    LOAD_REAL(glBindBuffer);
    LOAD_REAL(glBufferData);
    if (!real_glMapBufferRange || !real_glUnmapBuffer || !real_glGetBufferParameteriv ||
        !real_glGenBuffers || !real_glBindBuffer || !real_glBufferData) {
        return 0;
    }
    GLint buffer_size = 0;
    real_glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &buffer_size);
    if (buffer_size <= 0 || offset < 0 || offset > (GLintptr)buffer_size ||
        (GLintptr)((size_t)count * elem) > (GLintptr)buffer_size - offset) {
        return 0;
    }
    void *mapped = real_glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, buffer_size,
                                         GL_MAP_READ_BIT);
    if (!mapped) {
        return 0;
    }
    unsigned char *copy = (unsigned char *)malloc((size_t)count * elem);
    if (!copy) {
        real_glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        return 0;
    }
    memcpy(copy, (const unsigned char *)mapped + offset, (size_t)count * elem);
    real_glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    for (GLsizei i = 0; i < count; i++) {
        if (elem == 1) {
            unsigned short v = (unsigned short)((unsigned char)copy[i] + basevertex);
            copy[i] = (unsigned char)(v > 255 ? 255 : v);
        } else if (elem == 2) {
            unsigned short v;
            memcpy(&v, copy + (size_t)i * 2, 2);
            v = (unsigned short)(v + basevertex);
            memcpy(copy + (size_t)i * 2, &v, 2);
        } else {
            uint32_t v;
            memcpy(&v, copy + (size_t)i * 4, 4);
            v += (uint32_t)basevertex;
            memcpy(copy + (size_t)i * 4, &v, 4);
        }
    }

    GLuint temp = 0;
    real_glGenBuffers(1, &temp);
    if (temp) {
        real_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, temp);
        real_glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)((size_t)count * elem),
                          copy, GL_STREAM_DRAW);
    }
    free(copy);
    *ok = temp != 0;
    return temp;
}

void glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type,
                              const void *indices, GLint basevertex)
{
    LOAD_REAL(glDrawElements);
    LOAD_REAL(glGetIntegerv);
    LOAD_REAL(glBindBuffer);
    LOAD_REAL(glDeleteBuffers);
    if (!real_glDrawElements) {
        return;
    }
    if (basevertex == 0 || !indices) {
        real_glDrawElements(mode, count, type, indices);
        return;
    }
    GLint previous = 0;
    if (real_glGetIntegerv) {
        real_glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &previous);
    }
    int ok = 0;
    GLuint temp = rebase_indices(type, (GLintptr)(uintptr_t)indices, count, basevertex, &ok);
    if (!ok) {
        TLOG(TITANGL_WARN, "base-vertex draw could not be rebased, drawing without the offset");
        real_glDrawElements(mode, count, type, indices);
        return;
    }
    real_glDrawElements(mode, count, type, NULL);
    if (real_glBindBuffer) {
        real_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)previous);
    }
    if (temp && real_glDeleteBuffers) {
        real_glDeleteBuffers(1, &temp);
    }
}

void glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count,
                                   GLenum type, const void *indices, GLint basevertex)
{
    (void)start;
    (void)end;
    glDrawElementsBaseVertex(mode, count, type, indices, basevertex);
}

void glMultiDrawArrays(GLenum mode, const GLint *first, const GLsizei *count,
                       GLsizei drawcount)
{
    LOAD_REAL(glDrawArrays);
    if (!real_glDrawArrays) {
        return;
    }
    for (GLsizei i = 0; i < drawcount; i++) {
        if (count[i] > 0) {
            real_glDrawArrays(mode, first[i], count[i]);
        }
    }
}

void glMultiDrawElements(GLenum mode, const GLsizei *count, GLenum type,
                         const void *const *indices, GLsizei drawcount)
{
    LOAD_REAL(glDrawElements);
    if (!real_glDrawElements) {
        return;
    }
    for (GLsizei i = 0; i < drawcount; i++) {
        if (count[i] > 0) {
            real_glDrawElements(mode, count[i], type, indices[i]);
        }
    }
}

void glMultiDrawElementsBaseVertex(GLenum mode, const GLsizei *count, GLenum type,
                                   const void *const *indices, GLsizei drawcount,
                                   const GLint *basevertex)
{
    for (GLsizei i = 0; i < drawcount; i++) {
        if (count[i] > 0) {
            glDrawElementsBaseVertex(mode, count[i], type, indices[i], basevertex[i]);
        }
    }
}

void glBindFragDataLocation(GLuint program, GLuint colorNumber, const GLchar *name)
{
    LOAD_REAL(glBindFragDataLocationEXT);
    if (real_glBindFragDataLocationEXT) {
        real_glBindFragDataLocationEXT(program, colorNumber, name);
    }
}

/* --------------------------------------------------------- pass-through */

FWD_VOID(glBlendFunc, (GLenum sfactor, GLenum dfactor), (sfactor, dfactor))
FWD_VOID(glBlendFuncSeparate,
         (GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha),
         (srcRGB, dstRGB, srcAlpha, dstAlpha))
FWD_VOID(glBlendEquation, (GLenum mode), (mode))
FWD_VOID(glBlendColor, (GLfloat r, GLfloat g, GLfloat b, GLfloat a), (r, g, b, a))
FWD_VOID(glDepthFunc, (GLenum func), (func))
FWD_VOID(glDepthMask, (GLboolean flag), (flag))
FWD_VOID(glColorMask, (GLboolean r, GLboolean g, GLboolean b, GLboolean a), (r, g, b, a))
FWD_VOID(glCullFace, (GLenum mode), (mode))
FWD_VOID(glFrontFace, (GLenum mode), (mode))
FWD_VOID(glPixelStorei, (GLenum pname, GLint param), (pname, param))
FWD_VOID(glPolygonOffset, (GLfloat factor, GLfloat units), (factor, units))
FWD_VOID(glClear, (GLbitfield mask), (mask))
FWD_VOID(glClearColor, (GLfloat r, GLfloat g, GLfloat b, GLfloat a), (r, g, b, a))
FWD_VOID(glClearStencil, (GLint s), (s))
FWD_VOID(glViewport, (GLint x, GLint y, GLsizei width, GLsizei height),
         (x, y, width, height))
FWD_VOID(glScissor, (GLint x, GLint y, GLsizei width, GLsizei height),
         (x, y, width, height))
FWD_VOID(glActiveTexture, (GLenum texture), (texture))
FWD_VOID(glBindTexture, (GLenum target, GLuint texture), (target, texture))
FWD_VOID(glGenTextures, (GLsizei n, GLuint *textures), (n, textures))
FWD_VOID(glDeleteTextures, (GLsizei n, const GLuint *textures), (n, textures))
FWD_VOID(glTexImage2D,
         (GLenum target, GLint level, GLint internalformat, GLsizei width,
          GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels),
         (target, level, internalformat, width, height, border, format, type, pixels))
FWD_VOID(glTexSubImage2D,
         (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
          GLsizei height, GLenum format, GLenum type, const void *pixels),
         (target, level, xoffset, yoffset, width, height, format, type, pixels))
FWD_VOID(glTexParameteri, (GLenum target, GLenum pname, GLint param),
         (target, pname, param))
FWD_VOID(glTexParameterf, (GLenum target, GLenum pname, GLfloat param),
         (target, pname, param))
FWD_VOID(glGenerateMipmap, (GLenum target), (target))
FWD_VOID(glUseProgram, (GLuint program), (program))
FWD_VOID(glUniform1i, (GLint location, GLint v0), (location, v0))
FWD_VOID(glUniform1f, (GLint location, GLfloat v0), (location, v0))
FWD_VOID(glUniform2f, (GLint location, GLfloat v0, GLfloat v1), (location, v0, v1))
FWD_VOID(glUniform3f, (GLint location, GLfloat v0, GLfloat v1, GLfloat v2),
         (location, v0, v1, v2))
FWD_VOID(glUniform4f, (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3),
         (location, v0, v1, v2, v3))
FWD_VOID(glUniform1iv, (GLint location, GLsizei count, const GLint *value),
         (location, count, value))
FWD_VOID(glUniform1fv, (GLint location, GLsizei count, const GLfloat *value),
         (location, count, value))
FWD_VOID(glUniform2fv, (GLint location, GLsizei count, const GLfloat *value),
         (location, count, value))
FWD_VOID(glUniform3fv, (GLint location, GLsizei count, const GLfloat *value),
         (location, count, value))
FWD_VOID(glUniform4fv, (GLint location, GLsizei count, const GLfloat *value),
         (location, count, value))
FWD_VOID(glUniformMatrix3fv,
         (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value),
         (location, count, transpose, value))
FWD_VOID(glUniformMatrix4fv,
         (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value),
         (location, count, transpose, value))
FWD_VOID(glEnableVertexAttribArray, (GLuint index), (index))
FWD_VOID(glDisableVertexAttribArray, (GLuint index), (index))
FWD_VOID(glVertexAttribPointer,
         (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
          const void *pointer),
         (index, size, type, normalized, stride, pointer))
FWD_VOID(glVertexAttribIPointer,
         (GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer),
         (index, size, type, stride, pointer))
FWD_VOID(glVertexAttribDivisor, (GLuint index, GLuint divisor), (index, divisor))
FWD_VOID(glGenVertexArrays, (GLsizei n, GLuint *arrays), (n, arrays))
FWD_VOID(glBindVertexArray, (GLuint array), (array))
FWD_VOID(glDeleteVertexArrays, (GLsizei n, const GLuint *arrays), (n, arrays))
FWD_VOID(glBindFramebuffer, (GLenum target, GLuint framebuffer), (target, framebuffer))
FWD_VOID(glGenFramebuffers, (GLsizei n, GLuint *framebuffers), (n, framebuffers))
FWD_VOID(glDeleteFramebuffers, (GLsizei n, const GLuint *framebuffers), (n, framebuffers))
FWD_VOID(glFramebufferTexture2D,
         (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level),
         (target, attachment, textarget, texture, level))
FWD_VOID(glFramebufferRenderbuffer,
         (GLenum target, GLenum attachment, GLenum renderbuffertarget,
          GLuint renderbuffer),
         (target, attachment, renderbuffertarget, renderbuffer))
FWD(GLenum, glCheckFramebufferStatus, (GLenum target), (target))
FWD_VOID(glGenRenderbuffers, (GLsizei n, GLuint *renderbuffers), (n, renderbuffers))
FWD_VOID(glDeleteRenderbuffers, (GLsizei n, const GLuint *renderbuffers), (n, renderbuffers))
FWD_VOID(glBindRenderbuffer, (GLenum target, GLuint renderbuffer), (target, renderbuffer))
FWD_VOID(glRenderbufferStorage, (GLenum target, GLenum internalformat, GLsizei width,
                                GLsizei height),
         (target, internalformat, width, height))
FWD_VOID(glReadPixels, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format,
                        GLenum type, void *pixels),
         (x, y, width, height, format, type, pixels))
FWD_VOID(glDrawBuffers, (GLsizei n, const GLenum *bufs), (n, bufs))
FWD_VOID(glFinish, (void), ())
FWD_VOID(glFlush, (void), ())
FWD_VOID(glStencilFunc, (GLenum func, GLint ref, GLuint mask), (func, ref, mask))
FWD_VOID(glStencilOp, (GLenum fail, GLenum zfail, GLenum zpass), (fail, zfail, zpass))
FWD_VOID(glStencilMask, (GLuint mask), (mask))
FWD_VOID(glTexStorage2D, (GLenum target, GLsizei levels, GLenum internalformat,
                          GLsizei width, GLsizei height),
         (target, levels, internalformat, width, height))
FWD_VOID(glInvalidateFramebuffer, (GLenum target, GLsizei numAttachments,
                                   const GLenum *attachments),
         (target, numAttachments, attachments))
FWD(GLsync, glFenceSync, (GLenum condition, GLbitfield flags), (condition, flags))
FWD(GLenum, glClientWaitSync, (GLsync sync, GLbitfield flags, GLuint64 timeout),
    (sync, flags, timeout))
FWD_VOID(glDeleteSync, (GLsync sync), (sync))
FWD_VOID(glWaitSync, (GLsync sync, GLbitfield flags, GLuint64 timeout),
         (sync, flags, timeout))
FWD_VOID(glGetSynciv, (GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length,
                       GLint *values),
         (sync, pname, bufSize, length, values))
FWD_VOID(glCopyBufferSubData, (GLenum readTarget, GLenum writeTarget,
                               GLintptr readOffset, GLintptr writeOffset,
                               GLsizeiptr size),
         (readTarget, writeTarget, readOffset, writeOffset, size))
FWD_VOID(glGetActiveUniform, (GLuint program, GLuint index, GLsizei bufSize,
                              GLsizei *length, GLint *size, GLenum *type, GLchar *name),
         (program, index, bufSize, length, size, type, name))
FWD_VOID(glGetActiveAttrib, (GLuint program, GLuint index, GLsizei bufSize,
                             GLsizei *length, GLint *size, GLenum *type, GLchar *name),
         (program, index, bufSize, length, size, type, name))
FWD(GLuint, glGetUniformBlockIndex, (GLuint program, const GLchar *uniformBlockName),
    (program, uniformBlockName))
FWD_VOID(glUniformBlockBinding, (GLuint program, GLuint uniformBlockIndex,
                                GLuint uniformBlockBinding),
         (program, uniformBlockIndex, uniformBlockBinding))
FWD_VOID(glGetActiveUniformBlockiv,
         (GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params),
         (program, uniformBlockIndex, pname, params))
FWD_VOID(glGetActiveUniformBlockName,
         (GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length,
          GLchar *uniformBlockName),
         (program, uniformBlockIndex, bufSize, length, uniformBlockName))

FWD(GLenum, glGetError, (void), ())
FWD(GLint, glGetAttribLocation, (GLuint program, const GLchar *name), (program, name))
FWD(GLint, glGetUniformLocation, (GLuint program, const GLchar *name), (program, name))
FWD_VOID(glGetShaderiv, (GLuint shader, GLenum pname, GLint *params),
         (shader, pname, params))
FWD_VOID(glGetShaderInfoLog, (GLuint shader, GLsizei bufSize, GLsizei *length,
                              GLchar *infoLog),
         (shader, bufSize, length, infoLog))
FWD_VOID(glGetProgramiv, (GLuint program, GLenum pname, GLint *params),
         (program, pname, params))
FWD_VOID(glGetProgramInfoLog, (GLuint program, GLsizei bufSize, GLsizei *length,
                               GLchar *infoLog),
         (program, bufSize, length, infoLog))
FWD_VOID(glBindAttribLocation, (GLuint program, GLuint index, const GLchar *name),
         (program, index, name))
FWD_VOID(glGetBooleanv, (GLenum pname, GLboolean *data), (pname, data))
FWD_VOID(glGetFloatv, (GLenum pname, GLfloat *params), (pname, params))
FWD(void *, glMapBufferRange,
    (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access),
    (target, offset, length, access))
FWD(GLboolean, glUnmapBuffer, (GLenum target), (target))
FWD_VOID(glFlushMappedBufferRange, (GLenum target, GLintptr offset, GLsizeiptr length),
         (target, offset, length))
FWD_VOID(glGetBufferParameteriv, (GLenum target, GLenum pname, GLint *params),
         (target, pname, params))
FWD_VOID(glBindBuffer, (GLenum target, GLuint buffer), (target, buffer))
FWD_VOID(glBufferData, (GLenum target, GLsizeiptr size, const void *data, GLenum usage),
         (target, size, data, usage))
FWD_VOID(glGenBuffers, (GLsizei n, GLuint *buffers), (n, buffers))
FWD_VOID(glDeleteBuffers, (GLsizei n, const GLuint *buffers), (n, buffers))
FWD_VOID(glDrawArrays, (GLenum mode, GLint first, GLsizei count), (mode, first, count))
FWD_VOID(glDrawElements, (GLenum mode, GLsizei count, GLenum type, const void *indices),
         (mode, count, type, indices))
FWD_VOID(glDrawArraysInstanced, (GLenum mode, GLint first, GLsizei count,
                                 GLsizei instancecount),
         (mode, first, count, instancecount))
FWD_VOID(glDrawElementsInstanced, (GLenum mode, GLsizei count, GLenum type,
                                   const void *indices, GLsizei instancecount),
         (mode, count, type, indices, instancecount))

/* --------------------------------------------------------- proc lookup */

typedef struct {
    const char *name;
    void *address;
} proc_entry;

#define PROC(name) { #name, (void *)name }

void *titangl_gl_lookup(const char *name)
{
    static const proc_entry table[] = {
        PROC(glCreateShader), PROC(glDeleteShader), PROC(glShaderSource),
        PROC(glCompileShader), PROC(glAttachShader), PROC(glDetachShader),
        PROC(glCreateProgram), PROC(glDeleteProgram), PROC(glLinkProgram),
        PROC(glGetShaderiv), PROC(glGetShaderInfoLog), PROC(glGetProgramiv),
        PROC(glGetProgramInfoLog), PROC(glBindAttribLocation),
        PROC(glGetAttribLocation), PROC(glGetUniformLocation),
        PROC(glGetString), PROC(glGetStringi), PROC(glGetIntegerv),
        PROC(glGetFloatv), PROC(glGetDoublev), PROC(glGetError),
        PROC(glEnable), PROC(glDisable), PROC(glClearDepth), PROC(glDepthRange),
        PROC(glPointSize), PROC(glPolygonMode), PROC(glDrawBuffer),
        PROC(glLineWidth), PROC(glMapBuffer), PROC(glGetBufferSubData),
        PROC(glDrawElementsBaseVertex), PROC(glDrawRangeElementsBaseVertex),
        PROC(glMultiDrawArrays), PROC(glMultiDrawElements),
        PROC(glMultiDrawElementsBaseVertex), PROC(glBindFragDataLocation),
        PROC(glBlendFunc), PROC(glBlendFuncSeparate), PROC(glBlendEquation),
        PROC(glBlendColor), PROC(glDepthFunc), PROC(glDepthMask),
        PROC(glColorMask), PROC(glCullFace), PROC(glFrontFace),
        PROC(glPixelStorei), PROC(glPolygonOffset), PROC(glClear),
        PROC(glClearColor), PROC(glClearStencil), PROC(glViewport),
        PROC(glScissor), PROC(glActiveTexture), PROC(glBindTexture),
        PROC(glGenTextures), PROC(glDeleteTextures), PROC(glTexImage2D),
        PROC(glTexSubImage2D), PROC(glTexParameteri), PROC(glTexParameterf),
        PROC(glGenerateMipmap), PROC(glUseProgram), PROC(glUniform1i),
        PROC(glUniform1f), PROC(glUniform2f), PROC(glUniform3f),
        PROC(glUniform4f), PROC(glUniform1iv), PROC(glUniform1fv),
        PROC(glUniform2fv), PROC(glUniform3fv), PROC(glUniform4fv),
        PROC(glUniformMatrix3fv), PROC(glUniformMatrix4fv),
        PROC(glEnableVertexAttribArray), PROC(glDisableVertexAttribArray),
        PROC(glVertexAttribPointer), PROC(glVertexAttribIPointer),
        PROC(glVertexAttribDivisor), PROC(glGenVertexArrays),
        PROC(glBindVertexArray), PROC(glDeleteVertexArrays),
        PROC(glBindFramebuffer), PROC(glGenFramebuffers),
        PROC(glDeleteFramebuffers), PROC(glFramebufferTexture2D),
        PROC(glFramebufferRenderbuffer), PROC(glCheckFramebufferStatus),
        PROC(glGenRenderbuffers), PROC(glDeleteRenderbuffers),
        PROC(glBindRenderbuffer), PROC(glRenderbufferStorage),
        PROC(glReadPixels), PROC(glDrawBuffers), PROC(glFinish), PROC(glFlush),
        PROC(glStencilFunc), PROC(glStencilOp), PROC(glStencilMask),
        PROC(glTexStorage2D), PROC(glInvalidateFramebuffer),
        PROC(glFenceSync), PROC(glClientWaitSync), PROC(glDeleteSync),
        PROC(glWaitSync), PROC(glGetSynciv), PROC(glCopyBufferSubData),
        PROC(glGetActiveUniform), PROC(glGetActiveAttrib),
        PROC(glGetUniformBlockIndex), PROC(glUniformBlockBinding),
        PROC(glGetActiveUniformBlockiv), PROC(glGetActiveUniformBlockName),
        PROC(glGetBooleanv), PROC(glMapBufferRange), PROC(glUnmapBuffer),
        PROC(glFlushMappedBufferRange), PROC(glGetBufferParameteriv),
        PROC(glBindBuffer), PROC(glBufferData), PROC(glGenBuffers),
        PROC(glDeleteBuffers), PROC(glDrawArrays), PROC(glDrawElements),
        PROC(glDrawArraysInstanced), PROC(glDrawElementsInstanced),
    };
    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (strcmp(table[i].name, name) == 0) {
            return table[i].address;
        }
    }
    return NULL;
}
