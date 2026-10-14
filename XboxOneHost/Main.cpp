// Xbox One Fat experimental ChonkyStation3 UWP host.
// This hosts the compiled native PS3 emulator DLL and reports runtime errors.
// Compatibility requires native GL 4.1 RSX or a D3D renderer; games are not verified.
#include <windows.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <cstdio>
#include <string>
#include <algorithm>
#include <cstdint>

using Microsoft::WRL::ComPtr;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Windows::Gaming::Input;
using namespace Windows::Storage;
using namespace Windows::System;

namespace
{
    void WriteLog(const wchar_t* text)
    {
        OutputDebugStringW(text);
        OutputDebugStringW(L"\r\n");
        try
        {
            std::wstring path(ApplicationData::Current->LocalFolder->Path->Data());
            path += L"\\xbox-emulator.log";
            FILE* f = nullptr;
            if (_wfopen_s(&f, path.c_str(), L"a") == 0 && f != nullptr)
            {
                fwprintf(f, L"%ls\n", text);
                fclose(f);
            }
        }
        catch (Platform::Exception^)
        {
            OutputDebugStringW(L"Cannot create LocalState log");
        }
    }

    void CheckHR(HRESULT hr, const wchar_t* operation)
    {
        if (FAILED(hr))
        {
            wchar_t buf[240]{};
            swprintf_s(buf, L"%ls: HRESULT 0x%08X", operation, static_cast<unsigned>(hr));
            WriteLog(buf);
            throw ref new Platform::FailureException(ref new Platform::String(buf));
        }
    }
}

