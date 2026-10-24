# Third-party notices

## TitanGL

TitanGL, the renderer core in `src/native/`, is part of TitanLD and is
licensed under the MIT licence included as `assets/LICENSE.txt`.

## Miuix, AndroidX and Jetpack Compose

The manager UI is built with Miuix 0.9.4 (`top.yukonga.miuix.kmp:miuix-ui`
and `miuix-icons`) and with AndroidX Activity and Jetpack Compose.  Those
libraries are licensed under the Apache License 2.0, included as
`assets/licenses/Apache-2.0.txt`.

No third-party renderer code is compiled into this APK.  The MobileGlues
library is not downloaded, linked, packaged or loaded.

## Historical notice

Release 1.0.0 of TitanLD packaged the unmodified MobileGlues 2.0.0 renderer
core under the GNU LGPL-2.1.  The old licence text and notice remain in the
source tree for that published release, but they are not part of this APK.

## Build tools

The APK is built with the Android SDK command-line tools, the NDK and Gradle.
No other code is bundled into the APK.
