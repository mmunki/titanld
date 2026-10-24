#!/usr/bin/env bash
# ============================================================================
# build.sh -- build TitanLD's release APK with the Android SDK command line
# tools and the NDK.  No Gradle, no network access.
#
# Inputs (environment, all optional):
#   ANDROID_HOME / ANDROID_SDK_ROOT   the Android SDK location
#   ANDROID_NDK_HOME / ANDROID_NDK_ROOT  the NDK location (auto-detected under
#                                     $ANDROID_HOME/ndk when not set)
#   TITANLD_KEYSTORE                  an existing signing keystore
#   TITANLD_KEYSTORE_PASSWORD         its store password
#   TITANLD_KEY_ALIAS                 the key alias
#   TITANLD_KEY_PASSWORD              the key password
#
# Output: build/TitanLD-<version>.apk
#
# The renderer is TitanGL, TitanLD's own C core in src/native.  MobileGlues is
# no longer downloaded or packaged; the APK contains only libtitanld.so built
# from this tree.
# ============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

VERSION_NAME="2.0.1"
VERSION_CODE="3"
ABIS=(arm64-v8a armeabi-v7a x86 x86_64)
MIN_SDK=26
TARGET_SDK=34

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

command -v javac   >/dev/null || die "javac (JDK 17+) is required"
command -v keytool >/dev/null || die "keytool (JDK 17+) is required"
command -v unzip   >/dev/null || die "unzip is required"
command -v zip     >/dev/null || die "zip is required"
command -v sha256sum >/dev/null || die "sha256sum is required"
command -v file    >/dev/null || die "file is required"

WORK="$ROOT/build"
rm -rf "$WORK"
mkdir -p "$WORK/lib" "$WORK/gen" "$WORK/classes" "$WORK/dex" "$WORK/assets" "$WORK/test"

# ------------------------------------------------------- host translator test
HOST_CC="${HOST_CC:-cc}"
log "running the host shader-translator tests"
"$HOST_CC" -std=c11 -O2 -Wall -Wextra \
  -o "$WORK/test/glsl_test" tests/glsl_test.c src/native/glsl.c
"$WORK/test/glsl_test"

