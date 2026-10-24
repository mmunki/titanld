/*
 * TitanGL EGL front-end.
 *
 * The launcher asks for a desktop OpenGL context through EGL.  There is no
 * desktop GL on Android, so TitanGL always creates an OpenGL ES 3 context on
 * the system driver and advertises a desktop version from glGetString.  The
 * GL entry points returned by eglGetProcAddress are TitanGL's own wrappers.
 */
#include "titangl.h"

#include <dlfcn.h>
#include <string.h>

#ifndef EGL_CONTEXT_MAJOR_VERSION
#define EGL_CONTEXT_MAJOR_VERSION 0x3098
#endif
#ifndef EGL_CONTEXT_MINOR_VERSION
#define EGL_CONTEXT_MINOR_VERSION 0x30FB
#endif
#ifndef EGL_CONTEXT_OPENGL_PROFILE_MASK
#define EGL_CONTEXT_OPENGL_PROFILE_MASK 0x30FD
#endif
#ifndef EGL_CONTEXT_OPENGL_DEBUG
#define EGL_CONTEXT_OPENGL_DEBUG 0x31B0
#endif
#ifndef EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE
#define EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE 0x31B1
#endif
#ifndef EGL_CONTEXT_OPENGL_ROBUST_ACCESS
#define EGL_CONTEXT_OPENGL_ROBUST_ACCESS 0x31B2
#endif
#ifndef EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY
#define EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY 0x31BD
#endif
#ifndef EGL_CONTEXT_FLAGS_KHR
#define EGL_CONTEXT_FLAGS_KHR 0x30FC
#endif
#ifndef EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR
#define EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR 0x30FD
#endif
#ifndef EGL_CONTEXT_OPENGL_NO_ERROR_KHR
#define EGL_CONTEXT_OPENGL_NO_ERROR_KHR 0x31B3
#endif
#ifndef EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT
#define EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT 0x30BF
#endif
#ifndef EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT
#define EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT 0x3138
#endif
#ifndef EGL_OPENGL_ES3_BIT
#define EGL_OPENGL_ES3_BIT 0x0040
#endif

static void *g_egl_handle;
static pthread_once_t g_egl_once = PTHREAD_ONCE_INIT;

#define DECL(name) static __typeof__(&name) real_##name

DECL(eglGetDisplay);
DECL(eglGetPlatformDisplay);
DECL(eglGetPlatformDisplayEXT);
DECL(eglInitialize);
DECL(eglTerminate);
DECL(eglQueryString);
DECL(eglGetConfigs);
DECL(eglChooseConfig);
DECL(eglGetConfigAttrib);
DECL(eglCreateWindowSurface);
DECL(eglCreatePlatformWindowSurface);
DECL(eglCreatePbufferSurface);
DECL(eglCreatePixmapSurface);
DECL(eglDestroySurface);
DECL(eglQuerySurface);
DECL(eglBindAPI);
DECL(eglQueryAPI);
DECL(eglWaitClient);
DECL(eglReleaseThread);
DECL(eglCreatePbufferFromClientBuffer);
DECL(eglSurfaceAttrib);
DECL(eglBindTexImage);
DECL(eglReleaseTexImage);
DECL(eglSwapInterval);
DECL(eglCreateContext);
DECL(eglDestroyContext);
DECL(eglMakeCurrent);
DECL(eglGetCurrentContext);
DECL(eglGetCurrentSurface);
DECL(eglGetCurrentDisplay);
DECL(eglQueryContext);
DECL(eglWaitGL);
DECL(eglWaitNative);
DECL(eglSwapBuffers);
DECL(eglCopyBuffers);
DECL(eglGetProcAddress);
DECL(eglGetError);
DECL(eglSwapBuffersWithDamageKHR);
DECL(eglSwapBuffersWithDamageEXT);
DECL(eglCreateImageKHR);
DECL(eglDestroyImageKHR);
DECL(eglCreateSync);
DECL(eglDestroySync);
DECL(eglClientWaitSync);
DECL(eglGetSyncAttrib);
DECL(eglWaitSync);

