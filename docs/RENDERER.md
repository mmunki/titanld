# TitanGL renderer notes

TitanGL is the renderer core shipped in `libtitanld.so`.  It is compiled from
`src/native/` for every Android ABI and loaded by the launcher through the
renderer metadata in `AndroidManifest.xml`.

## Why an EGL front-end is needed

Minecraft: Java Edition asks for a desktop OpenGL context.  Android exposes
OpenGL ES through EGL.  TitanGL sits between the two:

1. `eglCreateContext` throws away the requested desktop version/profile
   attributes, binds `EGL_OPENGL_ES_API` and creates an ES 3 context (retrying
   ES 2 only if ES 3 is unavailable).
2. `eglGetProcAddress` returns TitanGL's wrappers for the calls that need
   translation or emulation, and the system driver's function for everything
   else.
3. `glGetString` / `glGetIntegerv` report a desktop version (4.6 by default)
   so the game selects its core-profile path.

The real driver functions are resolved with `dlopen("libGLESv3.so")` /
`dlsym` and `eglGetProcAddress`, never by linking `libGLESv3.so`.  That keeps
the library loadable on devices where the vendor exposes a slightly different
GLES library layout.

## Shader translation

`glsl.c` is a token-aware rewriter, not a full GLSL compiler.  It:

- rewrites `#version 110/120/.../460 [core|compatibility]` to `#version 300 es`;
- inserts the precision statements GLSL ES requires;
- maps `attribute`/`varying` to `in`/`out` with the correct direction for the
  stage;
- maps the `texture2D` / `textureCube` / `shadow2D` families to `texture`,
  `textureProj` and `textureLod`;
- turns `gl_FragColor` and `gl_FragData[n]` into explicit output variables;
- records `layout(location = N)` on vertex inputs and applies the locations
  with `glBindAttribLocation` at link time;
- keeps `layout(std140)` on uniform blocks but removes the desktop-only
  `binding = N` part;
- drops desktop `#extension` directives that GLSL ES would reject.

Comments and string literals are copied verbatim, so a keyword in a comment is
not rewritten.  Unsupported constructs are left alone: the driver then emits a
normal compile error instead of TitanGL guessing.

The translator is pure C and is covered by `tests/glsl_test.c`, which runs on
the build host before any APK is produced.

## Emulated desktop calls

| Desktop call | TitanGL implementation |
| --- | --- |
| `glClearDepth` | `glClearDepthf` with a float conversion |
| `glDepthRange` | `glDepthRangef` with a float conversion |
| `glPointSize`, `glPolygonMode`, `glDrawBuffer` | accepted and ignored (no GLES equivalent) |
| `glLineWidth` | clamped to 1.0 on GLES |
| `glEnable`/`glDisable` of fixed-function caps | ignored (`GL_TEXTURE_2D`, `GL_FOG`, `GL_LIGHTING`, ...) |
| `glMapBuffer` | `glMapBufferRange` over the whole buffer |
| `glGetBufferSubData` | map/copy/unmap |
| `glDrawElementsBaseVertex` | rebases the index range into a temporary element buffer |
| `glMultiDrawArrays` / `glMultiDrawElements` | loops over the system draw call |
| `glGetDoublev` | `glGetFloatv`/`glGetIntegerv` plus conversion |
| `glBindFragDataLocation` | `glBindFragDataLocationEXT` when the driver exposes it |

## Not implemented

These are deliberately absent rather than silently wrong:

- fixed-function vertex arrays and matrix stacks;
- display lists, `glBegin`/`glEnd`, immediate mode;
- geometry, tessellation and compute shaders;
- shader storage buffer objects;
- desktop-only compressed texture formats and `GL_BGRA` uploads;
- multisample resolve beyond what GLES already provides.

A game or mod that depends on one of these will fail at the corresponding
call or shader compile.  The `TitanGL` log tag contains the shader stage and
the driver message.