ref class EmulatorView sealed : IFrameworkView
{
public:
    virtual void Initialize(CoreApplicationView^ view)
    {
        view->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(
            this, &EmulatorView::OnActivated);
    }

    virtual void SetWindow(CoreWindow^ window)
    {
        window_ = window;
        window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(
            this, &EmulatorView::OnClosed);
        window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(
            this, &EmulatorView::OnVisibility);
        window->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(
            this, &EmulatorView::OnSizeChanged);
    }

    virtual void Load(Platform::String^)
    {
        WriteLog(L"PS3 Xbox One Fat: experimental native UWP emulator host starting");
        try
        {
            InitGraphics();
            WriteLog(L"D3D11 device and DXGI swap chain created");
            DiscoverCore();
        }
        catch (Platform::Exception^ e)
        {
            WriteLog(e->Message->Data());
            throw;
        }
    }

    virtual void Run()
    {
        uint64_t lastReport = 0;
        while (!closed_)
        {
            window_->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
            if (!visible_)
            {
                window_->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
                continue;
            }

            unsigned buttons = 0;
            auto pads = Gamepad::Gamepads;
            if (pads->Size > 0)
            {
                const auto reading = pads->GetAt(0)->GetCurrentReading();
                buttons = static_cast<unsigned>(reading.Buttons);
            }

            const bool pressedA = (buttons & static_cast<unsigned>(GamepadButtons::A)) != 0;
            const bool pressedB = (buttons & static_cast<unsigned>(GamepadButtons::B)) != 0;
            if (pressedA && !wasAPressed_)
                AttemptEmulationBoot();
            wasAPressed_ = pressedA;
            if (session_ && step_) {
                char fault[512]{};
                const int status = step_(session_, fault, sizeof(fault));
                if (status != 0) {
                    OutputDebugStringA(fault);
                    WriteLog(status == 1 ? L"PS3 homebrew stopped" : L"PS3 homebrew execution failed");
                    destroy_(session_);
                    session_ = nullptr;
                }
            }
            if (pressedB && session_) {
                destroy_(session_);
                session_ = nullptr;
                attempted_ = false;
                WriteLog(L"Guest session stopped by the Xbox B button");
            }
            Render(pressedA, pressedB);

            const uint64_t now = GetTickCount64();
            if (now - lastReport > 5000)
            {
                lastReport = now;
                wchar_t info[256]{};
                swprintf_s(info,
                    L"GPU D3D FL: 0x%04X; gamepads: %u; UWP RAM: %llu MiB / %llu MiB; A: %d B: %d",
                    static_cast<unsigned>(featureLevel_),
                    static_cast<unsigned>(pads->Size),
                    static_cast<unsigned long long>(MemoryManager::AppMemoryUsage / (1024 * 1024)),
                    static_cast<unsigned long long>(MemoryManager::AppMemoryUsageLimit / (1024 * 1024)),
                    pressedA ? 1 : 0, pressedB ? 1 : 0);
                if (session_ && frames_) {
                    wchar_t guest[120]{};
                    swprintf_s(guest, L"Guest RSX flips so far: %llu", frames_(session_));
                    WriteLog(guest);
                }
                WriteLog(info);
            }
        }
    }

    virtual void Uninitialize()
    {
        WriteLog(L"UWP probe shutting down");
        if (session_ && destroy_) {
            destroy_(session_);
            session_ = nullptr;
        }
        target_.Reset();
        swapChain_.Reset();
        context_.Reset();
        device_.Reset();
        if (core_) FreeLibrary(core_);
        if (sdl_) FreeLibrary(sdl_);
    }

private:
    CoreWindow^ window_ = nullptr;
    bool closed_ = false;
    bool visible_ = true;
    bool wasAPressed_ = false;
    bool attempted_ = false;
    bool emulationReady_ = false;
    HMODULE core_ = nullptr;
    HMODULE sdl_ = nullptr;
    using GuestCreateFn = void* (__cdecl*)(const char*, const char*,
        ID3D11DeviceContext*, ID3D11RenderTargetView*, char*, size_t);
    using GuestStepFn = int (__cdecl*)(void*, char*, size_t);
    using GuestDestroyFn = void (__cdecl*)(void*);
    using GuestFrameFn = unsigned long long (__cdecl*)(void*);
    GuestCreateFn create_ = nullptr;
    GuestStepFn step_ = nullptr;
    GuestDestroyFn destroy_ = nullptr;
    GuestFrameFn frames_ = nullptr;
    void* session_ = nullptr;

    static std::string Utf8(const std::wstring& wide) {
        const int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1,
            nullptr, 0, nullptr, nullptr);
        if (size <= 0) return {};
        std::string str(static_cast<size_t>(size), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &str[0], size, nullptr, nullptr);
        str.resize(static_cast<size_t>(size - 1));
        return str;
    }

    // This host is a real bridge to the PS3 emulation library. It does not
    // silently replace an unavailable graphics renderer with a fake game.
    void DiscoverCore()
    {
        core_ = LoadPackagedLibrary(L"ChonkyStation3.dll", 0);
        if (!core_) {
            wchar_t status[128]{};
            swprintf_s(status, L"PS3 core DLL load failed: Windows error %lu", GetLastError());
            WriteLog(status);
            return;
        }
        if (!GetProcAddress(core_, "external_main")) {
            WriteLog(L"ChonkyStation3.dll was found, but external_main export is missing");
            return;
        }
        sdl_ = LoadPackagedLibrary(L"SDL2.dll", 0);
        if (!sdl_) {
            wchar_t status[128]{};
            swprintf_s(status, L"SDL2 DLL load failed: Windows error %lu", GetLastError());
            WriteLog(status);
            return;
        }
        create_ = reinterpret_cast<GuestCreateFn>(GetProcAddress(core_, "xbox_ps3_create"));
        step_ = reinterpret_cast<GuestStepFn>(GetProcAddress(core_, "xbox_ps3_step"));
        destroy_ = reinterpret_cast<GuestDestroyFn>(GetProcAddress(core_, "xbox_ps3_destroy"));
        frames_ = reinterpret_cast<GuestFrameFn>(GetProcAddress(core_, "xbox_ps3_frames"));
        emulationReady_ = true;
        if (create_ && step_ && destroy_)
            WriteLog(L"Native RSX D3D11 guest API found. Copy homebrew/boot.elf into LocalState; press A to run.");
        else
            WriteLog(L"Native RSX D3D11 guest API absent; legacy SDL OpenGL path is not suitable for Xbox One.");
    }

    void AttemptEmulationBoot()
    {
        if (attempted_) return;
        attempted_ = true;
        if (!emulationReady_) {
            WriteLog(L"Emulation not started: UWP core or SDL2 library is unavailable");
            return;
        }
        if (create_ && step_ && destroy_) {
            const std::wstring local(ApplicationData::Current->LocalFolder->Path->Data());
            const std::wstring elf = local + L"\\homebrew\\boot.elf";
            if (GetFileAttributesW(elf.c_str()) == INVALID_FILE_ATTRIBUTES) {
                WriteLog(L"HOME BREW NOT FOUND: transfer LocalState/homebrew/boot.elf via Device Portal.");
                attempted_ = false;
                return;
            }
            const std::string elf8 = Utf8(elf);
            const std::string state8 = Utf8(local);
            if (elf8.empty() || state8.empty()) {
                WriteLog(L"PS3 ELF or LocalState path UTF-8 conversion failed");
                attempted_ = false;
                return;
            }
            char error[512]{};
            session_ = create_(elf8.c_str(), state8.c_str(),
                               context_.Get(), target_.Get(), error, sizeof(error));
            if (!session_) {
                OutputDebugStringA(error);
                WriteLog(L"PS3 guest initialization failed (see preceding debug output)");
                attempted_ = false;
                return;
            }
            WriteLog(L"Guest PPU/SPU initialized with native RSX D3D11 clear/flip backend");
            return;
        }
        using SDLInitFn = int(__cdecl*)(unsigned);
        using SDLCreateWindowFn = void*(__cdecl*)(const char*, int, int, int, int, unsigned);
        using SDLGLCreateContextFn = void*(__cdecl*)(void*);
        using SDLDestroyWindowFn = void(__cdecl*)(void*);
        using SDLGLDeleteContextFn = void(__cdecl*)(void*);
        using SDLQuitFn = void(__cdecl*)();
        using SDLGetErrorFn = const char*(__cdecl*)();
        using EmulatorMainFn = int(__cdecl*)(void*, void*, int, char**);

        const auto init = reinterpret_cast<SDLInitFn>(GetProcAddress(sdl_, "SDL_Init"));
        const auto createWindow = reinterpret_cast<SDLCreateWindowFn>(GetProcAddress(sdl_, "SDL_CreateWindow"));
        const auto createContext = reinterpret_cast<SDLGLCreateContextFn>(GetProcAddress(sdl_, "SDL_GL_CreateContext"));
        const auto destroyWindow = reinterpret_cast<SDLDestroyWindowFn>(GetProcAddress(sdl_, "SDL_DestroyWindow"));
        const auto deleteContext = reinterpret_cast<SDLGLDeleteContextFn>(GetProcAddress(sdl_, "SDL_GL_DeleteContext"));
        const auto quit = reinterpret_cast<SDLQuitFn>(GetProcAddress(sdl_, "SDL_Quit"));
        const auto getError = reinterpret_cast<SDLGetErrorFn>(GetProcAddress(sdl_, "SDL_GetError"));
        const auto ps3Main = reinterpret_cast<EmulatorMainFn>(GetProcAddress(core_, "external_main"));

        if (!init || !createWindow || !createContext || !destroyWindow ||
            !deleteContext || !quit || !getError || !ps3Main) {
            WriteLog(L"SDL2/ChonkyStation3 API entry points missing");
            return;
        }
        WriteLog(L"Starting SDL2 video+controller for actual emulation");
        // SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER.
        if (init(0x20u | 0x2000u) != 0) {
            OutputDebugStringA(getError());
            WriteLog(L"SDL2 initialization failed in the UWP sandbox");
            return;
        }
        // SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN. Native GL 4.1 is not
        // provided by Xbox One UWP; this may fail until RSX is ported.
        void* window = createWindow("ChonkyStation3 Xbox One", 0, 0, 1280, 720, 0x2u | 0x4u);
        if (!window) {
            OutputDebugStringA(getError());
            WriteLog(L"SDL OpenGL window unavailable; native RSX/D3D port required");
            quit();
            return;
        }
        void* gl = createContext(window);
        if (!gl) {
            OutputDebugStringA(getError());
            WriteLog(L"SDL desktop GL context unavailable; RSX graphics port required");
            destroyWindow(window);
            quit();
            return;
        }
        WriteLog(L"Calling real ChonkyStation3 PS3 emulation entry point");
        std::wstring localPath(ApplicationData::Current->LocalFolder->Path->Data());
        const int utf8Size = WideCharToMultiByte(CP_UTF8, 0, localPath.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8Size <= 0) {
            WriteLog(L"LocalState path conversion failed");
            deleteContext(gl);
            destroyWindow(window);
            quit();
            return;
        }
        std::string utf8(static_cast<size_t>(utf8Size), '\0');
        WideCharToMultiByte(CP_UTF8, 0, localPath.c_str(), -1, &utf8[0], utf8Size, nullptr, nullptr);
        char executable[] = "ChonkyStation3";
        char blank[] = "";
        char* args[] = { executable, blank, &utf8[0] };
        // external_main blocks while the guest runs. Any failure is logged;
        // never claim a boot without explicit hardware test evidence.
        const int result = ps3Main(window, gl, 3, args);
        wchar_t msg[128]{};
        swprintf_s(msg, L"PS3 core exited with code %d", result);
        WriteLog(msg);
        deleteContext(gl);
        destroyWindow(window);
        quit();
    }

    D3D_FEATURE_LEVEL featureLevel_ = D3D_FEATURE_LEVEL_9_1;
    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<IDXGISwapChain1> swapChain_;
    ComPtr<ID3D11RenderTargetView> target_;

    void OnActivated(CoreApplicationView^, IActivatedEventArgs^)
    {
        window_->Activate();
    }

    void OnClosed(CoreWindow^, CoreWindowEventArgs^)
    {
        closed_ = true;
    }

    void OnVisibility(CoreWindow^, VisibilityChangedEventArgs^ args)
    {
        visible_ = args->Visible;
    }

    void OnSizeChanged(CoreWindow^, WindowSizeChangedEventArgs^)
    {
        if (!swapChain_) return;
        if (session_ && destroy_) {
            destroy_(session_);
            session_ = nullptr;
            attempted_ = false;
            WriteLog(L"Xbox display resized; guest stopped to prevent stale D3D11 render targets");
        }
        context_->OMSetRenderTargets(0, nullptr, nullptr);
        target_.Reset();
        // BufferCount 0 keeps the previous DXGI buffer count.
        CheckHR(swapChain_->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0), L"ResizeBuffers");
        CreateRenderTarget();
    }

    void InitGraphics()
    {
        const D3D_FEATURE_LEVEL levels[] = {
            D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
        };
        CheckHR(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT, levels, ARRAYSIZE(levels),
            D3D11_SDK_VERSION, device_.GetAddressOf(), &featureLevel_,
            context_.GetAddressOf()), L"D3D11CreateDevice");

        ComPtr<IDXGIDevice1> dxgiDevice;
        CheckHR(device_.As(&dxgiDevice), L"IDXGIDevice1");
        ComPtr<IDXGIAdapter> adapter;
        CheckHR(dxgiDevice->GetAdapter(adapter.GetAddressOf()), L"GetAdapter");
        ComPtr<IDXGIFactory2> factory;
        CheckHR(adapter->GetParent(IID_PPV_ARGS(factory.GetAddressOf())), L"DXGIFactory2");

        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width = 0;
        desc.Height = 0;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        auto coreWindow = reinterpret_cast<IUnknown*>(window_);
        CheckHR(factory->CreateSwapChainForCoreWindow(
            device_.Get(), coreWindow, &desc, nullptr,
            swapChain_.GetAddressOf()), L"CreateSwapChainForCoreWindow");
        CreateRenderTarget();
    }

    void CreateRenderTarget()
    {
        ComPtr<ID3D11Texture2D> buffer;
        CheckHR(swapChain_->GetBuffer(0, IID_PPV_ARGS(buffer.GetAddressOf())), L"GetBuffer");
        CheckHR(device_->CreateRenderTargetView(buffer.Get(), nullptr, target_.GetAddressOf()),
            L"CreateRenderTargetView");
    }

    void Render(bool a, bool b)
    {
        const float normal[] = { 0.05f, 0.11f, 0.19f, 1.f };
        const float yes[] = { 0.03f, 0.48f, 0.16f, 1.f };
        const float no[] = { 0.65f, 0.18f, 0.02f, 1.f };
        context_->OMSetRenderTargets(1, target_.GetAddressOf(), nullptr);
        const float ready[] = { 0.03f, 0.21f, 0.12f, 1.f };
        const float blocked[] = { 0.27f, 0.09f, 0.07f, 1.f };
        if (!session_) {
            context_->ClearRenderTargetView(target_.Get(),
                attempted_ ? blocked : emulationReady_ ? ready : b ? no : normal);
        }
        CheckHR(swapChain_->Present(1, 0), L"Present");
    }
};

ref class EmulatorViewSource sealed : IFrameworkViewSource
{
public:
    virtual IFrameworkView^ CreateView() { return ref new EmulatorView(); }
};

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
    CoreApplication::Run(ref new EmulatorViewSource());
    return 0;
}
