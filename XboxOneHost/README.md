# PS3 Xbox One Fat — experimental emulator package

This project targets **Xbox One Fat (2013), UWP x64 Developer Mode**. This is a new host project, separate from the tiny `XboxOneProbe` hardware-only APPX.

## What is included (only if full APPX CI succeeds)

The workflow [Xbox One Fat PS3 Core APPX](https://github.com/kuaan08/ps3/actions/workflows/xbox-one-emulator-appx.yml) **refuses to publish** a package if any of these are missing:

- `XboxOneHost.exe`: native UWP x64 entry point, Direct3D11 presentation, Xbox gamepad input, emulator-loading diagnostics and a boot attempt.
- `ChonkyStation3.dll`: actual UWP x64 PlayStation 3 emulator library, containing the imported PPU, SPU, RSX and OS implementations.
- `SDL2.dll`: UWP x64 SDL2 dependency.
- `AppxManifest.xml` and assets: UWP install metadata and icons.
- A separate `.cer` public testing certificate for developer-mode deployment.

**This is an experimental port, NOT yet a playable PS3 emulator.** A signed UWP package containing all these files does not imply game compatibility. The existing RSX renderer requires desktop OpenGL 4.1/GLSL 410, unavailable in the normal Xbox One UWP graphics environment. The Direct3D11 host does not automatically translate RSX OpenGL. Some homebrew will also require OS/HLE services not implemented by the upstream project.

## How to install (after a successful GitHub build)

1. Enable Xbox Developer Mode.
2. Download the **PS3-XboxOne-Fat-EXPERIMENTAL-UWP-FULL-CORE** artifact from the successful Actions run.
3. Extract its ZIP to access `PS3XboxOneFat-Experimental.appx` and `PS3XboxOneEmulatorDev.cer`.
4. Deploy the APPX using Xbox Device Portal. If the device rejects the developer-signing certificate, follow the device's certificate trust procedure.
5. Launch it. Check the Xbox console output and `LocalState/xbox-emulator.log` for module-load and GPU information.
6. Press **A** to attempt SDL2/GL boot. Any failure to create an OpenGL 4.1 context is a *known emulation graphics blocker*, not proof that an actual PS3 title is executing.

Never deploy the unrelated desktop `ChonkyStation3-Windows-x64-NOT-UWP` artifact to Xbox.

## Build caching

This workflow uses `actions/cache@v4` for `build-uwp-core` and `XboxOneHost/x64`. Cache keys account for project definitions and commits, with a restore prefix that enables incremental C++ builds. Fresh builds can remain slow. The native Windows desktop cache is deliberately not reused as an Xbox UWP cache.

## What remains for gameplay

- Adapt the RSX backend to D3D11 compatible with the Xbox One's available feature level, including shaders, textures, vertex buffers, framebuffer formats, synchronization and readback.
- Verify that the native core correctly initializes with Xbox sandbox files and host callbacks.
- Resolve unknown UWP API restrictions and guest PPU/SPU emulation compatibility issues.
- Implement audio/input/game installation and virtual filesystem flows suitable for Xbox.
- Test the actual console before claiming any particular title is playable.

No copyrighted Sony firmware, PS3 games, keys or decrypted commercial assets are included.

See [the build matrix](../docs/BUILD-STATUS-AND-CACHE.md) and [test plan](../docs/TEST-PLAN.md).
