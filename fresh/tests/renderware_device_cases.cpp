#include "support/native_test_display.h"
#include "renderware_device_cases.h"
#include <windows.h>
#include <d3d9.h>
#include <rwcore.h>
#include <wrl/client.h>
#include <algorithm>
#include <cstdio>
#include <map>
#include <stdexcept>
#include <tuple>
#include <type_traits>

namespace pal4::test {
namespace {
using Microsoft::WRL::ComPtr;
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
static_assert(std::is_same_v<decltype(&RwD3D9SetRenderTarget), RwBool (__cdecl *)(RwUInt32, RwRaster*)>);
static_assert(std::is_same_v<decltype(&RwD3D9EngineSetMultiSamplingLevels), void (__cdecl *)(RwUInt32)>);
using ModeKey = std::tuple<UINT, UINT, D3DFORMAT>;
RwUInt32 requested_samples{1};
void Modes() {
    ComPtr<IDirect3D9> d3d; d3d.Attach(Direct3DCreate9(D3D_SDK_VERSION));
    Require(d3d != nullptr, "D3D enumeration unavailable");
    const auto adapter = RwEngineGetCurrentSubSystem();
    D3DDISPLAYMODE desktop{};
    Require(SUCCEEDED(d3d->GetAdapterDisplayMode(adapter, &desktop)), "desktop mode query");
    std::map<ModeKey, UINT> expected, actual;
    for (auto format : {D3DFMT_R5G6B5, D3DFMT_X8R8G8B8, D3DFMT_A2R10G10B10}) {
        for (UINT i = 0; i < d3d->GetAdapterModeCount(adapter, format); ++i) {
            D3DDISPLAYMODE mode{};
            Require(SUCCEEDED(d3d->EnumAdapterModes(adapter, format, i, &mode)), "adapter enumeration");
            auto& refresh = expected[{mode.Width, mode.Height, mode.Format}];
            refresh = std::max(refresh, mode.RefreshRate);
        }
    }
    const int count = RwEngineGetNumVideoModes();
    Require(count > 0 && count == static_cast<int>(expected.size()) + 1, "mode dedup count");
    for (int i = 0; i < count; ++i) {
        RwVideoMode mode{};
        Require(RwEngineGetVideoModeInfo(&mode, i) == &mode, "mode info");
        if (i == 0) {
            Require(!(mode.flags & rwVIDEOMODEEXCLUSIVE) && mode.width == desktop.Width &&
                    mode.height == desktop.Height, "desktop windowed entry");
        } else {
            // Public format is an RwRaster format; retain the 16/32/10-bit distinction.
            D3DFORMAT format = mode.format == rwRASTERFORMAT565 ? D3DFMT_R5G6B5 :
                mode.format == rwRASTERFORMAT888 ? D3DFMT_X8R8G8B8 : D3DFMT_A2R10G10B10;
            Require(mode.flags & rwVIDEOMODEEXCLUSIVE, "exclusive mode flag");
            Require(actual.emplace(ModeKey{mode.width, mode.height, format}, mode.refRate).second,
                    "duplicate public mode");
        }
    }
    Require(actual == expected, "mode dimensions/format/maximum refresh mismatch");
    RwVideoMode sentinel{}; sentinel.width = 0x1234;
    Require(!RwEngineGetVideoModeInfo(&sentinel, -1) && sentinel.width == 0x1234, "negative mode info");
    Require(!RwEngineGetVideoModeInfo(&sentinel, count) && sentinel.width == 0x1234, "past-end mode info");
    const auto before = RwEngineGetCurrentVideoMode();
    Require(!RwEngineSetVideoMode(-1) && !RwEngineSetVideoMode(count) &&
            RwEngineGetCurrentVideoMode() == before, "invalid mode changes selection");
    std::printf("device modes: %d (windowed + %zu deduplicated exclusive)\n", count, expected.size());
}
IDirect3DDevice9* Device() {
    auto* device = static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    Require(device != nullptr, "missing started D3D device"); return device;
}
void Present(bool sampling) {
    auto* device = Device();
    ComPtr<IDirect3DSwapChain9> chain;
    Require(SUCCEEDED(device->GetSwapChain(0, &chain)), "swap chain query");
    D3DPRESENT_PARAMETERS p{};
    Require(SUCCEEDED(chain->GetPresentParameters(&p)), "present parameters query");
    Require(p.Windowed && p.BackBufferCount == 1 && p.EnableAutoDepthStencil &&
            p.FullScreen_RefreshRateInHz == 0, "windowed presentation contract");
    Require(p.BackBufferWidth > 0 && p.BackBufferHeight > 0, "runtime client dimensions");
    if (!sampling || requested_samples == 1) {
        Require(p.MultiSampleType == D3DMULTISAMPLE_NONE && p.SwapEffect == D3DSWAPEFFECT_COPY,
                "default nonmultisampled copy swap chain");
    } else {
        Require(p.MultiSampleType != D3DMULTISAMPLE_NONE && p.SwapEffect == D3DSWAPEFFECT_DISCARD,
                "multisampled discard swap chain");
        ComPtr<IDirect3D9> d3d; Require(SUCCEEDED(device->GetDirect3D(&d3d)), "device parent");
        DWORD quality{};
        Require(SUCCEEDED(d3d->CheckDeviceMultiSampleType(RwEngineGetCurrentSubSystem(), D3DDEVTYPE_HAL,
            p.BackBufferFormat, TRUE, p.MultiSampleType, &quality)) && p.MultiSampleQuality < quality,
            "selected multisampling not supported");
    }
    ComPtr<IDirect3DSurface9> color, depth;
    Require(SUCCEEDED(device->GetRenderTarget(0, &color)) &&
            SUCCEEDED(device->GetDepthStencilSurface(&depth)), "initial borrowed surfaces");
    // Start clears the RW target cache even though D3D already owns its backbuffer.
    Require(RwD3D9GetCurrentD3DRenderTarget(0) == nullptr, "initial render target cache not cleared");
    D3DSURFACE_DESC c{}, z{}; color->GetDesc(&c); depth->GetDesc(&z);
    Require(c.Width == p.BackBufferWidth && c.Height == p.BackBufferHeight &&
            z.Width == c.Width && z.Height == c.Height && z.MultiSampleType == c.MultiSampleType,
            "color/depth dimensions and samples");
    Require(!RwEngineSetVideoMode(RwEngineGetCurrentVideoMode()), "SetVideoMode allowed after Start");
    std::printf("device present: %ux%u format=%u depth=%u swap=%u samples=%u quality=%lu requested=%u\n",
                p.BackBufferWidth, p.BackBufferHeight, p.BackBufferFormat, p.AutoDepthStencilFormat,
                p.SwapEffect, p.MultiSampleType, p.MultiSampleQuality, requested_samples);
}
void CacheFailure() {
    auto* device = Device();
    ComPtr<IDirect3DSurface9> before, after;
    Require(SUCCEEDED(device->GetRenderTarget(0, &before)), "initial target");
    struct Target {
        IDirect3DDevice9* device;
        IDirect3DSurface9* original;
        RwRaster* raster = RwRasterCreate(64, 64, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);
        ~Target() {
            // Test-only cleanup restores the borrowed original surface before freeing the raster.
            device->SetRenderTarget(0, original);
            if (raster) RwRasterDestroy(raster);
        }
    } target{device, before.Get()};
    Require(target.raster && RwD3D9SetRenderTarget(0, target.raster), "bind camera texture");
    ComPtr<IDirect3DSurface9> bound;
    Require(SUCCEEDED(device->GetRenderTarget(0, &bound)) &&
            bound.Get() == RwD3D9GetCurrentD3DRenderTarget(0), "successful target cache");
    // D3D9 forbids null target zero. The library caches the request before HRESULT.
    Require(!RwD3D9SetRenderTarget(0, nullptr), "null target zero unexpectedly accepted");
    Require(RwD3D9GetCurrentD3DRenderTarget(0) == nullptr, "failed request not cached");
    Require(SUCCEEDED(device->GetRenderTarget(0, &after)) && after.Get() == bound.Get(),
            "failed call changed actual target");
    Require(RwD3D9SetRenderTarget(0, nullptr), "same failed request did not short circuit");
    after.Reset();
    Require(SUCCEEDED(device->GetRenderTarget(0, &after)) && after.Get() == bound.Get(),
            "cache-hit retry changed actual target");
    // No rendering follows this diagnostic. Stop owns cache cleanup; no private shim.
    std::puts("device cache diagnostic: first=false retry=true cached=null actual=bound texture");
}
void DialogModeProbe() {
    auto* device = Device();
    ComPtr<IDirect3DSwapChain9> chain;
    Require(SUCCEEDED(device->GetSwapChain(0, &chain)), "dialog probe swap chain");
    D3DPRESENT_PARAMETERS present{};
    Require(SUCCEEDED(chain->GetPresentParameters(&present)),
            "dialog probe present parameters");
    const HRESULT result = device->SetDialogBoxMode(TRUE);
    ComPtr<IDirect3DSurface9> backbuffer;
    Require(SUCCEEDED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO,
                                            &backbuffer)),
            "dialog probe backbuffer");
    D3DLOCKED_RECT lock{};
    const HRESULT lock_result = backbuffer->LockRect(&lock, nullptr,
                                                      D3DLOCK_READONLY);
    if (SUCCEEDED(lock_result)) {
        Require(SUCCEEDED(backbuffer->UnlockRect()),
                "dialog probe unlock backbuffer");
    }
    std::printf("dialog mode probe: SetDialogBoxMode=0x%08lX flags=0x%08lX "
                "swap=%u samples=%u format=%u LockRect=0x%08lX\n",
                static_cast<unsigned long>(result),
                static_cast<unsigned long>(present.Flags), present.SwapEffect,
                present.MultiSampleType, present.BackBufferFormat,
                static_cast<unsigned long>(lock_result));
    Require(SUCCEEDED(result), "SDK device rejected post-start dialog mode");
    Require((present.Flags & D3DPRESENTFLAG_LOCKABLE_BACKBUFFER) == 0u &&
                present.SwapEffect == D3DSWAPEFFECT_COPY &&
                FAILED(lock_result),
            "SDK presentation unexpectedly matched original dialog mode");
    Require(SUCCEEDED(device->SetDialogBoxMode(FALSE)),
            "dialog probe restore device mode");