static void *load_symbol(const char *name)
{
    return g_egl_handle ? dlsym(g_egl_handle, name) : NULL;
}

static void init_egl(void)
{
    titangl_config_load();
    g_egl_handle = dlopen("libEGL.so", RTLD_NOW | RTLD_LOCAL);
    if (!g_egl_handle) {
        TLOG(TITANGL_ERROR, "cannot open the system EGL driver");
        return;
    }

#define LOAD(name) real_##name = (__typeof__(real_##name))load_symbol(#name)
    LOAD(eglGetDisplay);
    LOAD(eglGetPlatformDisplay);
    LOAD(eglGetPlatformDisplayEXT);
    LOAD(eglInitialize);
    LOAD(eglTerminate);
    LOAD(eglQueryString);
    LOAD(eglGetConfigs);
    LOAD(eglChooseConfig);
    LOAD(eglGetConfigAttrib);
    LOAD(eglCreateWindowSurface);
    LOAD(eglCreatePlatformWindowSurface);
    LOAD(eglCreatePbufferSurface);
    LOAD(eglCreatePixmapSurface);
    LOAD(eglDestroySurface);
    LOAD(eglQuerySurface);
    LOAD(eglBindAPI);
    LOAD(eglQueryAPI);
    LOAD(eglWaitClient);
    LOAD(eglReleaseThread);
    LOAD(eglCreatePbufferFromClientBuffer);
    LOAD(eglSurfaceAttrib);
    LOAD(eglBindTexImage);
    LOAD(eglReleaseTexImage);
    LOAD(eglSwapInterval);
    LOAD(eglCreateContext);
    LOAD(eglDestroyContext);
    LOAD(eglMakeCurrent);
    LOAD(eglGetCurrentContext);
    LOAD(eglGetCurrentSurface);
    LOAD(eglGetCurrentDisplay);
    LOAD(eglQueryContext);
    LOAD(eglWaitGL);
    LOAD(eglWaitNative);
    LOAD(eglSwapBuffers);
    LOAD(eglCopyBuffers);
    LOAD(eglGetProcAddress);
    LOAD(eglGetError);
    LOAD(eglSwapBuffersWithDamageKHR);
    LOAD(eglSwapBuffersWithDamageEXT);
    LOAD(eglCreateImageKHR);
    LOAD(eglDestroyImageKHR);
    LOAD(eglCreateSync);
    LOAD(eglDestroySync);
    LOAD(eglClientWaitSync);
    LOAD(eglGetSyncAttrib);
    LOAD(eglWaitSync);
#undef LOAD

    titangl_proc_install((void *(*)(const char *))real_eglGetProcAddress);
    TLOG(TITANGL_INFO, "TitanGL EGL front-end ready");
}

static void ensure_egl(void)
{
    pthread_once(&g_egl_once, init_egl);
}

#define EGL_FORWARD(ret, name, params, args)                                    \
    ret name params                                                            \
    {                                                                          \
        ensure_egl();                                                          \
        if (!real_##name) {                                                    \
            return (ret)0;                                                     \
        }                                                                      \
        return real_##name args;                                               \
    }

#define EGL_FORWARD_VOID(name, params, args)                                    \
    void name params                                                           \
    {                                                                          \
        ensure_egl();                                                          \
        if (real_##name) {                                                     \
            real_##name args;                                                  \
        }                                                                      \
    }

EGL_FORWARD(EGLDisplay, eglGetDisplay, (EGLNativeDisplayType display_id), (display_id))
EGL_FORWARD(EGLDisplay, eglGetPlatformDisplay,
            (EGLenum platform, void *native_display, const EGLAttrib *attrib_list),
            (platform, native_display, attrib_list))
EGL_FORWARD(EGLDisplay, eglGetPlatformDisplayEXT,
            (EGLenum platform, void *native_display, const EGLint *attrib_list),
            (platform, native_display, attrib_list))
