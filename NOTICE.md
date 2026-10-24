# Third-party notices

## TitanGL

TitanGL, the renderer core in `src/native/`, is part of TitanLD and is
licensed under the MIT licence in `LICENSE`.  Copyright (c) 2026 mmunki.

## Miuix, AndroidX and Jetpack Compose

The manager UI is built with Miuix 0.9.4 (`top.yukonga.miuix.kmp:miuix-ui`
and `miuix-icons`) and with AndroidX Activity and Jetpack Compose.  Those
libraries are licensed under the Apache License 2.0; the licence text is in
`licenses/Apache-2.0.txt` and is packaged in the APK as
`assets/licenses/Apache-2.0.txt`.

No third-party renderer code is compiled into TitanLD 3.0.0.  In particular,
the MobileGlues library is not downloaded, linked, packaged or loaded.

## Historical: MobileGlues 2.0.0 (release 1.0.0 only)

Release 1.0.0 of this repository packaged the unmodified MobileGlues 2.0.0
renderer core (`libmobileglues.so`, LGPL-2.1).  The licence text and the old
notice are kept in the source tree for that published release:

- `licenses/MobileGlues-LGPL-2.1.txt`
- `assets/licenses/MobileGlues-LGPL-2.1.txt`

They are not part of the 3.0.0 APK.  The old source is still in the Git
history; the files were not deleted because published releases remain
reproducible and the project does not rewrite history.

## Build tools

The APK is built with the Android SDK command-line tools, the NDK and Gradle.
No other code is bundled into the APK.
