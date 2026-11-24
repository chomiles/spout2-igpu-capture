# Building Spout2 iGPU Capture

## Requirements

- Windows x64, OBS Studio 33.0.3 x64, and Git.
- Visual Studio 2022 with the x64 C++ build tools and Windows SDK.
- CMake 3.24 or newer and PowerShell 7.
- Inno Setup 6.7.3 or compatible to compile release packages.

From PowerShell 7, run:

```powershell
.\build.ps1
.\packaging\package.ps1 -Iscc 'C:\path\to\ISCC.exe' -Version '0.1.0'
```

For a custom OBS location, pass `-ObsBin 'D:\OBS\bin\64bit'` to `build.ps1`. The build script fetches the OBS 33.0.3 headers at commit `f2aa9ce69caa7239ef26660b5889de4f53d9880e` and builds an import library from the existing OBS DLL. The plugin DLL is produced under `build/Release`, and the installer, portable ZIP, and checksums are written to `dist`.

## License and upstream source

- This plugin is distributed under GPL-2.0-or-later. See [LICENSE](LICENSE).
- The [vendored Spout2 SDK subset](third_party/spout) comes from [Spout2 2.007.017 at commit f49e2f4](https://github.com/leadedge/Spout2/tree/f49e2f469f8cb25f559a6eaa61a3f5b8173fc100). The SDK files retain their original notices and are covered by the [Spout license](third_party/spout/LICENSE).
- Build headers and host APIs come from [OBS Studio 33.0.3](https://github.com/obsproject/obs-studio/tree/f2aa9ce69caa7239ef26660b5889de4f53d9880e).
