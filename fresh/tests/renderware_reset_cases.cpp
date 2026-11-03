#include "support/native_test_display.h"
#include "renderware_reset_cases.h"
#include <d3d9.h>
#include <rwcore.h>
#include <wrl/client.h>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <vector>

namespace pal4::test {
namespace {
using Microsoft::WRL::ComPtr;
void Require(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
struct DropImage { void operator()(RwImage* p) const { if (p) RwImageDestroy(p); } };
using Image = std::unique_ptr<RwImage, DropImage>;
struct Camera {
    RwCamera* value = RwCameraCreate();
    RwFrame* frame = RwFrameCreate();
    Camera() { Require(value && frame, "camera/frame creation"); RwCameraSetFrame(value, frame); }
    ~Camera() {
        if (value) {
            // AttachWindow and resize can replace rasters; consult current attachments.
            auto* color = RwCameraGetRaster(value); auto* depth = RwCameraGetZRaster(value);
            RwCameraSetRaster(value, nullptr); RwCameraSetZRaster(value, nullptr);
            RwCameraSetFrame(value, nullptr); RwCameraDestroy(value);
            if (color) RwRasterDestroy(color);
            if (depth) RwRasterDestroy(depth);
        }
        if (frame) RwFrameDestroy(frame);
    }
    void Raster(int width, int height, int type, bool depth = false) {
        auto* raster = RwRasterCreate(width, height, 0, type);
        Require(raster != nullptr, "camera raster creation"); RwCameraSetRaster(value, raster);
        if (depth) {
            auto* z = RwRasterCreate(width, height, 0, rwRASTERTYPEZBUFFER);
            Require(z != nullptr, "depth raster creation"); RwCameraSetZRaster(value, z);
        }
    }
};
Image Read(RwRaster* raster) {
    Image image(RwImageCreate(RwRasterGetWidth(raster), RwRasterGetHeight(raster), 32));
    Require(image && RwImageAllocatePixels(image.get()) && RwImageSetFromRaster(image.get(), raster),
            "camera clear readback"); return image;
}
void CheckPixel(RwImage* image, int x, int y, RwRGBA expected) {
    auto* p = reinterpret_cast<RwRGBA*>(RwImageGetPixels(image) + y * RwImageGetStride(image)) + x;
    Require(p->red == expected.red && p->green == expected.green && p->blue == expected.blue,
            "clear region pixel mismatch");
}
void ClearRegions() {
    Camera camera; camera.Raster(32, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);
    auto* parent = RwCameraGetRaster(camera.value);
    RwRGBA base{19, 37, 73, 255}, inset{201, 103, 41, 255};
    Require(RwCameraClear(camera.value, &base, rwCAMERACLEARIMAGE) == camera.value, "full texture clear");
    auto* child = RwRasterCreate(0, 0, 0, rwRASTERTYPECAMERATEXTURE | rwRASTERDONTALLOCATE);
    Require(child != nullptr, "subraster creation");
    struct Child {
        RwCamera* camera; RwRaster* parent; RwRaster* value;
        ~Child() { RwCameraSetRaster(camera, parent); RwRasterDestroy(value); }
    } cleanup{camera.value, parent, child};
    RwRect rect{5, 7, 8, 9}; Require(RwRasterSubRaster(child, parent, &rect) == child, "subraster attach");
    RwCameraSetRaster(camera.value, child);
    Require(RwCameraClear(camera.value, &inset, rwCAMERACLEARIMAGE) == camera.value, "subraster clear");
    // With no camera Z raster the texture path strips both depth and stencil bits.
    Require(RwCameraClear(camera.value, nullptr, rwCAMERACLEARZ | rwCAMERACLEARSTENCIL) == camera.value,
            "missing depth clear flags");
    auto image = Read(parent);
    for (int y = 0; y < 32; ++y) for (int x = 0; x < 32; ++x)
        CheckPixel(image.get(), x, y, x >= 5 && x < 13 && y >= 7 && y < 16 ? inset : base);
}
std::vector<int> events;
rwD3D9DeviceReleaseCallBack previous_release{};
rwD3D9DeviceRestoreCallBack previous_restore{};
void Released() { if (previous_release) previous_release(); events.push_back(1); }
void Restored() { if (previous_restore) previous_restore(); events.push_back(2); }
struct Callbacks {
    Callbacks() {
        events.clear(); previous_release = _rwD3D9DeviceGetReleaseCallback();
        previous_restore = _rwD3D9DeviceGetRestoreCallback();
        _rwD3D9DeviceSetReleaseCallback(Released); _rwD3D9DeviceSetRestoreCallback(Restored);
    }
    ~Callbacks() {
        _rwD3D9DeviceSetReleaseCallback(previous_release);
        _rwD3D9DeviceSetRestoreCallback(previous_restore);
    }
};
void Resize(HWND window, bool hold_backbuffer) {
    RECT initial{}; GetClientRect(window, &initial);
    Camera camera; camera.Raster(initial.right, initial.bottom, rwRASTERTYPECAMERA, true);
    RwRGBA color{23, 45, 67, 255};
    Require(RwCameraClear(camera.value, &color, rwCAMERACLEARIMAGE | rwCAMERACLEARZ) == camera.value,
            "initial camera clear");
    Callbacks callbacks;
    auto* device = static_cast<IDirect3DDevice9*>(RwD3D9GetCurrentD3DDevice());
    ComPtr<IDirect3DSurface9> held;
    if (hold_backbuffer)
        Require(SUCCEEDED(device->GetRenderTarget(0, &held)), "retain reset-blocking backbuffer reference");
    Require(SetWindowPos(window, nullptr, 0, 0, 240, 180, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE),
            "hidden window resize");
    RECT resized{}; GetClientRect(window, &resized);
    Require(resized.right != initial.right && resized.bottom != initial.bottom, "client dimensions unchanged");
    if (hold_backbuffer) {
        Require(!RwCameraClear(camera.value, &color, rwCAMERACLEARIMAGE | rwCAMERACLEARZ),
                "reset succeeded with outstanding backbuffer reference");
        Require(events == std::vector<int>({1}), "failed reset invoked restore callback");
        held.Reset();
        const auto cooperative = device->TestCooperativeLevel();
        std::printf("after failed reset: cooperative=%08lX\n", cooperative);
        Require(cooperative == D3DERR_DEVICENOTRESET, "failed-reset diagnostic precondition");
    }
    Require(RwCameraClear(camera.value, &color, rwCAMERACLEARIMAGE | rwCAMERACLEARZ) == camera.value,
            "clear-triggered reset failed");
    const auto expected = hold_backbuffer ? std::vector<int>{1, 1, 2} : std::vector<int>{1, 2};
    Require(events == expected, "release/restore callback order");
    ComPtr<IDirect3DSwapChain9> chain; Require(SUCCEEDED(device->GetSwapChain(0, &chain)), "resized swapchain");
    D3DPRESENT_PARAMETERS p{}; Require(SUCCEEDED(chain->GetPresentParameters(&p)), "resized present params");
    std::printf("reset result: backbuffer=%ux%u raster=%dx%d client=%ldx%ld\n",
                p.BackBufferWidth, p.BackBufferHeight, RwRasterGetWidth(RwCameraGetRaster(camera.value)),
                RwRasterGetHeight(RwCameraGetRaster(camera.value)), resized.right, resized.bottom);
    // On retry from DEVICENOTRESET, Clear skips copying client dimensions to
    // Present and resets using the old fallback size. It still caches the new
    // client size. Preserve this observed original/SDK divergence as a diagnostic.
    const auto expected_width = hold_backbuffer ? initial.right : resized.right;
    const auto expected_height = hold_backbuffer ? initial.bottom : resized.bottom;
    Require(p.Windowed && p.BackBufferWidth == expected_width && p.BackBufferHeight == expected_height,
            "reset dimension contract");
    Require(RwRasterGetWidth(RwCameraGetRaster(camera.value)) == expected_width &&
            RwRasterGetHeight(RwCameraGetRaster(camera.value)) == expected_height, "camera raster dimensions");
    Require(RwCameraClear(camera.value, &color, rwCAMERACLEARIMAGE) == camera.value,
            "subsequent same-client clear");
    Require(events == expected, "same-client clear retriggered resize");
    Require(RwCameraBeginUpdate(camera.value) == camera.value && RwCameraEndUpdate(camera.value) == camera.value,
            "post-reset BeginScene/camera update");
    Require(events == expected, "stable update unexpectedly reset device");
    std::printf("camera reset: %ldx%ld -> %ldx%ld failed_attempt=%d releases=%u restores=1\n",
                initial.right, initial.bottom, resized.right, resized.bottom, hold_backbuffer,
                hold_backbuffer ? 2u : 1u);
}
struct Window {
    HWND value = pal4::test::CreateTestWindow("STATIC", "PAL4 additional camera", WS_OVERLAPPEDWINDOW, 200, 150, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    ~Window() { if (value) DestroyWindow(value); }
};
void Attach(HWND engine_window) {
    Window window; Require(window.value != nullptr, "secondary hidden window");
    Camera camera; camera.Raster(16, 16, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888, true);
    auto* original = RwCameraGetRaster(camera.value);
    Require(!RwD3D9CameraAttachWindow(camera.value, engine_window) &&
            RwCameraGetRaster(camera.value) == original, "main-window rejection mutated camera");
    Require(RwD3D9CameraAttachWindow(camera.value, window.value), "additional swapchain attach");
    RECT client{}; GetClientRect(window.value, &client);
    Require(RwCameraGetRaster(camera.value) && RwCameraGetZRaster(camera.value), "attached camera resources");
    Require(RwRasterGetWidth(RwCameraGetRaster(camera.value)) == client.right &&
            RwRasterGetHeight(RwCameraGetRaster(camera.value)) == client.bottom, "attached client dimensions");
    RwRGBA color{113, 79, 31, 255};
    Require(RwCameraClear(camera.value, &color, rwCAMERACLEARIMAGE | rwCAMERACLEARZ) == camera.value,
            "additional camera clear");
    auto image = Read(RwCameraGetRaster(camera.value)); CheckPixel(image.get(), 0, 0, color);
    Require(SetWindowPos(window.value, nullptr, 0, 0, 260, 190, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE),
            "additional window resize");
    GetClientRect(window.value, &client);
    Require(RwCameraClear(camera.value, &color, rwCAMERACLEARIMAGE) == camera.value, "additional chain recreation");
    Require(RwRasterGetWidth(RwCameraGetRaster(camera.value)) == client.right &&
            RwRasterGetHeight(RwCameraGetRaster(camera.value)) == client.bottom, "additional raster resize");
}
void ModeNoOp() {
    Callbacks callbacks;
    auto* device = RwD3D9GetCurrentD3DDevice();
    const auto current = RwEngineGetCurrentVideoMode();
    Require(RwD3D9ChangeVideoMode(-1) && RwD3D9ChangeVideoMode(RwEngineGetNumVideoModes()) &&
            RwD3D9ChangeVideoMode(current), "ChangeVideoMode no-op return contract");
    Require(RwEngineGetCurrentVideoMode() == current && RwD3D9GetCurrentD3DDevice() == device && events.empty(),
            "no-op mode call reset resources");
}
}
void RunResetCase(std::string_view name, HWND engine_window) {
    if (name == "reset_clear_regions") ClearRegions();
    else if (name == "reset_resize") Resize(engine_window, false);
    else if (name == "reset_resize_failure") Resize(engine_window, true);
    else if (name == "reset_attach") Attach(engine_window);
    else if (name == "reset_mode_noop") ModeNoOp();
    else throw std::runtime_error("unknown reset case");
}
}
