#include "PlayStation3.hpp"

#ifdef CHONKYSTATION3_UWP_D3D11
#include <d3d11.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <filesystem>

// Opaque C ABI avoids leaking C++ STL, WinRT or PS3 core types into the
// Xbox UWP host executable. Called on the same thread as its D3D11 context.
// This is an interpreter-backed homebrew test, not a retail game loader.
struct XboxPS3Session {
    std::unique_ptr<PlayStation3> ps3;
    unsigned long long frames = 0;
};

static void writeHostError(char* buffer, size_t capacity, const char* text) noexcept {
    if (!buffer || !capacity) return;
    if (!text) text = "Unknown PS3 error";
    const auto length = std::strlen(text);
    const auto n = length < capacity - 1 ? length : capacity - 1;
    std::memcpy(buffer, text, n);
    buffer[n] = '\0';
}

extern "C" __declspec(dllexport)
void* __cdecl xbox_ps3_create(const char* elf_utf8,
                              const char* local_state_utf8,
                              ID3D11DeviceContext* context,
                              ID3D11RenderTargetView* target,
                              char* error,
                              size_t error_capacity) noexcept {
    writeHostError(error, error_capacity, "");
    try {
        if (!elf_utf8 || !*elf_utf8 || !local_state_utf8 || !*local_state_utf8)
            throw std::runtime_error("A PS3 homebrew ELF path and UWP LocalState path are required");
        if (!context || !target)
            throw std::runtime_error("A native Xbox D3D11 context and render target are required");
        if (_putenv_s("CHONKYSTATION3_LOCAL_STATE_PATH", local_state_utf8) != 0)
            throw std::runtime_error("Unable to set sandbox game-data path");

        auto session = std::make_unique<XboxPS3Session>();
        // C++23 WindowsStore: avoid deprecated u8path and preserve UTF-8.
        const auto elf_path = std::filesystem::path(
            std::u8string(reinterpret_cast<const char8_t*>(elf_utf8)));
        session->ps3 = std::make_unique<PlayStation3>(elf_path);
        session->ps3->rsx.initD3D11(context, target);
        session->ps3->setFlipHandler([pointer = session.get()] {
            ++pointer->frames;
        });
        session->ps3->init();
        return session.release();
    } catch (const std::exception& e) {
        writeHostError(error, error_capacity, e.what());
    } catch (...) {
        writeHostError(error, error_capacity, "Unhandled PS3 homebrew initialization failure");
    }
    return nullptr;
}

// Return 0 for a cooperative slice, 1 if the emulator stops, -1 for error.
// Never let C++ exceptions cross the UWP host/DLL boundary.
extern "C" __declspec(dllexport)
int __cdecl xbox_ps3_step(void* opaque, char* error, size_t error_capacity) noexcept {
    writeHostError(error, error_capacity, "");
    if (!opaque) {
        writeHostError(error, error_capacity, "PS3 session is null");
        return -1;
    }
    try {
        auto* session = static_cast<XboxPS3Session*>(opaque);
        session->ps3->run();
        if (session->ps3->stopped) {
            writeHostError(error, error_capacity, "Guest stopped; inspect PPU/SPU trace logs");
            return 1;
        }
        return 0;
    } catch (const std::exception& e) {
        writeHostError(error, error_capacity, e.what());
        return -1;
    } catch (...) {
        writeHostError(error, error_capacity, "Unhandled PS3 guest execution error");
        return -1;
    }
}

extern "C" __declspec(dllexport)
unsigned long long __cdecl xbox_ps3_frames(void* opaque) noexcept {
    return opaque ? static_cast<XboxPS3Session*>(opaque)->frames : 0;
}

extern "C" __declspec(dllexport)
void __cdecl xbox_ps3_destroy(void* opaque) noexcept {
    delete static_cast<XboxPS3Session*>(opaque);
}

#endif