EGL_FORWARD(EGLBoolean, eglInitialize,
            (EGLDisplay dpy, EGLint *major, EGLint *minor), (dpy, major, minor))
EGL_FORWARD(EGLBoolean, eglTerminate, (EGLDisplay dpy), (dpy))
EGL_FORWARD(EGLBoolean, eglGetConfigs,
            (EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config),
            (dpy, configs, config_size, num_config))
EGL_FORWARD(EGLSurface, eglCreateWindowSurface,
            (EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win,
             const EGLint *attrib_list),
            (dpy, config, win, attrib_list))
EGL_FORWARD(EGLSurface, eglCreatePlatformWindowSurface,
            (EGLDisplay dpy, EGLConfig config, void *native_window,
             const EGLAttrib *attrib_list),
            (dpy, config, native_window, attrib_list))
EGL_FORWARD(EGLSurface, eglCreatePbufferSurface,
            (EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list),
            (dpy, config, attrib_list))
EGL_FORWARD(EGLSurface, eglCreatePixmapSurface,
            (EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap,
             const EGLint *attrib_list),
            (dpy, config, pixmap, attrib_list))
EGL_FORWARD(EGLBoolean, eglDestroySurface, (EGLDisplay dpy, EGLSurface surface),
            (dpy, surface))
EGL_FORWARD(EGLBoolean, eglQuerySurface,
            (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value),
            (dpy, surface, attribute, value))
EGL_FORWARD(EGLBoolean, eglWaitClient, (void), ())
EGL_FORWARD(EGLBoolean, eglReleaseThread, (void), ())
EGL_FORWARD(EGLSurface, eglCreatePbufferFromClientBuffer,
            (EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer,
             EGLConfig config, const EGLint *attrib_list),
            (dpy, buftype, buffer, config, attrib_list))
EGL_FORWARD(EGLBoolean, eglSurfaceAttrib,
            (EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value),
            (dpy, surface, attribute, value))
EGL_FORWARD(EGLBoolean, eglBindTexImage,
            (EGLDisplay dpy, EGLSurface surface, EGLint buffer), (dpy, surface, buffer))
EGL_FORWARD(EGLBoolean, eglReleaseTexImage,
            (EGLDisplay dpy, EGLSurface surface, EGLint buffer), (dpy, surface, buffer))
EGL_FORWARD(EGLBoolean, eglSwapInterval, (EGLDisplay dpy, EGLint interval), (dpy, interval))
EGL_FORWARD(EGLBoolean, eglDestroyContext, (EGLDisplay dpy, EGLContext ctx), (dpy, ctx))
EGL_FORWARD(EGLBoolean, eglMakeCurrent,
            (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx),
            (dpy, draw, read, ctx))
EGL_FORWARD(EGLContext, eglGetCurrentContext, (void), ())
EGL_FORWARD(EGLSurface, eglGetCurrentSurface, (EGLint readdraw), (readdraw))
EGL_FORWARD(EGLDisplay, eglGetCurrentDisplay, (void), ())
EGL_FORWARD(EGLBoolean, eglQueryContext,
            (EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value),
            (dpy, ctx, attribute, value))
EGL_FORWARD(EGLBoolean, eglWaitGL, (void), ())
EGL_FORWARD(EGLBoolean, eglWaitNative, (EGLint engine), (engine))
EGL_FORWARD(EGLBoolean, eglSwapBuffers, (EGLDisplay dpy, EGLSurface surface),
            (dpy, surface))
EGL_FORWARD(EGLBoolean, eglCopyBuffers,
            (EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target),
            (dpy, surface, target))
EGL_FORWARD(EGLSync, eglCreateSync,
            (EGLDisplay dpy, EGLenum type, const EGLAttrib *attrib_list),
            (dpy, type, attrib_list))
