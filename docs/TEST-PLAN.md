# Xbox One / PlayStation 3 emulator: test plan

**Platform:** Original Xbox One (Jaguar, x64), with Xbox One S/X as secondary targets.  
**Purpose:** Evaluate whether an experimental PS3 emulator can boot homebrew and eventually a simple game in Xbox UWP Developer Mode.  
**Ground rule:** A rendered menu, an APPX package, a successful CI compile and a game successfully booting are four different milestones.

## T0 — Source and desktop baseline
- [x] Import the source from momo-AUX1/ChonkyStation3, retaining GPL-3.0 `LICENSE.md` and upstream commit history (merge commit `562494ed`, upstream commit `5093c7f`).
- [x] Record upstream SHA (`5093c7f`, 2025-04-01) and existing CMake Xbox build flag (`ENABLE_XBOX_BUILD`). Toolchain/dependency versions still require build verification.
- [ ] Validate recursive submodules and build host desktop target.
- [ ] Run the frontend in a controlled desktop environment; record failures.

## T1 — Xbox UWP host
- [ ] Build a real Xbox-compatible x64 UWP host with SDL2 or native WinRT integration.
- [ ] Load the emulator shared library with correct exports / calling conventions.
- [ ] Handle gamepad events, platform storage, graphics initialization and graceful exit.
- [ ] Display and log the Xbox model, runtime build version, memory usage and effective memory limit.
- [ ] Install a signed APPX/MSIX package and prove native launch **on an Xbox One**, not just a Windows PC.

## T2 — Interpreter / ELF smoke tests
- [ ] Exercise PPU interpreter on a small self-created test ELF.
- [ ] Exercise SPU instructions / synchronization with known expected results.
- [ ] Boot a minimal legally distributed PS3 homebrew ELF, with no proprietary files bundled in the repository.
- [ ] Log failure modes: access violations, unsupported syscalls, out-of-range memory, deadlocks.

## T3 — RSX graphics compatibility
- [ ] Confirm Xbox UWP graphics capabilities at runtime.
- [ ] Do not assume that the desktop OpenGL 4.1 backend can run unchanged on Xbox.
- [ ] Test an SDL2 UWP + GLES/ANGLE host first only if the required GL functions/shaders can actually be provided.
- [ ] If not, architect a translation layer / RSX backend with D3D11 or D3D12 according to confirmed UWP support.
- [ ] Separate emulation correctness from rendering artifacts.

## T4 — First commercial target
**Arkedo Series – 003 PIXEL!** (PS3 PSN 2D game; demonstrated by other ChonkyStation3 builds).

- [ ] Record lawful test asset provenance and its content ID privately (no copyrighted assets in GitHub).
- [ ] Check whether emulator loads the title and its required system modules.
- [ ] Reach title screen, then gameplay, then 5-minute stability test.
- [ ] Capture minimum/average FPS and 1% low, RAM peak, GPU problems and logs.
- [ ] Never call it 'playable' before controls, graphics, sound and progression have been tested on Xbox One.

## Version / result log template

| Build | Console model | UWP App/Game | Host boots | ELF runs | RSX renders | Game menu | In-game FPS | RAM peak | Notes |
|---|---|---|---|---|---|---|---|---|---|
| V0 (planning) | Xbox One | Not tested | No proof | No proof | No proof | No proof | N/A | N/A | No build released |

## Stop/go criteria

1. **No boot:** solve package / runtime / dependency issues before FPS tweaks.
2. **Boot but ELF fails:** debug CPU/syscall or memory correctness before renderer changes.
3. **ELF runs but no graphics:** focus on RSX backend and shader compatibility.
4. **Graphics render, FPS poor:** profile CPU, GPU and scheduler first. Do not promise a specific FPS.
5. **Crashes or excessive RAM:** profile allocations and actual UWP memory limits instead of adding unverified 'RAM cleanup' toggles.

## Upstream references

- Source / Xbox build: https://github.com/momo-AUX1/ChonkyStation3
- Original source: https://github.com/liuk7071/ChonkyStation3
- Experimental Xbox guide (Series S/X): https://wiki.xbdev.store/en/Chonkystation3Guide
- Microsoft UWP Xbox resource documentation: https://learn.microsoft.com/en-us/windows/uwp/xbox-apps/system-resource-allocation
