#!/usr/bin/env bash
# ============================================================================
# build.sh -- build TitanLD.
#
#   build.sh                native core + Gradle release APK + verification
#   build.sh --native-only  only the four libtitanld.so files and host tests
#
# Inputs (environment, all optional):
#   ANDROID_HOME / ANDROID_SDK_ROOT      the Android SDK location
#   ANDROID_NDK_HOME / ANDROID_NDK_ROOT  the NDK location
#   TITANLD_KEYSTORE                     an existing signing keystore
#   TITANLD_KEYSTORE_PASSWORD            its store password
#   TITANLD_KEY_ALIAS                    the key alias
#   TITANLD_KEY_PASSWORD                 the key password
#   GRADLE_BIN                           the gradle executable (default: gradle)
#
# Output: build/TitanLD-<version>.apk
#
# TitanGL, the renderer core in src/native, is built for every ABI into
# build/native.  The Gradle app packages those libraries; no build output is
# ever committed and no renderer library is downloaded.
# ============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

VERSION_NAME="3.0.0"
VERSION_CODE="4"
ABIS=(arm64-v8a armeabi-v7a x86 x86_64)
MIN_SDK=26
TARGET_SDK=37

NATIVE_ONLY=0
for arg in "$@"; do
  case "$arg" in
    --native-only) NATIVE_ONLY=1 ;;
    -h|--help)
      sed -n '2,21p' "$0" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *) printf 'error: unknown argument: %s\n' "$arg" >&2; exit 2 ;;
  esac
done

log()  { printf '==> %s\n' "$*" >&2; }
die()  { printf 'error: %s\n' "$*" >&2; exit 1; }

# --------------------------------------------------------------- Android SDK
SDK="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"
[ -n "$SDK" ] || die "set ANDROID_HOME or ANDROID_SDK_ROOT to the Android SDK"
[ -d "$SDK" ] || die "Android SDK not found: $SDK"

pick_latest() { # pick_latest <dir> <glob-prefix> [required-file]
  local dir="$1" prefix="$2" required="${3:-}"
  local candidate
  candidate="$(
    find "$dir" -maxdepth 1 -mindepth 1 -type d -name "${prefix}*" 2>/dev/null \
      | grep -viE 'beta|preview|rc[0-9]|canary|dev' \
      | sort -V | while read -r d; do
          if [ -z "$required" ] || [ -e "$d/$required" ]; then printf '%s\n' "$d"; fi
        done | tail -n 1
  )"
  if [ -z "$candidate" ]; then
    candidate="$(
      find "$dir" -maxdepth 1 -mindepth 1 -type d -name "${prefix}*" 2>/dev/null \
        | sort -V | while read -r d; do
            if [ -z "$required" ] || [ -e "$d/$required" ]; then printf '%s\n' "$d"; fi
          done | tail -n 1
    )"
  fi
  [ -n "$candidate" ] || die "no ${prefix}* under $dir${required:+ containing $required}"
  printf '%s\n' "$candidate"
}

BUILD_TOOLS="$(pick_latest "$SDK/build-tools" "" aapt2)"
PLATFORM="$(pick_latest "$SDK/platforms" "android-" android.jar)"
for tool in aapt2 d8 apksigner zipalign dexdump; do
  [ -x "$BUILD_TOOLS/$tool" ] || die "missing $BUILD_TOOLS/$tool"
done
ANDROID_JAR="$PLATFORM/android.jar"

# ----------------------------------------------------------------------- NDK
NDK="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}"
if [ -z "$NDK" ] && [ -d "$SDK/ndk" ]; then
  NDK="$(pick_latest "$SDK/ndk" "" source.properties)"
fi
[ -n "$NDK" ] || die "set ANDROID_NDK_HOME or install an NDK under $SDK/ndk"
[ -d "$NDK" ] || die "Android NDK not found: $NDK"

case "$(uname -s)-$(uname -m)" in
  Linux-x86_64)  HOST_TAG=linux-x86_64 ;;
  Linux-aarch64) HOST_TAG=linux-aarch64 ;;
  Darwin-x86_64) HOST_TAG=darwin-x86_64 ;;
  Darwin-arm64)  HOST_TAG=darwin-x86_64 ;;
  *) die "unsupported build host: $(uname -s)-$(uname -m)" ;;
esac