# ------------------------------------------------------------- TitanGL core
build_abi() {
  local abi="$1"
  local obj="$WORK/obj/$abi"
  local cc="${CC[$abi]}"
  local src
  mkdir -p "$obj" "$WORK/lib/$abi"
  for src in src/native/*.c; do
    "$cc" -std=c11 -O2 -fPIC -Wall -Wextra -Wno-unused-parameter \
      -Isrc/native -DANDROID \
      -c "$src" -o "$obj/$(basename "${src%.c}").o"
  done
  "$cc" -shared -o "$WORK/lib/$abi/libtitanld.so" "$obj"/*.o \
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
  lib="$WORK/lib/$abi/libtitanld.so"
  [ -s "$lib" ] || die "TitanGL did not produce $lib"
  machine="$("$LLVM_READELF" -h "$lib" | awk '/Machine:/{print substr($0, index($0,$2))}')"
  case "$machine" in
    *"${EXPECT_MACHINE[$abi]}"*) ;;
    *) die "lib/$abi has the wrong ELF machine: $machine" ;;
  esac
  # Capture the tool output before grepping.  A `tool | grep -q` pipeline
  # under `set -o pipefail` is racy: grep exits at the first match, the tool
  # gets SIGPIPE and the pipeline reports failure even though the symbol was
  # found.
  nm_out="$("$LLVM_NM" -D --defined-only "$lib")"
  for sym in eglGetProcAddress eglCreateContext glCreateShader glShaderSource \
             glGetString glDrawElementsBaseVertex glMapBuffer; do
    grep -q " $sym\$" <<<"$nm_out" \
      || die "lib/$abi is missing the exported symbol $sym"
  done
  dyn_out="$("$LLVM_READELF" -d "$lib")"
  if grep -q 'libGLESv3\.so' <<<"$dyn_out"; then
    die "lib/$abi must not hard-link libGLESv3.so; it dispatches through dlopen"
  fi
  log "ABI $abi: $machine, $(stat -c%s "$lib") bytes, symbols OK"
done

# ----------------------------------------------------------------- resources
cp -R "$ROOT/assets/." "$WORK/assets/" 2>/dev/null || true
# The MobileGlues licence belongs to the 1.0.0 release; it is kept in the
# source tree but is not part of the custom-renderer APK.
rm -f "$WORK/assets/licenses/MobileGlues-LGPL-2.1.txt"
cp "$ROOT/LICENSE" "$WORK/assets/LICENSE.txt"
"$BUILD_TOOLS/aapt2" compile --dir "$ROOT/res" -o "$WORK/res.zip"
"$BUILD_TOOLS/aapt2" link \
  -o "$WORK/base.apk" \
  -I "$ANDROID_JAR" \
  --manifest "$ROOT/AndroidManifest.xml" \
  --java "$WORK/gen" \
  -A "$WORK/assets" \
  --min-sdk-version "$MIN_SDK" \
  --target-sdk-version "$TARGET_SDK" \
  --version-code "$VERSION_CODE" \
  --version-name "$VERSION_NAME" \
  --auto-add-overlay \
  "$WORK/res.zip"
log "resources linked"

# ---------------------------------------------------------------------- Java
mapfile -t SOURCES < <(find "$ROOT/src" "$WORK/gen" -name '*.java' | sort)
[ "${#SOURCES[@]}" -gt 0 ] || die "no Java sources found"
javac -encoding UTF-8 -source 17 -target 17 \
  -classpath "$ANDROID_JAR" -d "$WORK/classes" "${SOURCES[@]}"
"$BUILD_TOOLS/d8" --release --min-api "$MIN_SDK" --lib "$ANDROID_JAR" \
  --output "$WORK/dex" $(find "$WORK/classes" -name '*.class' | sort)
[ -f "$WORK/dex/classes.dex" ] || die "d8 produced no classes.dex"
log "Java compiled and dexed"

# ------------------------------------------------------------------- package
cp "$WORK/base.apk" "$WORK/unsigned.apk"
( cd "$WORK/dex" && zip -q "$WORK/unsigned.apk" classes.dex )
( cd "$WORK" && zip -q -r unsigned.apk lib )
"$BUILD_TOOLS/zipalign" -f -p 4 "$WORK/unsigned.apk" "$WORK/aligned.apk"

KEYSTORE="${TITANLD_KEYSTORE:-$WORK/titanld-release.jks}"
KS_PASS="${TITANLD_KEYSTORE_PASSWORD:-titanld-build}"
KEY_ALIAS="${TITANLD_KEY_ALIAS:-titanld}"
KEY_PASS="${TITANLD_KEY_PASSWORD:-titanld-build}"
if [ ! -f "$KEYSTORE" ]; then
  log "generating a build signing key (set TITANLD_KEYSTORE to reuse one)"
  keytool -genkeypair -noprompt \
    -keystore "$KEYSTORE" -storetype PKCS12 \
    -storepass "$KS_PASS" -keypass "$KEY_PASS" \
    -alias "$KEY_ALIAS" -keyalg RSA -keysize 2048 -validity 10000 \
    -dname "CN=TitanLD, OU=TitanLD, O=TitanLD, L=Unknown, ST=Unknown, C=US"
fi

OUT="$ROOT/build/TitanLD-${VERSION_NAME}.apk"
"$BUILD_TOOLS/apksigner" sign \
  --ks "$KEYSTORE" --ks-key-alias "$KEY_ALIAS" \
  --ks-pass "pass:$KS_PASS" --key-pass "pass:$KEY_PASS" \
  --out "$OUT" "$WORK/aligned.apk"
"$BUILD_TOOLS/apksigner" verify --verbose --print-certs "$OUT" >&2
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
"$BUILD_TOOLS/dexdump" "$OUT" > "$WORK/dexdump.txt" 2>/dev/null
grep -q 'Lio/github/mmunki/titanld/MainActivity;' "$WORK/dexdump.txt" \
  || die "MainActivity is missing from classes.dex"
unzip -p "$OUT" "assets/THIRD_PARTY_NOTICES.md" >/dev/null \
  || die "third-party notices are missing from the APK"
for abi in "${ABIS[@]}"; do
  grep -q "'$abi'" <<<"$BADGING" || die "badging does not list ABI $abi"
done
log "APK verified: package, label, dex, signature, notices and all four ABIs"
printf '\n%s\n' "$OUT"
