// Xbox One Fat (2013) UWP Direct3D 11 host probe.
// This is a native, AppContainer-safe graphics/input/memory diagnostic.
// It deliberately does NOT claim to execute PlayStation 3 titles.
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
            path += L"\\xbox-probe.log";
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

ref class ProbeView sealed : IFrameworkView
{
public:
    virtual void Initialize(CoreApplicationView^ view)
    {
        view->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(
            this, &ProbeView::OnActivated);
    }

    virtual void SetWindow(CoreWindow^ window)
    {
        window_ = window;
        window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(
            this, &ProbeView::OnClosed);
        window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(
            this, &ProbeView::OnVisibility);
        window->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(
            this, &ProbeView::OnSizeChanged);
    }

    virtual void Load(Platform::String^)
    {
        WriteLog(L"PS3 Xbox One Fat: UWP x64 graphics diagnostic starting");
        try
        {
            InitGraphics();
            WriteLog(L"D3D11 device and DXGI swap chain created");
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
                WriteLog(info);
            }
        }
    }

    virtual void Uninitialize()
    {
        WriteLog(L"UWP probe shutting down");
        target_.Reset();
        swapChain_.Reset();
        context_.Reset();
        device_.Reset();
    }

private:
    CoreWindow^ window_ = nullptr;
    bool closed_ = false;
    bool visible_ = true;
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
        context_->ClearRenderTargetView(target_.Get(), a ? yes : b ? no : normal);
        CheckHR(swapChain_->Present(1, 0), L"Present");
    }
};

ref class ProbeViewSource sealed : IFrameworkViewSource
{
public:
    virtual IFrameworkView^ CreateView() { return ref new ProbeView(); }
};

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
    CoreApplication::Run(ref new ProbeViewSource());
    return 0;
}