EGL_FORWARD(EGLBoolean, eglDestroySync, (EGLDisplay dpy, EGLSync sync), (dpy, sync))
EGL_FORWARD(EGLint, eglClientWaitSync,
            (EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTime timeout),
            (dpy, sync, flags, timeout))
EGL_FORWARD(EGLBoolean, eglGetSyncAttrib,
            (EGLDisplay dpy, EGLSync sync, EGLint attribute, EGLAttrib *value),
            (dpy, sync, attribute, value))
EGL_FORWARD(EGLBoolean, eglWaitSync, (EGLDisplay dpy, EGLSync sync, EGLint flags),
            (dpy, sync, flags))
EGL_FORWARD(EGLint, eglGetError, (void), ())
EGL_FORWARD(EGLBoolean, eglSwapBuffersWithDamageKHR,
            (EGLDisplay dpy, EGLSurface surface, EGLint *rects, EGLint n_rects),
            (dpy, surface, rects, n_rects))
EGL_FORWARD(EGLBoolean, eglSwapBuffersWithDamageEXT,
            (EGLDisplay dpy, EGLSurface surface, EGLint *rects, EGLint n_rects),
            (dpy, surface, rects, n_rects))
EGL_FORWARD(EGLImageKHR, eglCreateImageKHR,
            (EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer,
             const EGLint *attrib_list),
            (dpy, ctx, target, buffer, attrib_list))
EGL_FORWARD(EGLBoolean, eglDestroyImageKHR, (EGLDisplay dpy, EGLImageKHR image),
            (dpy, image))

static __thread EGLenum g_frontend_api = EGL_OPENGL_ES_API;

EGLBoolean eglBindAPI(EGLenum api)
{
    ensure_egl();
    if (!real_eglBindAPI) {
        return EGL_FALSE;
    }
    EGLenum backend_api = (api == EGL_OPENGL_API) ? EGL_OPENGL_ES_API : api;
    EGLBoolean ok = real_eglBindAPI(backend_api);
    if (ok) {
        g_frontend_api = api;
    }
    return ok;
}

EGLenum eglQueryAPI(void)
{
    ensure_egl();
    return g_frontend_api;
}

static EGLint backend_renderable_mask(EGLint mask)
{
    if (mask & EGL_OPENGL_BIT) {
        mask &= ~EGL_OPENGL_BIT;
        mask |= EGL_OPENGL_ES3_BIT;
    }
    return mask;
}

static EGLint frontend_renderable_mask(EGLint mask)
{
    if (mask & EGL_OPENGL_ES3_BIT) {
        mask |= EGL_OPENGL_BIT;
    }
    return mask;
}

const char *eglQueryString(EGLDisplay dpy, EGLint name)
{
    ensure_egl();
    if (name == EGL_CLIENT_APIS) {
        return "OpenGL OpenGL_ES";
    }
    return real_eglQueryString ? real_eglQueryString(dpy, name) : NULL;
}

EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list,
                           EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    EGLint mapped[256];
    int n = 0;
    int has_renderable = 0;
    int wants_desktop = (g_frontend_api == EGL_OPENGL_API) ? 1 : 0;

    ensure_egl();
    if (!real_eglChooseConfig) {
        return EGL_FALSE;
    }
    if (attrib_list) {
        for (int i = 0; attrib_list[i] != EGL_NONE && n + 3 < (int)(sizeof(mapped) / sizeof(mapped[0])); i += 2) {
            EGLint key = attrib_list[i];
            EGLint value = attrib_list[i + 1];
            if (key == EGL_RENDERABLE_TYPE) {
                has_renderable = 1;
                wants_desktop |= (value & EGL_OPENGL_BIT) != 0;
                value = backend_renderable_mask(value);
            } else if (key == EGL_CONFORMANT) {
                wants_desktop |= (value & EGL_OPENGL_BIT) != 0;
                value = backend_renderable_mask(value);
            }
            mapped[n++] = key;
            mapped[n++] = value;
        }
    }
    if (wants_desktop && !has_renderable) {
        mapped[n++] = EGL_RENDERABLE_TYPE;
        mapped[n++] = EGL_OPENGL_ES3_BIT;
    }
    mapped[n++] = EGL_NONE;
    return real_eglChooseConfig(dpy, mapped, configs, config_size, num_config);
}

EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute,
                              EGLint *value)
{
    ensure_egl();
    if (!real_eglGetConfigAttrib) {
        return EGL_FALSE;
    }
    EGLBoolean ok = real_eglGetConfigAttrib(dpy, config, attribute, value);
    if (ok && value && (attribute == EGL_RENDERABLE_TYPE || attribute == EGL_CONFORMANT)) {
        *value = frontend_renderable_mask(*value);
    }
    return ok;
}

EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context,
                            const EGLint *attrib_list)
{
    EGLint attrs[64];
    int n = 0;
    ensure_egl();
    if (!real_eglCreateContext) {
        return EGL_NO_CONTEXT;
    }
    if (real_eglBindAPI) {
        real_eglBindAPI(EGL_OPENGL_ES_API);
    }
    if (attrib_list) {
        for (int i = 0; attrib_list[i] != EGL_NONE && n + 3 < (int)(sizeof(attrs) / sizeof(attrs[0])); i += 2) {
            EGLint key = attrib_list[i];
            EGLint value = attrib_list[i + 1];
            if (key == EGL_CONTEXT_MAJOR_VERSION ||
                key == EGL_CONTEXT_MINOR_VERSION ||
                key == EGL_CONTEXT_OPENGL_PROFILE_MASK ||
                key == EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR ||
                key == EGL_CONTEXT_OPENGL_DEBUG ||
                key == EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE ||
                key == EGL_CONTEXT_OPENGL_ROBUST_ACCESS ||
                key == EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT ||
                key == EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY ||
                key == EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT ||
                key == EGL_CONTEXT_FLAGS_KHR ||
                key == EGL_CONTEXT_OPENGL_NO_ERROR_KHR ||
                key == EGL_CONTEXT_CLIENT_VERSION) {
                continue;
            }
            attrs[n++] = key;
            attrs[n++] = value;
        }
    }
    int version_index = n;
    attrs[n++] = EGL_CONTEXT_CLIENT_VERSION;
    attrs[n++] = 3;
    attrs[n++] = EGL_NONE;

    EGLContext ctx = real_eglCreateContext(dpy, config, share_context, attrs);
    if (ctx == EGL_NO_CONTEXT) {
        attrs[version_index + 1] = 2;
        ctx = real_eglCreateContext(dpy, config, share_context, attrs);
    }
    if (ctx == EGL_NO_CONTEXT) {
        TLOG(TITANGL_ERROR, "could not create an OpenGL ES context");
    } else {
        TLOG(TITANGL_INFO, "created OpenGL ES context (desktop version advertised: %d.%d)",
             titangl_config_state.fake_major, titangl_config_state.fake_minor);
    }
    return ctx;
}

__eglMustCastToProperFunctionPointerType eglGetProcAddress(const char *procname)
{
    ensure_egl();
    if (!procname) {
        return NULL;
    }
    void *ours = titangl_egl_lookup(procname);
    if (ours) {
        return (__eglMustCastToProperFunctionPointerType)ours;
    }
    ours = titangl_gl_lookup(procname);
    if (ours) {
        return (__eglMustCastToProperFunctionPointerType)ours;
    }
    if (real_eglGetProcAddress) {
        __eglMustCastToProperFunctionPointerType p = real_eglGetProcAddress(procname);
        if (p) {
            return p;
        }
    }
    return (__eglMustCastToProperFunctionPointerType)titangl_get_proc(procname);
}


/*
 * glXGetProcAddress is what older LWJGL builds use instead of
 * eglGetProcAddress.  Both have to resolve to TitanGL's wrappers.
 */
__eglMustCastToProperFunctionPointerType glXGetProcAddress(const GLubyte *procName)
{
    return eglGetProcAddress((const char *)procName);
}

