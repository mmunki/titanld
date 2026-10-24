/*
 * TitanGL -- TitanLD's own GL-to-GLES renderer core.
 */
#ifndef TITANGL_H
#define TITANGL_H

#ifndef EGL_EGLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES 1
#endif
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <android/log.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#define TITANGL_LOG_TAG "TitanGL"

typedef struct {
    int fake_major;
    int fake_minor;
    int max_glsl_cache_size;
    int log_level;
    int force_es_version;
    int drop_desktop_extensions;
    char config_path[512];
} titangl_config;

extern titangl_config titangl_config_state;

void titangl_config_load(void);

void titangl_log(int level, const char *fmt, ...);

#define TITANGL_ERROR 1
#define TITANGL_WARN 2
#define TITANGL_INFO 3
#define TITANGL_DEBUG 4

#define TLOG(level, ...) titangl_log((level), __VA_ARGS__)

void *titangl_get_proc(const char *name);
void titangl_proc_install(void *(*get_proc)(const char *));

/* gl_shim.c: our own entry points, used by eglGetProcAddress. */
void *titangl_gl_lookup(const char *name);

/* egl_shim.c */
void titangl_egl_install(void);

#endif /* TITANGL_H */