TC="$NDK/toolchains/llvm/prebuilt/$HOST_TAG/bin"
LLVM_NM="$TC/llvm-nm"
LLVM_READELF="$TC/llvm-readelf"
[ -x "$LLVM_NM" ] || die "missing $LLVM_NM"
[ -x "$LLVM_READELF" ] || die "missing $LLVM_READELF"

declare -A CC=(
  [arm64-v8a]="$TC/aarch64-linux-android${MIN_SDK}-clang"
  [armeabi-v7a]="$TC/armv7a-linux-androideabi${MIN_SDK}-clang"
  [x86]="$TC/i686-linux-android${MIN_SDK}-clang"
  [x86_64]="$TC/x86_64-linux-android${MIN_SDK}-clang"
)
for abi in "${ABIS[@]}"; do
  [ -x "${CC[$abi]}" ] || die "missing compiler for $abi: ${CC[$abi]}"
done

log "SDK:          $SDK"
log "build-tools:  $(basename "$BUILD_TOOLS")"
log "platform:     $(basename "$PLATFORM")"
log "NDK:          $NDK"

command -v unzip     >/dev/null || die "unzip is required"
command -v zip       >/dev/null || die "zip is required"
command -v sha256sum >/dev/null || die "sha256sum is required"
command -v file      >/dev/null || die "file is required"

WORK="$ROOT/build"
NATIVE="$WORK/native"

# ------------------------------------------------------- host translator test
HOST_CC="${HOST_CC:-cc}"
log "running the host shader-translator tests"
mkdir -p "$WORK/test"
"$HOST_CC" -std=c11 -O2 -Wall -Wextra \
  -o "$WORK/test/glsl_test" tests/glsl_test.c src/native/glsl.c
"$WORK/test/glsl_test"

# ------------------------------------------------------------- TitanGL core
rm -rf "$NATIVE" "$WORK/obj"
mkdir -p "$NATIVE"

