# Building the source preview

The public build entry is the shared portable C++20 runtime. It uses vendored
open-source components and does not require the original executable, IDA,
RenderWare SDK, proprietary DLLs, game resources, JNI or a console SDK.
This is a host contract-test build, not a standalone playable game.

On Windows install Python 3, CMake 3.22+ and Visual Studio 2022 Build Tools
with the C++ desktop workload. In PowerShell, from the source directory:

```powershell
$Pal4Build = Join-Path $env:TEMP 'pal4-portable-build'
cmake -S fresh/ports/runtime -B $Pal4Build -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON -DPAL4_ENABLE_GLES=OFF -DPAL4_MODERN_GAMEPAD=OFF
cmake --build $Pal4Build --config Debug --parallel 2
ctest --test-dir $Pal4Build -C Debug --output-on-failure
```

Use Debug for contract tests: fixtures use C/C++ assertions. Build outputs
must stay outside the source tree. The development machine uses
`H:/PAL4/objs/` instead of the temporary directory above.
The `Portable host contracts` GitHub Actions workflow runs this Windows build
without game resources; adding the workflow does not mean a hosted run has
already passed.

Platform applications in the full development tree have additional dependencies; their projects are not included in this host-only preview:

| Target | Entry | Additional requirements |
| --- | --- | --- |
| Android | `fresh/ports/android/README.md` | Android SDK/NDK, Gradle/JDK; CEGUI/FreeType and optional FFmpeg configured externally |
| Switch | `fresh/ports/switch_game/README.md` | devkitPro/libnx, portlibs, CEGUI/FreeType, SDL and mpv |
| Linux | `fresh/ports/linux_game/CMakeLists.txt` | SDL2/ttf/mixer, mpv, FreeType, GLES/EGL and external CEGUI 0.4.1 |
| Windows original-comparison research | `fresh/native/renderware` | external proprietary RenderWare/D3DX inputs and original DLLs; outside the self-contained public build |

Game execution requires the user's original game data. Do not distribute game
CPKs, extracted scripts, textures, music, fonts, videos, original DLLs or SDK
archives. Platform dependency versions and binary redistribution obligations
are still being audited; a successful build is not release clearance.

See [release status](docs/release/STATUS.md) and
[third-party inventory](THIRD_PARTY_NOTICES.md). Project-owned code is licensed
under [GPL-3.0-or-later](LICENSING.md); platform binary releases still require their own
source/dependency/signing checks.
