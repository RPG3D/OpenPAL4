#pragma once
#include "pal4/native_display_target.h"
#include <windows.h>
#include <rwcore.h>
#include <rpworld.h>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace pal4::test {
inline void* AllocateMemory(size_t bytes, RwUInt32) { return std::malloc(bytes); }
inline void FreeMemory(void* memory) { std::free(memory); }
inline void* ReallocateMemory(void* memory, size_t bytes, RwUInt32) {
    return std::realloc(memory, bytes);
}
inline void* CallocateMemory(size_t count, size_t bytes, RwUInt32) {
    return std::calloc(count, bytes);
}

struct NativeFrameSession {
    HWND window = nullptr;
    RwCamera* camera = nullptr;
    RwFrame* frame = nullptr;
    RwRaster* color = nullptr;
    RwRaster* depth = nullptr;
    bool initialized = false;
    bool opened = false;
    bool started = false;
    bool updating = false;

    int width=32, height=32;

    bool Start(int raster_width=32,int raster_height=32) {
        width=raster_width;height=raster_height;
        const RwMemoryFunctions memory{
            AllocateMemory, FreeMemory, ReallocateMemory, CallocateMemory};
        if (!RwEngineInit(&memory, 0, 0)) return false;
        initialized = true;
        if (!RpWorldPluginAttach()) return false;
        const auto display = pal4::SelectNativeDisplayTarget();
        window = CreateWindowA("STATIC", "PAL4 CEGUI probe", WS_OVERLAPPEDWINDOW,
                               display.bounds.left, display.bounds.top,
                               64, 64, nullptr, nullptr,
                               GetModuleHandleA(nullptr), nullptr);
        if (!window) return false;
        MONITORINFOEXA actual{sizeof(actual)};
        if (!GetMonitorInfoA(MonitorFromWindow(window, MONITOR_DEFAULTTONULL),
                             &actual) || display.device != actual.szDevice)
            throw std::runtime_error("PAL4 CEGUI probe opened on the wrong screen");
        RwEngineOpenParams params{window};
        if (!RwEngineOpen(&params)) return false;
        opened = true;
        if (!RwEngineStart()) return false;
        started = true;

        camera = RwCameraCreate();
        frame = RwFrameCreate();
        color = RwRasterCreate(width, height, 32,
            rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);
        depth = RwRasterCreate(width, height, 0, rwRASTERTYPEZBUFFER);
        if (!camera || !frame || !color || !depth) return false;
        RwCameraSetFrame(camera, frame);
        RwCameraSetRaster(camera, color);
        RwCameraSetZRaster(camera, depth);
        RwCameraSetNearClipPlane(camera, 1.0f);
        RwCameraSetFarClipPlane(camera, 10.0f);
        const RwV2d view{1.0f, 1.0f};
        RwCameraSetViewWindow(camera, &view);
        if (!RwCameraBeginUpdate(camera)) return false;
        RwCameraEndUpdate(camera);
        return RwCameraShowRaster(camera, window, 0) != nullptr;
    }

    bool Begin(RwRGBA clear = {0, 0, 0, 255}) {
        if (!RwCameraClear(camera, &clear,
                           rwCAMERACLEARIMAGE | rwCAMERACLEARZ)) return false;
        updating = RwCameraBeginUpdate(camera) != nullptr;
        return updating;
    }

    bool ReadPixel(const int x, const int y, RwRGBA& result) {
        if (updating) {
            RwCameraEndUpdate(camera);
            updating = false;
        }
        if (!RwCameraShowRaster(camera, window, 0)) return false;
        auto* image = RwImageCreate(width, height, 32);
        if (!image) return false;
        const bool ok = RwImageAllocatePixels(image) &&
                        RwImageSetFromRaster(image, color);
        if (ok) {
            std::memcpy(&result, RwImageGetPixels(image) +
                        y * RwImageGetStride(image) + x * 4, sizeof(result));
        }
        RwImageDestroy(image);
        return ok;
    }

    std::vector<std::uint8_t> ReadPixels() {
        if(updating){RwCameraEndUpdate(camera);updating=false;}
        if(!RwCameraShowRaster(camera,window,0))return {};
        std::unique_ptr<RwImage,decltype(&RwImageDestroy)> image(RwImageCreate(width,height,32),&RwImageDestroy);
        if(!image || !RwImageAllocatePixels(image.get()) || !RwImageSetFromRaster(image.get(),color))return {};
        std::vector<std::uint8_t> result(static_cast<std::size_t>(width)*height*4u);
        for(int y=0;y<height;++y)std::memcpy(result.data()+static_cast<std::size_t>(y)*width*4u,
            RwImageGetPixels(image.get())+y*RwImageGetStride(image.get()),static_cast<std::size_t>(width)*4u);
        return result;
    }

    std::uint32_t CountNonBlack() {
        if (updating) {
            RwCameraEndUpdate(camera);
            updating = false;
        }
        if (!RwCameraShowRaster(camera, window, 0)) return 0;
        auto* image = RwImageCreate(width, height, 32);
        if (!image) return 0;
        if (!RwImageAllocatePixels(image) ||
            !RwImageSetFromRaster(image, color)) {
            RwImageDestroy(image);
            return 0;
        }
        std::uint32_t count = 0;
        for (int y = 0; y < height; ++y) {
            const auto* row = reinterpret_cast<const RwRGBA*>(
                RwImageGetPixels(image) + y * RwImageGetStride(image));
            for (int x = 0; x < width; ++x) {
                if (row[x].red || row[x].green || row[x].blue) ++count;
            }
        }
        RwImageDestroy(image);
        return count;
    }

    ~NativeFrameSession() {
        if (updating) RwCameraEndUpdate(camera);
        if (camera) {
            RwCameraSetFrame(camera, nullptr);
            RwCameraSetRaster(camera, nullptr);
            RwCameraSetZRaster(camera, nullptr);
            RwCameraDestroy(camera);
        }
        if (frame) RwFrameDestroy(frame);
        if (color) RwRasterDestroy(color);
        if (depth) RwRasterDestroy(depth);
        if (started) RwEngineStop();
        if (opened) RwEngineClose();
        if (initialized) RwEngineTerm();
        if (window) DestroyWindow(window);
    }
};

}  // namespace pal4::test
