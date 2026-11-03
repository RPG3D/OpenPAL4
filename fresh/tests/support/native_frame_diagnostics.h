#pragma once

#include <rwcore.h>
#include <d3d9.h>
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include "d3d9_device_forwarder.h"

// SDK-owned data symbol, already used by the independent D3D9 submit probes.
extern "C" IDirect3DDevice9* _RwD3DDevice;

namespace pal4::test {
inline bool DiagnoseNativeFrameNumber(std::uint32_t frame) {
    return frame == 0u || frame == 60u || frame == 180u || frame == 600u;
}
class NativeFramePresentationProbe final : public D3D9DeviceForwarder {
    bool enabled_{};
public:
    NativeFramePresentationProbe() : D3D9DeviceForwarder(_RwD3DDevice),
        enabled_(GetEnvironmentVariableW(L"PAL4_FRAME_DIAGNOSTICS", nullptr, 0u) != 0u) {
        if (enabled_) _RwD3DDevice = this;
    }
    ~NativeFramePresentationProbe() { if (enabled_) _RwD3DDevice = original_; }
    HRESULT STDMETHODCALLTYPE Present(const RECT* source, const RECT* target,
        HWND window, const RGNDATA* dirty) override {
        IDirect3DSurface9* back{};
        IDirect3DSurface9* render{};
        original_->GetRenderTarget(0u, &render);
        if (SUCCEEDED(original_->GetBackBuffer(0u, 0u, D3DBACKBUFFER_TYPE_MONO, &back))) {
            D3DSURFACE_DESC description{}; back->GetDesc(&description);
            D3DLOCKED_RECT lock{};
            const auto locked = back->LockRect(&lock, nullptr, D3DLOCK_READONLY);
            std::uint32_t white{}, black{}, other{};
            if (SUCCEEDED(locked)) {
                for (UINT y = 0; y < description.Height; ++y) {
                    const auto* row = reinterpret_cast<const std::uint32_t*>(
                        static_cast<const std::byte*>(lock.pBits) + y * lock.Pitch);
                    for (UINT x = 0; x < description.Width; ++x) {
                        const auto rgb = row[x] & 0xffffffu;
                        if (rgb == 0xffffffu) ++white;
                        else if (!rgb) ++black;
                        else ++other;
                    }
                }
                back->UnlockRect();
            }
            std::cout << "GPU SDK backbuffer format=" << description.Format
                      << " lock_status=" << locked << " render_is_back=" << (render == back)
                      << " white=" << white << " black=" << black << " other=" << other << std::endl;
            back->Release();
        }
        if (render) render->Release();
        const auto status = original_->Present(source, target, window, dirty);
        std::cout << "GPU SDK Present status=" << status << " override_window=" << window << std::endl;
        return status;
    }
    HRESULT STDMETHODCALLTYPE Reset(D3DPRESENT_PARAMETERS* params) override {
        const auto status = original_->Reset(params);
        std::cout << "GPU SDK Reset status=" << status << std::endl;
        return status;
    }
};
// Status-only observer for failures inside the live presentation call. No readback,
// cooperative reset, or presentation retries; the SDK keeps control of recovery.
class NativeFramePresentationStatusProbe final : public D3D9DeviceForwarder {
    bool enabled_{};
    HRESULT present_{S_OK},reset_{S_OK};
    unsigned presents_{},resets_{};
public:
    NativeFramePresentationStatusProbe() : D3D9DeviceForwarder(_RwD3DDevice),
        enabled_(GetEnvironmentVariableW(L"PAL4_PRESENT_STATUS_DIAGNOSTICS",nullptr,0u)!=0u) {
        if(enabled_)_RwD3DDevice=this;
    }
    ~NativeFramePresentationStatusProbe(){if(enabled_)_RwD3DDevice=original_;}
    HRESULT STDMETHODCALLTYPE Present(const RECT* source,const RECT* target,
        HWND window,const RGNDATA* dirty) override {
        ++presents_;return present_=original_->Present(source,target,window,dirty);
    }
    HRESULT STDMETHODCALLTYPE Reset(D3DPRESENT_PARAMETERS* params) override {
        ++resets_;return reset_=original_->Reset(params);
    }
    void ReportFailure(const char* state,RwCamera* camera) const {
        if(!enabled_)return;
        auto* raster=camera?RwCameraGetRaster(camera):nullptr;
        std::cerr<<"GPU presentation failure state="<<state
            <<" present_calls="<<presents_<<" present_status="<<present_
            <<" reset_calls="<<resets_<<" reset_status="<<reset_
            <<" cooperative="<<original_->TestCooperativeLevel()
            <<" camera="<<camera<<" raster="<<raster
            <<" raster_size="<<(raster?RwRasterGetWidth(raster):0)<<'x'
            <<(raster?RwRasterGetHeight(raster):0)<<std::endl;
    }
};
// Opt-in GPU readback, before presentation. Keep desktop capture separate so
// offscreen rendering and swap-chain/target-window failures can be distinguished.
inline void DiagnoseNativeFrame(RwCamera* camera, const char* label,
                                std::uint32_t sequence) {
    wchar_t directory[32768]{};
    const auto length = GetEnvironmentVariableW(L"PAL4_FRAME_DIAGNOSTICS", directory, 32768u);
    if (!length || length >= 32768u) return;
    auto* raster = camera ? RwCameraGetRaster(camera) : nullptr;
    if (!raster) throw std::runtime_error("frame diagnostic requires camera raster");
    const int width = RwRasterGetWidth(raster), height = RwRasterGetHeight(raster);
    std::unique_ptr<RwImage, decltype(&RwImageDestroy)> image(
        RwImageCreate(width, height, 32), &RwImageDestroy);
    if (!image || !RwImageAllocatePixels(image.get()) ||
        !RwImageSetFromRaster(image.get(), raster))
        throw std::runtime_error("frame diagnostic GPU readback failed");
    const auto path = std::filesystem::path(directory) /
        (std::string(label) + "-" + std::to_string(sequence) + ".ppm");
    std::ofstream output(path, std::ios::binary);
    output << "P6\n" << width << ' ' << height << "\n255\n";
    std::uint32_t white{}, black{}, other{};
    for (int y = 0; y < height; ++y) {
        const auto* row = reinterpret_cast<const RwRGBA*>(
            RwImageGetPixels(image.get()) + y * RwImageGetStride(image.get()));
        for (int x = 0; x < width; ++x) {
            const auto& pixel = row[x];
            const char rgb[]{static_cast<char>(pixel.red), static_cast<char>(pixel.green),
                             static_cast<char>(pixel.blue)};
            output.write(rgb, 3);
            if (pixel.red == 255u && pixel.green == 255u && pixel.blue == 255u) ++white;
            else if (!pixel.red && !pixel.green && !pixel.blue) ++black;
            else ++other;
        }
    }
    if (!output) throw std::runtime_error("frame diagnostic output failed");
    std::cout << "GPU frame " << label << " sequence=" << sequence
              << " size=" << width << 'x' << height << " white=" << white
              << " black=" << black << " other=" << other << std::endl;
}
inline void DiagnoseNativePresentation(RwCamera* camera, HWND window) {
    if (!GetEnvironmentVariableW(L"PAL4_FRAME_DIAGNOSTICS", nullptr, 0u)) return;
    auto* device = static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    if (!device) throw std::runtime_error("frame diagnostic missing D3D9 device");
    IDirect3DSwapChain9* chain{};
    if (FAILED(device->GetSwapChain(0u, &chain)))
        throw std::runtime_error("frame diagnostic missing presentation chain");
    D3DPRESENT_PARAMETERS params{};
    const auto status = chain->GetPresentParameters(&params);
    chain->Release();
    D3DDEVICE_CREATION_PARAMETERS creation{};
    device->GetCreationParameters(&creation);
    IDirect3D9* d3d{};
    HMONITOR adapter_monitor{};
    if (SUCCEEDED(device->GetDirect3D(&d3d))) {
        adapter_monitor = d3d->GetAdapterMonitor(creation.AdapterOrdinal);
        d3d->Release();
    }
    std::cout << "GPU presentation window=" << window
              << " chain_window=" << params.hDeviceWindow
              << " focus_window=" << creation.hFocusWindow
              << " adapter=" << creation.AdapterOrdinal
              << " adapter_monitor=" << adapter_monitor
              << " window_monitor=" << MonitorFromWindow(window, MONITOR_DEFAULTTONULL)
              << " windowed=" << params.Windowed
              << " size=" << params.BackBufferWidth << 'x' << params.BackBufferHeight
              << " raster_type=" << RwRasterGetType(RwCameraGetRaster(camera))
              << " params_status=" << status
              << " cooperative_status=" << device->TestCooperativeLevel() << std::endl;
}
} // namespace pal4::test
