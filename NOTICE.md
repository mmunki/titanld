# Third-party notices

## MobileGlues 2.0.0 renderer core

- Upstream: https://github.com/MobileGL-Dev/MobileGlues
- Releases: https://github.com/MobileGL-Dev/MobileGlues-release
- Copyright: MobileGL-Dev and contributors
- Licence: GNU Lesser General Public License v2.1 (LGPL-2.1)
- Licence text: `licenses/MobileGlues-LGPL-2.1.txt`
- Binary used: `MobileGlues_2.0.0.apk`, asset
  `lib/<abi>/libmobileglues.so`, SHA-256 of the upstream APK
  `a7e1eb29731fdece7ab3af7fb00692b856f1808a7544436e7b3eb6c08355581a`
- Changes: none. The library is redistributed unmodified inside the TitanLD
  APK. TitanLD adds the Android manifest, permission flow, configuration
  writer and UI, all of which are published in this repository and in the
  source archive attached to every release.

The renderer core is dynamically loaded by the launcher as
`libmobileglues.so`. The complete corresponding source of the core is the
upstream repository above; the complete source of the TitanLD wrapper is this
repository.

## Build tools

The APK is built with the Android SDK command-line tools. No other code is
bundled into the APK.
