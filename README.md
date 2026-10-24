# TitanLD

TitanLD is an Android renderer plug-in for Minecraft: Java Edition launchers
(Fold Craft Launcher, Zalith Launcher and Pojav-based launchers).  Version
3.0.0 keeps **TitanGL**, TitanLD's own renderer core built from the C sources in
`src/native/`, and replaces the old hand-drawn manager with a Miuix materials
UI (Kotlin, Jetpack Compose).

TitanGL is not a wrapper around MobileGlues and does not contain a
`libmobileglues.so` file.  It creates the OpenGL ES context itself, exports an
EGL/GL entry-point table, translates desktop GLSL to GLSL ES 3.00, and
emulates the desktop calls that GLES does not have.

## What is in the APK

| ABI | Renderer library |
| --- | --- |
| `arm64-v8a` | `lib/arm64-v8a/libtitanld.so` |
| `armeabi-v7a` | `lib/armeabi-v7a/libtitanld.so` |
| `x86` | `lib/x86/libtitanld.so` |
| `x86_64` | `lib/x86_64/libtitanld.so` |

Each `libtitanld.so` is compiled from this repository for the launcher's
renderer slot.  On top of the core the APK adds:

- FCL / Zalith / Pojav renderer metadata pointing at `libtitanld.so`;
- `MANAGE_EXTERNAL_STORAGE` plus the legacy read/write permissions, with an
  in-app flow that opens the correct system screen;
- creation of `/sdcard/TitanLD/config.json` with TitanGL's settings;
- a Miuix 0.9.4 materials manager with a blurred aurora background, a bottom
  navigation bar (Status / Config / About), a configuration viewer and the
  licence;
- no `INTERNET` permission at all.

It is **not** a Minecraft launcher and it does not contain Minecraft.

## The renderer core

TitanGL lives in `src/native/`:

| File | Purpose |
| --- | --- |
| `egl_shim.c` | EGL 1.4/1.5 front-end.  Forces an OpenGL ES 3 context on the system driver, even when the launcher asks for desktop OpenGL, and answers `eglGetProcAddress` from TitanGL's table. |
| `gl_shim.c` | GL entry points: shader/program tracking, fake desktop version queries, desktop-only call emulation (`glClearDepth`, `glDrawElementsBaseVertex`, `glMapBuffer`, multi-draw, ...), and pass-through dispatch. |
| `glsl.c` | Token-aware GLSL translator: `#version` rewrite, `attribute`/`varying`, `texture2D` family, `gl_FragColor`/`gl_FragData`, explicit attribute locations, desktop `#extension` removal. |
| `config.c` | Reads `/sdcard/TitanLD/config.json` (falls back to `/sdcard/MG/config.json`) and built-in defaults. |
| `proc.c` | Resolves the real system GLES functions through `dlopen`/`dlsym`, so `libtitanld.so` has no hard dependency on `libGLESv3.so`. |

The design and its limits are described in `docs/RENDERER.md`.

## Honest limits

TitanGL 0.2.0 is a new renderer, not a drop-in replacement for years of
MobileGlues / gl4es work.  It is honest about what it does today:

- it translates the shader and state paths used by Minecraft 1.17+ (core
  profile, GLSL 1.20 through 4.60 input) and the launchers in front of it;
- fixed-function OpenGL 1.x built-ins (`gl_Vertex`, `gl_ModelViewMatrix`,
  `glBegin`, display lists) are **not** emulated;
- geometry/tesellation/compute shaders, SSBOs and some desktop-only texture
  formats are not mapped;
- a shader that cannot be translated is passed through unchanged, so the
  driver's compile error points at the real feature instead of producing wrong
  output silently;
- no performance claim is made.  The renderer has a per-shader translation
  step and the emulated base-vertex path rebases indices on the CPU.

The compatibility matrix that matters is the shader set of the game version
and mods you run.  If a shader fails, the TitanGL log names the shader stage
and the driver's compile/link log.

## Install

1. Download `TitanLD-3.0.0.apk` from the release page.
2. Install it (Android 8.0 or newer, OpenGL ES 3.0 device).
3. Open TitanLD and tap **Grant storage access**, then allow *All files
   access* (Android 11+) or storage (Android 8-10).
4. The app creates `/sdcard/TitanLD/config.json`.
5. In your launcher, open the renderer/plug-in list and select **TitanLD**.
   The launcher loads `libtitanld.so` from this APK.

The APK is signed with a build-local key, so a locally rebuilt APK has a
different signature.  Uninstall the previous build before installing a
rebuild.

## Configuration

```json
{
  "glVersion": "4.6",
  "maxGlslCacheSize": 64,
  "logLevel": 3,
  "forceGLESVersion": 0,
  "dropDesktopExtensions": true
}
```

- `glVersion` is the desktop version reported by `glGetString(GL_VERSION)`.
- `maxGlslCacheSize` is reserved for the on-disk shader cache (reserved, not
  yet used by 0.2.0).
- `logLevel` is 1 (error) to 4 (debug); the log tag is `TitanGL`.
- `forceGLESVersion` is 0 to let TitanGL choose, or 2/3 to force a context
  version.
- `dropDesktopExtensions` removes `GL_ARB_*`, `GL_NV_*`, `GL_AMD_*`,
  `GL_INTEL_*`, `GL_ATI_*` and `GL_KHR_*` extension directives during
  translation.

## Build

The build uses the Android SDK command line tools, the NDK and Gradle 9.8.1.
The first Gradle run downloads the Miuix and AndroidX artifacts from Maven
Central and Google's Maven repository.

```bash
export ANDROID_HOME=/path/to/android-sdk
export ANDROID_NDK_HOME=/path/to/android-ndk   # optional, auto-detected
./build.sh
# build/TitanLD-3.0.0.apk
```

`./build.sh --native-only` builds just the four renderer libraries and runs the
host translator tests. Requirements: JDK 17+, Android SDK build-tools (aapt2,
d8, zipalign, apksigner, dexdump), a platform with `android.jar`, the NDK
(clang, llvm-nm, llvm-readelf) and Gradle 9.8.1.

The build first runs the host-side translator tests (`tests/glsl_test.c`),
then compiles TitanGL for every ABI and verifies each library:

- the ELF machine matches the ABI;
- `eglGetProcAddress`, `eglChooseConfig`, `eglGetConfigAttrib`,
  `eglQueryString`, `eglBindAPI`, `eglQueryAPI`, `glXGetProcAddress`,
  `glXGetProcAddressARB`, `eglCreateContext`, `glCreateShader`,
  `glShaderSource`, `glGetString`, `glDrawElementsBaseVertex`,
  `glDrawElementsInstancedBaseVertex` and `glMapBuffer` are exported;
- there is no hard dependency on `libGLESv3.so`;
- the finished APK contains exactly four `libtitanld.so` entries, no
  `libmobileglues.so`, and no `INTERNET` permission.

The ABI matrix and its result are repeated in the release notes.

To reuse one signing identity across builds, point the build at an existing
keystore:

```bash
TITANLD_KEYSTORE=/path/to/release.jks \
TITANLD_KEYSTORE_PASSWORD=... \
TITANLD_KEY_ALIAS=... \
TITANLD_KEY_PASSWORD=... \
./build.sh
```

## Licence

TitanLD's own code, including TitanGL, is MIT (see `LICENSE`).  The repository
still contains the MobileGlues LGPL notice from release 1.0.0 for reference,
but no MobileGlues code or binary is used or packaged by this version.

TitanLD is an independent project.  It is not affiliated with or endorsed by
MobileGL-Dev, Mojang or Microsoft.
