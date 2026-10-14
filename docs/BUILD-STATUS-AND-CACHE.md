# PS3 Emulator Build Targets & Cache

## Which build is for Xbox One Fat (2013)?

| Build workflow | Artifact | Contains actual PS3 emulator? | Native Xbox One UWP? | Playable? |
| --- | --- | --- | --- | --- |
| `xbox-one-uwp-probe.yml` | Small APPX + certificate | No. GPU/controller/memory diagnostics only | Yes (UWP x64, requires console validation) | No |
| `build-emulator-core.yml` | ChonkyStation3.dll | Yes. Full emulator code in a Windows desktop DLL | **No** (regular Win32 library) | No on Xbox One |
| `full-uwp-core.yml` | AppContainer DLL, only if compile succeeds | Attempts full emulator code | Attempts true native UWP x64 compilation | **Not yet**: host and RSX are unfinished |
| `xbox-one-core-tests.yml` | Test reports | Only isolated memory tests | No (Windows desktop) | No |

A DLL containing PS3 emulator logic is not enough by itself. An installable, functional Xbox One emulation package needs a **native UWP entry point** linked to the actual PS3 PPU/SPU/memory/RSX core, a **working RSX graphics backend** for the Xbox One GPU, input, audio, filesystem sandbox support, and packaging with compatible dependencies.

The existing Xbox One UWP probe does **not** use the ChonkyStation3 engine. Never market its tiny size or green/blue diagnostic screen as a PS3 emulator.

## Cache configuration (already enabled)

- Complete emulator Windows x64: `actions/cache@v4` restores `build-desktop` (CMake/MSVC intermediate object files). Cache key includes platform, CMake/submodule layout and commit; prefix restore reuses results across commits.
- Xbox One UWP probe: caches `XboxOneProbe/x64` and `XboxOneProbe/obj` (MSVC intermediate files).
- Experimental complete emulator UWP build: caches `build-uwp-core` to avoid repeating unchanged CMake/MSVC work after a successful run.

The **first** run with an empty cache will not be faster. GitHub's cache is immutable per key and can expire/be evicted; the time savings depend on the amount of code that actually changes and on the runner/SDK. Because submodules and source changes can invalidate intermediate outputs, always rerun tests when restoring caches.

**Do not** cache compiled binaries from desktop Windows and use them in UWP — they are different runtime targets. Avoid `/arch:AVX2` on Xbox One Fat: Jaguar does not support AVX2.

## Main blocker to a complete playable Xbox One APPX

ChonkyStation3 RSX currently generates `#version 410 core` GLSL and invokes desktop OpenGL 4.1 functions. Xbox One's UWP Direct3D device does not provide desktop OpenGL 4.1. The sample Xbox UWP host (SDL/ANGLE/OpenGL ES) does not magically make the missing features available.

A genuine Xbox One port needs either:
1. An RSX Direct3D renderer with shader translation to compatible HLSL and robust texture/buffer/framebuffer handling; or
2. A proved, Xbox One-compatible OpenGL implementation satisfying the required desktop GL 4.1 APIs and shader semantics.

Until that work is done, uploading an APPX that contains a Windows desktop DLL is **not** evidence of PS3 execution.

## Hardware test plan

Xbox One Fat targets UWP x64 in Developer Mode. First validate the signed diagnostic probe on the actual console, including D3D feature level and process RAM limit, then complete the emulator UWP compile, integrate the host/graphics, boot basic homebrew ELF, and only then evaluate small commercial games.

## References

- [Xbox One UWP probe](../XboxOneProbe/README.md)
- [Compatibility milestones](TEST-PLAN.md)
- [Xbox One DirectX UWP limitations](https://walbourn.github.io/directx-and-uwp-on-xbox-one/)
- [Official Microsoft UWP Xbox resource limits](https://learn.microsoft.com/en-us/windows/uwp/xbox-apps/system-resource-allocation)