__eglMustCastToProperFunctionPointerType glXGetProcAddressARB(const GLubyte *procName)
{
    return eglGetProcAddress((const char *)procName);
}

void *titangl_egl_lookup(const char *name)
{
    static const struct {
        const char *name;
        void *address;
    } table[] = {
        {"eglGetDisplay", (void *)eglGetDisplay},
        {"eglGetPlatformDisplay", (void *)eglGetPlatformDisplay},
        {"eglGetPlatformDisplayEXT", (void *)eglGetPlatformDisplayEXT},
        {"eglInitialize", (void *)eglInitialize},
        {"eglTerminate", (void *)eglTerminate},
        {"eglQueryString", (void *)eglQueryString},
        {"eglGetConfigs", (void *)eglGetConfigs},
        {"eglChooseConfig", (void *)eglChooseConfig},
        {"eglGetConfigAttrib", (void *)eglGetConfigAttrib},
        {"eglCreateWindowSurface", (void *)eglCreateWindowSurface},
        {"eglCreatePlatformWindowSurface", (void *)eglCreatePlatformWindowSurface},
        {"eglCreatePbufferSurface", (void *)eglCreatePbufferSurface},
        {"eglCreatePixmapSurface", (void *)eglCreatePixmapSurface},
        {"eglDestroySurface", (void *)eglDestroySurface},
        {"eglQuerySurface", (void *)eglQuerySurface},
        {"eglBindAPI", (void *)eglBindAPI},
        {"eglQueryAPI", (void *)eglQueryAPI},
        {"eglWaitClient", (void *)eglWaitClient},
        {"eglReleaseThread", (void *)eglReleaseThread},
        {"eglCreatePbufferFromClientBuffer", (void *)eglCreatePbufferFromClientBuffer},
        {"eglSurfaceAttrib", (void *)eglSurfaceAttrib},
        {"eglBindTexImage", (void *)eglBindTexImage},
        {"eglReleaseTexImage", (void *)eglReleaseTexImage},
        {"eglSwapInterval", (void *)eglSwapInterval},
        {"eglCreateContext", (void *)eglCreateContext},
        {"eglDestroyContext", (void *)eglDestroyContext},
        {"eglMakeCurrent", (void *)eglMakeCurrent},
        {"eglGetCurrentContext", (void *)eglGetCurrentContext},
        {"eglGetCurrentSurface", (void *)eglGetCurrentSurface},
        {"eglGetCurrentDisplay", (void *)eglGetCurrentDisplay},
        {"eglQueryContext", (void *)eglQueryContext},
        {"eglWaitGL", (void *)eglWaitGL},
        {"eglWaitNative", (void *)eglWaitNative},
        {"eglSwapBuffers", (void *)eglSwapBuffers},
        {"eglSwapBuffersWithDamageKHR", (void *)eglSwapBuffersWithDamageKHR},
        {"eglSwapBuffersWithDamageEXT", (void *)eglSwapBuffersWithDamageEXT},
        {"eglCopyBuffers", (void *)eglCopyBuffers},
        {"eglGetProcAddress", (void *)eglGetProcAddress},
        {"eglGetError", (void *)eglGetError},
        {"eglCreateSync", (void *)eglCreateSync},
        {"eglDestroySync", (void *)eglDestroySync},
        {"eglClientWaitSync", (void *)eglClientWaitSync},
        {"eglGetSyncAttrib", (void *)eglGetSyncAttrib},
        {"eglWaitSync", (void *)eglWaitSync},
        {"eglCreateImageKHR", (void *)eglCreateImageKHR},
        {"eglDestroyImageKHR", (void *)eglDestroyImageKHR},
        {"glXGetProcAddress", (void *)glXGetProcAddress},
        {"glXGetProcAddressARB", (void *)glXGetProcAddressARB},
    };
    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (strcmp(table[i].name, name) == 0) {
            return table[i].address;
        }
    }
    return NULL;
}
