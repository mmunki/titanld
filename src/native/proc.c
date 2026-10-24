#include "titangl.h"

#include <dlfcn.h>

static void *(*g_system_get_proc)(const char *);
static void *g_gles_handle;
static pthread_mutex_t g_proc_lock = PTHREAD_MUTEX_INITIALIZER;

void titangl_proc_install(void *(*get_proc)(const char *))
{
    g_system_get_proc = get_proc;
}

void *titangl_get_proc(const char *name)
{
    void *p = NULL;
    pthread_mutex_lock(&g_proc_lock);
    if (!g_gles_handle) {
        /*
         * libtitanld.so is the renderer itself, so libGLESv3.so is always the
         * system driver.  RTLD_LOCAL keeps our own exported gl* symbols out
         * of the lookup.  A GLES2-only device still gets a loadable library,
         * even though the shader path needs ES 3.
         */
        g_gles_handle = dlopen("libGLESv3.so", RTLD_NOW | RTLD_LOCAL);
        if (!g_gles_handle) {
            g_gles_handle = dlopen("libGLESv2.so", RTLD_NOW | RTLD_LOCAL);
        }
    }
    if (g_gles_handle) {
        p = dlsym(g_gles_handle, name);
    }
    void *(*system_get_proc)(const char *) = g_system_get_proc;
    pthread_mutex_unlock(&g_proc_lock);
    if (!p && system_get_proc) {
        p = system_get_proc(name);
    }
    return p;
}
