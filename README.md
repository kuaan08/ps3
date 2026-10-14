# PS3 on Xbox One — Experimental Port

An experimental PlayStation 3 emulator port targeting **Xbox One / Xbox One S / Xbox One X in UWP Developer Mode (x64)**.

> **Status: ChonkyStation3 Xbox source imported (upstream commit `5093c7f`, 2025-04-01).** No Xbox One APPX or playable PS3 game has been verified in this repository. This is not a release-ready emulator.

## Upstream

Primary experimental baseline: [momo-AUX1/ChonkyStation3](https://github.com/momo-AUX1/ChonkyStation3) (the Xbox UWP-oriented fork of [liuk7071/ChonkyStation3](https://github.com/liuk7071/ChonkyStation3)).

Imported baseline from the Xbox-oriented fork with its upstream commit history and GPL-3.0 license, retaining our Xbox One-specific project documentation. Future changes should remain reviewable commits. [RPCS3](https://github.com/RPCS3/rpcs3) is a technical reference, **not** the source tree imported here.

## Why this baseline?

The Xbox fork already has an `ENABLE_XBOX_BUILD` option, an SDL2 + ImGui frontend path, PPU/SPU interpreter code and an OpenGL-based RSX renderer. However, its build outputs a **shared library**, not a self-contained APPX. An external UWP host and compatible rendering context are required. The available Xbox POC documentation targets **Xbox Series S/X**, not the original Xbox One.

## Goals, in order

1. **Imported:** baseline source and history. **Next:** verify submodules and native desktop build.
2. Create / validate an actual x64 UWP host with working window, controller, logs and measured memory budget on Xbox One.
3. Exercise the PPU/SPU interpreter and run a minimal homebrew ELF **on the console**.
4. Make the RSX render path compatible with the UWP graphics stack; investigate GLES/ANGLE or an explicit Direct3D backend.
5. Attempt **Arkedo Series – 003 PIXEL!** as the first *commercial compatibility target*. It has been demonstrated on ChonkyStation3 elsewhere; this does **not** establish Xbox One compatibility.
6. Measure and optimize frametimes, memory, emulation correctness and crashes before attempting more demanding games.

See [docs/TEST-PLAN.md](docs/TEST-PLAN.md) for exit criteria and reporting format.

## Important limitations

- The AMD Jaguar CPU in the Xbox One lacks AVX2 and has limited performance for PS3 emulation.
- UWP App and Game resource budgets differ; do not assume 5 GB is available unless the measured Xbox app classification confirms it.
- Native desktop OpenGL 4.x contexts are **not** guaranteed in Xbox UWP; a CMake Xbox flag alone does not provide a functioning render backend.
- No PS3 firmware, keys, decrypted commercial binaries or copyrighted game files are included.
- Only use software and firmware obtained lawfully.

## License and credits

ChonkyStation3 is licensed under the **GNU GPL v3** (see upstream `LICENSE.md`). When importing code, retain its license, full attribution and applicable notices. This project is unofficial, is not affiliated with Sony or Microsoft, and makes no claims of game compatibility before hardware validation.
