#!/usr/bin/env bash
# ============================================================================
# build.sh -- build TitanLD's release APK with the Android SDK command line
# tools only.  No Gradle, no network dependency beyond the pinned renderer
# core download.
#
# Inputs (environment, all optional):
#   ANDROID_HOME / ANDROID_SDK_ROOT   the Android SDK location
#   TITANLD_KEYSTORE                  an existing signing keystore
#   TITANLD_KEYSTORE_PASSWORD         its store password
#   TITANLD_KEY_ALIAS                 the key alias
#   TITANLD_KEY_PASSWORD              the key password
#
# Output: build/TitanLD-<version>.apk
# ============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

VERSION_NAME="1.0.0"
VERSION_CODE="1"
CORE_VERSION="2.0.0"
CORE_APK="MobileGlues_${CORE_VERSION}.apk"
CORE_URL="https://github.com/MobileGL-Dev/MobileGlues-release/releases/download/V${CORE_VERSION}/${CORE_APK}"
CORE_SHA256="a7e1eb29731fdece7ab3af7fb00692b856f1808a7544436e7b3eb6c08355581a"
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
  # Prefer a stable directory; only fall back to previews/betas when nothing
  # stable is installed.  sorted -V puts android-36 after android-35.
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
for tool in aapt2 d8 apksigner zipalign; do
  [ -x "$BUILD_TOOLS/$tool" ] || die "missing $BUILD_TOOLS/$tool"
done
ANDROID_JAR="$PLATFORM/android.jar"
log "SDK:          $SDK"
log "build-tools:  $(basename "$BUILD_TOOLS")"
log "platform:     $(basename "$PLATFORM")"

command -v javac   >/dev/null || die "javac (JDK 17+) is required"
command -v keytool >/dev/null || die "keytool (JDK 17+) is required"
command -v curl    >/dev/null || die "curl is required"
command -v unzip   >/dev/null || die "unzip is required"
command -v zip     >/dev/null || die "zip is required"
command -v sha256sum >/dev/null || die "sha256sum is required"

# ------------------------------------------------------- renderer core asset
WORK="$ROOT/build"
CACHE="$WORK/cache"
rm -rf "$WORK"
mkdir -p "$CACHE" "$WORK/lib" "$WORK/gen" "$WORK/classes" "$WORK/dex" "$WORK/assets"

MG_APK="$CACHE/$CORE_APK"
if [ ! -f "$MG_APK" ]; then
  log "downloading renderer core $CORE_VERSION"
  curl -fL --retry 3 --retry-delay 2 -o "$MG_APK.part" "$CORE_URL"
  mv "$MG_APK.part" "$MG_APK"
fi
echo "$CORE_SHA256  $MG_APK" | sha256sum -c - >/dev/null \
  || die "renderer core checksum mismatch: $MG_APK"
log "renderer core verified: sha256 $CORE_SHA256"

for abi in "${ABIS[@]}"; do
  mkdir -p "$WORK/lib/$abi"
  unzip -p "$MG_APK" "lib/$abi/libmobileglues.so" > "$WORK/lib/$abi/libmobileglues.so"
  [ -s "$WORK/lib/$abi/libmobileglues.so" ] || die "renderer core has no lib/$abi/libmobileglues.so"
done

# ABI validation: every packaged library must really be ELF of its ABI.
declare -A EXPECT=(
  [arm64-v8a]='ARM aarch64'
  [armeabi-v7a]='ARM'
  [x86]='Intel 80386'
  [x86_64]='x86-64'
)
for abi in "${ABIS[@]}"; do
  desc="$(file -b "$WORK/lib/$abi/libmobileglues.so")"
  case "$desc" in
    *"${EXPECT[$abi]}"*) ;;
    *) die "lib/$abi is not ${EXPECT[$abi]}: $desc" ;;
  esac
  log "ABI $abi: ${desc%%,*} ($(stat -c%s "$WORK/lib/$abi/libmobileglues.so") bytes)"
done

# ----------------------------------------------------------------- resources
cp -R "$ROOT/assets/." "$WORK/assets/" 2>/dev/null || true
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
unzip -l "$OUT" | grep -E 'classes.dex|resources.arsc|lib/.+/libmobileglues.so|assets/' >&2 || true

for abi in "${ABIS[@]}"; do
  unzip -l "$OUT" | grep -q "lib/$abi/libmobileglues.so" \
    || die "built APK is missing lib/$abi/libmobileglues.so"
done

BADGING="$("$BUILD_TOOLS/aapt2" dump badging "$OUT")"
grep -q "package: name='io.github.mmunki.titanld'" <<<"$BADGING" \
  || die "unexpected package name"
grep -q "application-label:'TitanLD'" <<<"$BADGING" \
  || die "unexpected application label"
"$BUILD_TOOLS/dexdump" "$OUT" > "$WORK/dexdump.txt" 2>/dev/null
grep -q 'Lio/github/mmunki/titanld/MainActivity;' "$WORK/dexdump.txt" \
  || die "MainActivity is missing from classes.dex"
unzip -p "$OUT" "assets/licenses/MobileGlues-LGPL-2.1.txt" > "$WORK/lgpl.txt"
grep -q "GNU LESSER GENERAL PUBLIC LICENSE" "$WORK/lgpl.txt" \
  || die "LGPL licence text is missing from assets"
for abi in "${ABIS[@]}"; do
  grep -q "'$abi'" <<<"$BADGING" || die "badging does not list ABI $abi"
done
log "APK verified: package, label, dex, signature and all four ABIs"
printf '\n%s\n' "$OUT"
