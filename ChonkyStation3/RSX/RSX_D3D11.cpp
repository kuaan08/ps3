#include "RSX.hpp"
#include "PlayStation3.hpp"

#ifdef CHONKYSTATION3_UWP_D3D11
#include <d3d11.h>
#include <stdexcept>

// First native Xbox One RSX backend milestone: guest command FIFO, clear,
// semaphore, and flip. Vertex/pixel shaders and textured draw operations
// must be implemented in D3D11 before commercial gameplay is possible.
void RSX::initD3D11(ID3D11DeviceContext* context, ID3D11RenderTargetView* target) {
    if (!context || !target)
        throw std::runtime_error("RSX D3D11 requires a valid host context and render target");
    d3d11_context = context;
    d3d11_target = target;
    d3d11_active = true;
    clear_color.r() = 0.f;
    clear_color.g() = 0.f;
    clear_color.b() = 0.f;
    clear_color.a() = 1.f;
    // The host owns both COM objects and the IDXGISwapChain.
}

void RSX::runCommandListD3D11(u64 /*put_addr*/) {
    if (!d3d11_active || !d3d11_context || !d3d11_target)
        throw std::runtime_error("RSX D3D11 backend was not initialized");
    if (!gcm.ctrl)
        throw std::runtime_error("RSX GCM command buffer was not initialized");

    // Avoid a malicious/broken guest hanging the Xbox UWP UI indefinitely.
    u32 last_jump_from = 0xffffffff;
    u32 last_jump_to = 0xffffffff;
    size_t processed = 0;
    while (gcm.ctrl->get != gcm.ctrl->put) {
        if (++processed > 65536)
            throw std::runtime_error("RSX D3D11 FIFO exceeded the command budget");

        const u32 cmd = fetch32();
        const u32 cmd_num = cmd & 0x3ffff;
        const u32 argc = (cmd >> 18) & 0x7ff;
        if (cmd & 0xa0030003) {
            u32 dest = 0;
            bool jumped = false;
            if ((cmd & 0xe0000003) == 0x20000000) {
                dest = cmd & 0x1ffffffc;
                jumped = true;
            } else if ((cmd & 0xe0000003) == 0x00000001) {
                dest = cmd & 0xfffffffc;
                jumped = true;
            }
            if (jumped) {
                const u32 from = gcm.ctrl->get - 4;
                if (from == last_jump_from && dest == last_jump_to)
                    throw std::runtime_error("RSX D3D11 detected an endless FIFO jump");
                last_jump_from = from;
                last_jump_to = dest;
                gcm.ctrl->get = dest;
                continue;
            }
            throw std::runtime_error("RSX D3D11 call/return FIFO control not implemented");
        }

        if (argc > 2047)
            throw std::runtime_error("Invalid RSX D3D11 command argument count");
        std::vector<u32> args;
        args.reserve(argc);
        for (u32 i = 0; i < argc; ++i)
            args.push_back(fetch32());
        auto first = [&]() -> u32 {
            if (args.empty())
                throw std::runtime_error("RSX D3D11 command missing arguments");
            return args.front();
        };

        switch (cmd_num) {
        case NV4097_NO_OPERATION:
        case NV4097_PIPE_NOP:
            break;
        case NV406E_SET_REFERENCE:
            gcm.ctrl->ref = first();
            break;
        case NV406E_SEMAPHORE_OFFSET:
        case NV4097_SET_SEMAPHORE_OFFSET:
            semaphore_offset = first();
            break;
        case NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE: {
            const u32 val = (first() & 0xff00ff00) |
                ((first() & 0xff) << 16) | ((first() >> 16) & 0xff);
            ps3->mem.write<u32>(gcm.label_addr + semaphore_offset, val);
            break;
        }
        case NV406E_SEMAPHORE_ACQUIRE:
            if (ps3->mem.read<u32>(gcm.label_addr + semaphore_offset) != first())
                throw std::runtime_error("RSX D3D11 semaphore acquire would block");
            break;
        case NV4097_SET_COLOR_CLEAR_VALUE: {
            const u32 color = first();
            clear_color.r() = static_cast<float>((color >> 0) & 0xff) / 255.f;
            clear_color.g() = static_cast<float>((color >> 8) & 0xff) / 255.f;
            clear_color.b() = static_cast<float>((color >> 16) & 0xff) / 255.f;
            clear_color.a() = static_cast<float>((color >> 24) & 0xff) / 255.f;
            break;
        }
        case NV4097_CLEAR_SURFACE: {
            const u32 flags = first();
            if ((flags & 0x03) != 0)
                throw std::runtime_error("RSX D3D11 depth/stencil clear not implemented yet");
            if (flags & 0xf0) {
                const float rgba[] = {
                    clear_color.r(), clear_color.g(), clear_color.b(), clear_color.a()
                };
                d3d11_context->ClearRenderTargetView(d3d11_target, rgba);
            }
            break;
        }
        case NV4097_SET_CONTEXT_DMA_REPORT:
            dma_report = first();
            break;
        case GCM_FLIP_COMMAND:
            (void)first();
            ps3->flip();
            break;
        default:
            // Do not silently drop unsupported draw/shader/texture commands:
            // a successful return here would falsely suggest game rendering.
            Helpers::panic("RSX D3D11 unsupported GPU command 0x%05x\n", cmd_num);
        }
    }
}
#endif