    const HWND second_window = pal4::test::CreateTestWindow(
        "STATIC", "PAL4 dialog creation probe", WS_OVERLAPPEDWINDOW, 64, 64, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    Require(second_window != nullptr, "dialog probe second window");
    ComPtr<IDirect3D9> d3d;
    d3d.Attach(Direct3DCreate9(D3D_SDK_VERSION));
    Require(d3d != nullptr, "dialog probe Direct3DCreate9");
    D3DPRESENT_PARAMETERS original_like{};
    original_like.BackBufferWidth = 64;
    original_like.BackBufferHeight = 64;
    original_like.BackBufferFormat = present.BackBufferFormat;
    original_like.BackBufferCount = 1;
    original_like.MultiSampleType = D3DMULTISAMPLE_NONE;
    original_like.SwapEffect = D3DSWAPEFFECT_DISCARD;
    original_like.hDeviceWindow = second_window;
    original_like.Windowed = TRUE;
    original_like.EnableAutoDepthStencil = TRUE;
    original_like.AutoDepthStencilFormat = present.AutoDepthStencilFormat;
    original_like.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
    ComPtr<IDirect3DDevice9> direct_device;
    const HRESULT create_result = d3d->CreateDevice(
        RwEngineGetCurrentSubSystem(), D3DDEVTYPE_HAL, second_window,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &original_like, &direct_device);
    std::printf("dialog mode direct CreateDevice=0x%08lX flags=0x%08lX "
                "swap=%u\n", static_cast<unsigned long>(create_result),
                static_cast<unsigned long>(original_like.Flags),
                original_like.SwapEffect);
    Require(SUCCEEDED(create_result), "driver rejected original-like device");
    ComPtr<IDirect3DSurface9> direct_backbuffer;
    Require(SUCCEEDED(direct_device->GetBackBuffer(
                0, 0, D3DBACKBUFFER_TYPE_MONO, &direct_backbuffer)),
            "dialog probe direct backbuffer");
    const HRESULT direct_lock = direct_backbuffer->LockRect(
        &lock, nullptr, D3DLOCK_READONLY);
    Require(SUCCEEDED(direct_lock),
            "original-like backbuffer is not lockable");
    Require(SUCCEEDED(direct_backbuffer->UnlockRect()),
            "dialog probe direct unlock");
    direct_backbuffer.Reset();
    direct_device.Reset();
    d3d.Reset();
    Require(DestroyWindow(second_window) != 0, "dialog probe window cleanup");
}
void AdaptedDialogMode() {
    auto* device = Device();
    ComPtr<IDirect3DSwapChain9> chain;
    Require(SUCCEEDED(device->GetSwapChain(0, &chain)),
            "adapted dialog swap chain");
    D3DPRESENT_PARAMETERS present{};
    Require(SUCCEEDED(chain->GetPresentParameters(&present)),
            "adapted dialog present parameters");
    Require((present.Flags & D3DPRESENTFLAG_LOCKABLE_BACKBUFFER) != 0u &&
                present.SwapEffect == D3DSWAPEFFECT_DISCARD &&
                present.MultiSampleType == D3DMULTISAMPLE_NONE,
            "adapted dialog present flags");
    ComPtr<IDirect3DSurface9> backbuffer;
    Require(SUCCEEDED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO,
                                            &backbuffer)),
            "adapted dialog backbuffer");
    D3DLOCKED_RECT lock{};
    Require(SUCCEEDED(backbuffer->LockRect(&lock, nullptr, D3DLOCK_READONLY)),
            "adapted dialog backbuffer lock");
    Require(SUCCEEDED(backbuffer->UnlockRect()),
            "adapted dialog backbuffer unlock");
}
}
void PrepareDeviceCase(std::string_view name) {
    if (name == "device_modes") Modes();
    Require(RwEngineSetVideoMode(0), "select windowed mode");
    requested_samples = name == "device_multisampling" ? RwD3D9EngineGetMaxMultiSamplingLevels() : 1;
    Require(requested_samples >= 1, "multisampling level minimum");
    RwD3D9EngineSetMultiSamplingLevels(requested_samples);
}
void RunDeviceCase(std::string_view name) {
    if (name == "device_modes" || name == "device_present") Present(false);
    else if (name == "device_multisampling") Present(true);
    else if (name == "device_cache_failure") CacheFailure();
    else if (name == "device_dialog_mode_probe") DialogModeProbe();
    else if (name == "device_dialog_mode_hook_probe")
        AdaptedDialogMode();
    else throw std::runtime_error("unknown device case");
}
}
