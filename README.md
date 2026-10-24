# TitanLD

TitanLD is an Android renderer plug-in for Minecraft: Java Edition launchers
(Fold Craft Launcher, Zalith Launcher and Pojav-based launchers). It ships a
GL-to-GLES renderer core, exposes the storage access the renderer needs, and
writes a tuned `/sdcard/MG/config.json`.

## What it actually is

TitanLD **packages the unmodified MobileGlues 2.0.0 renderer core**
(`libmobileglues.so`, LGPL-2.1) for all four Android ABIs:

| ABI | Renderer library |
| --- | --- |
| `arm64-v8a` | `lib/arm64-v8a/libmobileglues.so` |
| `armeabi-v7a` | `lib/armeabi-v7a/libmobileglues.so` |
| `x86` | `lib/x86/libmobileglues.so` |
| `x86_64` | `lib/x86_64/libmobileglues.so` |

On top of the core it adds:

- the FCL / Zalith / Pojav renderer metadata, so the launcher lists
  **TitanLD** as a renderer next to the other plug-ins;
- `MANAGE_EXTERNAL_STORAGE` plus the legacy read/write permissions, with an
  in-app flow that opens the correct system screen;
- automatic creation of `/sdcard/MG` and a conservative, performance-oriented
  `config.json` (64 MiB GLSL cache, ANGLE when the launcher supplies it,
  default MultiDraw order, no experimental image-corrupting options);
- a small status screen and a viewer for the active configuration;
- no `INTERNET` permission at all.

It is **not** a Minecraft launcher and it does not contain Minecraft.

## Honest limits

The request behind this project asked for a renderer that is faster than
MobileGlues and has no bugs. That cannot be promised, and this project does not
claim it:

- the rendering code is MobileGlues 2.0.0, byte-for-byte; TitanLD changes the
  packaging, the permissions and the default configuration, not the renderer;
- "faster than MobileGlues" is device-, driver- and shader-dependent; the only
  measurable difference here is the larger GLSL cache and the default ANGLE
  preference. No benchmark result is claimed;
- no complex renderer can be honestly described as bug-free. The upstream
  compatibility matrices are the reference:
  <https://github.com/MobileGL-Dev/MobileGlues-release>.

If you need a different profile, edit `/sdcard/MG/config.json` from the app or
with any file manager. The keys are the ones MobileGlues documents in its own
settings UI.

## Install

1. Download `TitanLD-1.0.0.apk` from the release page.
2. Install it (Android 8.0 or newer).
3. Open TitanLD and tap **Grant storage access**, then allow *All files
   access* (Android 11+) or storage (Android 8-10).
4. The app creates `/sdcard/MG/config.json` with the recommended profile.
5. In your launcher, open the renderer/plug-in list and select **TitanLD**.
   The launcher launches Minecraft with `libmobileglues.so` from this APK.

The APK is signed with a build-local key, so a locally rebuilt APK has a
different signature. Uninstall the previous build before installing a rebuild.

## Configuration written by the app

```json
{
  "enableANGLE": 1,
  "enableNoError": 0,
  "enableExtComputeShader": 0,
  "enableExtTimerQuery": 1,
  "enableExtDirectStateAccess": 0,
  "maxGlslCacheSize": 64,
  "angleDepthClearFixMode": 0,
  "customGLVersion": 0,
  "fsr1Setting": 0,
  "hideMGEnvLevel": 0,
  "multidrawOrder": "native,multiindirect,multibasevertex,multiarrays,indirect,basevertex,unroll,compute"
}
```

`enableANGLE: 1` means "enable if the device and the launcher provide ANGLE".
ANGLE is not bundled in this APK; the launcher supplies it. Experimental
switches (`enableExtDirectStateAccess`, `fsr1Setting`, a custom GL version) are
left off because they are the ones that produce rendering artefacts.

## Build

The build uses only the Android SDK command-line tools; Gradle is not needed.

```bash
export ANDROID_HOME=/path/to/android-sdk
./build.sh
# build/TitanLD-1.0.0.apk
```

Requirements: JDK 17+, Android SDK build-tools (aapt2, d8, zipalign,
apksigner) and a platform with `android.jar`. The renderer core is downloaded
from the pinned upstream release and verified against
`a7e1eb29731fdece7ab3af7fb00692b856f1808a7544436e7b3eb6c08355581a`.

The build validates every ABI by reading the ELF header of each packaged
library, then checks package name, label, dex, signature and the four
`native-code` entries of the finished APK. See
`.github/workflows/build.yml` for the CI version.

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

TitanLD's own code is MIT (see `LICENSE`). The renderer core is
**MobileGlues 2.0.0**, Copyright MobileGL-Dev, licensed under the
**GNU LGPL-2.1**. Its licence text is in
`licenses/MobileGlues-LGPL-2.1.txt` and in the APK assets. The upstream source
is at <https://github.com/MobileGL-Dev/MobileGlues>; the full source of this
wrapper is in every release source archive.

TitanLD is an independent packaging project. It is not affiliated with or
endorsed by MobileGL-Dev.
