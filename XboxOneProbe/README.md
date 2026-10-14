# Xbox One Fat (2013) UWP Host Probe

**What this is:** A minimal x64 native UWP application with a real Direct3D 11 swap chain, Xbox controller polling, and memory/GPU diagnostics.

**What this is NOT:** The PlayStation 3 emulator and **not a game-playing build**. The `ChonkyStation3.dll` built for desktop Windows is not UWP-compatible and is not packaged inside the probe.

## What you should see

- Normal background: dark blue, indicating a functioning UWP window and D3D11 swap chain.
- Hold **A** on an Xbox controller: green background.
- Hold **B** on an Xbox controller: orange background.
- Every five seconds the app records the detected Direct3D feature level, controller count, UWP memory usage and current memory limit.
- Logs are written to the app's `LocalState/xbox-probe.log` and `OutputDebugStringW` (Visual Studio / Device Portal debugging).

If the app immediately crashes or displays a blank screen, please capture the crash and GPU initialization logs from Xbox Device Portal.

## CI build

GitHub Actions: [Xbox One Fat UWP graphics probe](https://github.com/kuaan08/ps3/actions/workflows/xbox-one-uwp-probe.yml).

On success the Actions artifact should contain a `.appx` and the public `.cer` of an ephemeral code-signing certificate. The private signing key is never included. Depending on Dev Mode trust settings, Xbox Device Portal may require the certificate to be trusted on the device before deployment succeeds.

**Do not install the desktop Windows DLL from the unrelated build workflow on the Xbox.**

## Build manually

Use Visual Studio 2022 on Windows with the **Universal Windows Platform development**, Desktop C++, and Windows 10 SDK 19041 components.

```powershell
./tools/generate-probe-assets.ps1
msbuild XboxOneProbe/XboxOneProbe.vcxproj /p:Configuration=Release /p:Platform=x64 /p:AppxPackageSigningEnabled=false /p:GenerateAppxPackageOnBuild=true
```

Sign the resulting UWP package with a trusted developer certificate (or use the GitHub Actions package for tests).

## Console checklist

1. Activate Xbox Developer Mode, not Retail Mode.
2. Ensure enough Dev Mode storage is allocated.
3. Install the **UWP x64** package using Xbox Device Portal.
4. Report whether the background appears and whether A and B change its color.
5. Capture the `xbox-probe.log` (GPU feature level, available UWP RAM).
6. If deployment fails, report its numeric error code and certificate trust status.

The probe does not read PS3 firmware or games and contains no copyrighted assets.

## Why this matters to the emulator

The ChonkyStation3 RSX code and shaders still rely on **desktop OpenGL 4.1 / GLSL 410**. Merely creating a Direct3D 11 device does **not** translate those GL calls into D3D. Once the probe is validated on the user's Xbox One Fat, the next milestone is to introduce a render backend abstraction and direct the RSX operations to UWP-compatible D3D11 shader and resource implementations.

Do not claim game compatibility before the PPU, SPU, RSX, storage and audio paths have all been tested on the target hardware.