build_abi() {
  local abi="$1"
  local obj="$WORK/obj/$abi"
  local cc="${CC[$abi]}"
  local src
  mkdir -p "$obj" "$NATIVE/$abi"
  for src in src/native/*.c; do
    "$cc" -std=c11 -O2 -fPIC -Wall -Wextra -Wno-unused-parameter \
      -Isrc/native -DANDROID \
      -c "$src" -o "$obj/$(basename "${src%.c}").o"
  done
  "$cc" -shared -o "$NATIVE/$abi/libtitanld.so" "$obj"/*.o \
    -Wl,-soname,libtitanld.so -Wl,--no-undefined \
    -Wl,-z,relro,-z,now \
    -llog -lEGL -landroid -ldl -lm
}

declare -A EXPECT_MACHINE=(
  [arm64-v8a]='AArch64'
  [armeabi-v7a]='ARM'
  [x86]='Intel 80386'
  [x86_64]='Advanced Micro Devices X86-64'
)

for abi in "${ABIS[@]}"; do
  log "building TitanGL for $abi"
  build_abi "$abi"
  lib="$NATIVE/$abi/libtitanld.so"
  [ -s "$lib" ] || die "TitanGL did not produce $lib"
  machine="$("$LLVM_READELF" -h "$lib" | awk '/Machine:/{print substr($0, index($0,$2))}')"
  case "$machine" in
    *"${EXPECT_MACHINE[$abi]}"*) ;;
    *) die "native/$abi has the wrong ELF machine: $machine" ;;
  esac
  nm_out="$("$LLVM_NM" -D --defined-only "$lib")"
  for sym in eglGetProcAddress eglChooseConfig eglGetConfigAttrib eglQueryString \
             eglBindAPI eglQueryAPI glXGetProcAddress glXGetProcAddressARB \
             eglCreateContext glCreateShader glShaderSource glGetString \
             glDrawElementsBaseVertex glDrawElementsInstancedBaseVertex glMapBuffer; do
    grep -q " $sym\$" <<<"$nm_out" \
      || die "native/$abi is missing the exported symbol $sym"
  done
  dyn_out="$("$LLVM_READELF" -d "$lib")"
  if grep -q 'libGLESv3\.so' <<<"$dyn_out"; then
    die "native/$abi must not hard-link libGLESv3.so; it dispatches through dlopen"
  fi
  log "ABI $abi: $machine, $(stat -c%s "$lib") bytes, symbols OK"
done

if [ "$NATIVE_ONLY" = 1 ]; then
  log "native core ready in $NATIVE"
  exit 0
fi

# ------------------------------------------------------------------- Gradle
command -v javac   >/dev/null || die "javac (JDK 17+) is required"
command -v keytool >/dev/null || die "keytool (JDK 17+) is required"
GRADLE="${GRADLE_BIN:-gradle}"
command -v "$GRADLE" >/dev/null || die "gradle is required (set GRADLE_BIN)"

KEYSTORE="${TITANLD_KEYSTORE:-$WORK/keys/titanld-release.jks}"
KS_PASS="${TITANLD_KEYSTORE_PASSWORD:-titanld-build}"
KEY_ALIAS="${TITANLD_KEY_ALIAS:-titanld}"
KEY_PASS="${TITANLD_KEY_PASSWORD:-titanld-build}"
if [ ! -f "$KEYSTORE" ]; then
  log "generating a build signing key (set TITANLD_KEYSTORE to reuse one)"
  mkdir -p "$(dirname "$KEYSTORE")"
  keytool -genkeypair -noprompt \
    -keystore "$KEYSTORE" -storetype PKCS12 \
    -storepass "$KS_PASS" -keypass "$KEY_PASS" \
    -alias "$KEY_ALIAS" -keyalg RSA -keysize 2048 -validity 10000 \
    -dname "CN=TitanLD, OU=TitanLD, O=TitanLD, L=Unknown, ST=Unknown, C=US"
fi

export TITANLD_KEYSTORE="$KEYSTORE"
export TITANLD_KEYSTORE_PASSWORD="$KS_PASS"
export TITANLD_KEY_ALIAS="$KEY_ALIAS"
export TITANLD_KEY_PASSWORD="$KEY_PASS"

log "building the Miuix manager and packaging the release APK"
"$GRADLE" --no-daemon :app:assembleRelease

GRADLE_APK="$ROOT/app/build/outputs/apk/release/app-release.apk"
[ -f "$GRADLE_APK" ] || die "Gradle did not produce $GRADLE_APK"
OUT="$ROOT/build/TitanLD-${VERSION_NAME}.apk"
cp -f "$GRADLE_APK" "$OUT"
log "signed: $OUT"

# ---------------------------------------------------------------- verification
log "package contents:"
unzip -l "$OUT" | grep -E 'classes.dex|resources.arsc|lib/.+/libtitanld.so|assets/' >&2 || true

zip_listing="$(unzip -l "$OUT")"
for abi in "${ABIS[@]}"; do
  grep -q "lib/$abi/libtitanld.so" <<<"$zip_listing" \
    || die "built APK is missing lib/$abi/libtitanld.so"
  if grep -q "lib/$abi/libmobileglues.so" <<<"$zip_listing"; then
    die "built APK must not contain MobileGlues"
  fi
done

BADGING="$("$BUILD_TOOLS/aapt2" dump badging "$OUT")"
grep -q "package: name='io.github.mmunki.titanld'" <<<"$BADGING" \
  || die "unexpected package name"
grep -q "application-label:'TitanLD'" <<<"$BADGING" \
  || die "unexpected application label"
grep -q "versionName='${VERSION_NAME}'" <<<"$BADGING" \
  || die "unexpected version name"
grep -q "versionCode='${VERSION_CODE}'" <<<"$BADGING" \
  || die "unexpected version code"
grep -q "targetSdkVersion:'${TARGET_SDK}'" <<<"$BADGING" \
  || die "unexpected target SDK"
for abi in "${ABIS[@]}"; do
  grep -q "'$abi'" <<<"$BADGING" || die "badging does not list ABI $abi"
done

"$BUILD_TOOLS/dexdump" "$OUT" > "$WORK/dexdump.txt" 2>/dev/null
grep -q 'Lio/github/mmunki/titanld/MainActivity;' "$WORK/dexdump.txt" \
  || die "MainActivity is missing from classes.dex"
grep -q 'Lio/github/mmunki/titanld/TitanStorage;' "$WORK/dexdump.txt" \
  || die "TitanStorage is missing from classes.dex"

unzip -p "$OUT" "assets/LICENSE.txt" >/dev/null \
  || die "the MIT licence is missing from the APK"
unzip -p "$OUT" "assets/THIRD_PARTY_NOTICES.md" >/dev/null \
  || die "third-party notices are missing from the APK"

"$BUILD_TOOLS/apksigner" verify --verbose --print-certs "$OUT" >&2
log "APK verified: package, version, label, dex, signature, assets and all four ABIs"
printf '\n%s\n' "$OUT"
