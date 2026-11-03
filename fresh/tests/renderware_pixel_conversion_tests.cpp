#include "support/native_test_display.h"
#include <windows.h>
#include <rwcore.h>
#include <d3d9.h>
#include <rtbmp.h>
#include <rtfsyst.h>
#include "pal4/renderware_image_native.h"
#include "pal4/screenshot_capture.h"
#include <array>
#include <bit>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <new>
#include <stdexcept>
#include <string_view>
#include <vector>

// Audited SDK data/helper symbols, used only to explore capability branches
// and prelocked CPU raster fixtures. Production uses public image/raster APIs.
extern "C" {
extern D3DCAPS9 _RwD3D9DeviceCaps;
extern RwInt32 _RwD3D9RasterExtOffset;
void __cdecl _rwD3D9CheckRasterSize(RwInt32 *, RwInt32 *, RwInt32);
}
namespace {
void Check(bool b, const char *text) {
    if (!b)
        throw std::runtime_error(text);
}
void Equal(RwRGBA actual, RwRGBA expected) {
    Check(std::memcmp(&actual, &expected, 4) == 0, "exact RGBA bytes");
}
void ClearError() {
    RwError e{};
    RwErrorGet(&e);
}
RwError Error() {
    RwError e{};
    RwErrorGet(&e);
    return e;
}
struct Session {
    HWND window{};
    Session() {
        Check(RwEngineInit(nullptr, 0, 0), "engine init");
        window = pal4::test::CreateTestWindow("STATIC", "PAL4 conversion", WS_OVERLAPPEDWINDOW, 64, 64, nullptr,
                               nullptr, GetModuleHandleA(nullptr), nullptr);
        Check(window, "window");
        RwEngineOpenParams p{window};
        Check(RwEngineOpen(&p) && RwEngineStart(), "engine open/start");
    }
    ~Session() {
        RwEngineStop();
        RwEngineClose();
        RwEngineTerm();
        DestroyWindow(window);
    }
};
struct DropImage {
    void operator()(RwImage *p) const {
        if (p)
            RwImageDestroy(p);
    }
};
struct DropRaster {
    void operator()(RwRaster *p) const {
        if (p)
            RwRasterDestroy(p);
    }
};
using Image = std::unique_ptr<RwImage, DropImage>;
using Raster = std::unique_ptr<RwRaster, DropRaster>;
Image NewImage(int w, int h, int depth = 32) {
    Image i(RwImageCreate(w, h, depth));
    Check(i && RwImageAllocatePixels(i.get()), "allocate image");
    return i;
}
RwRGBA First(RwImage *i) {
    RwRGBA c;
    std::memcpy(&c, i->cpPixels, 4);
    return c;
}
void Pack() {
    RwRGBA input{0xab, 0xcd, 0xef, 0x80};
    const std::array<std::pair<int, RwUInt32>, 8> cases{{{rwRASTERFORMATDEFAULT, 0x80abcdef},
                                                         {rwRASTERFORMAT8888, 0x80abcdef},
                                                         {rwRASTERFORMAT888, 0xffabcdef},
                                                         {rwRASTERFORMAT1555, 0xd73d},
                                                         {rwRASTERFORMAT555, 0x573d},
                                                         {rwRASTERFORMAT565, 0xae7d},
                                                         {rwRASTERFORMAT4444, 0x8ace},
                                                         {rwRASTERFORMATLUM8, 99}}};
    for (auto [format, expected] : cases) {
        Check(RwRGBAToPixel(&input, format) == expected, "public packed pixel");
        RwUInt32 out = 0;
        Check(RWSRCGLOBAL(stdFunc)[rwSTANDARDRGBTOPIXEL](&out, &input, format | rwRASTERFORMATMIPMAP) &&
                  out == expected,
              "standard2 format ignores mip bits");
    }
    ClearError();
    RwUInt32 out = 0xaabbccdd;
    Check(!RWSRCGLOBAL(stdFunc)[rwSTANDARDRGBTOPIXEL](&out, &input, 0x700) && out == 0xaabbccdd,
          "bad pack format preserves output");
    auto error = Error();
    Check(error.errorCode == static_cast<RwInt32>(0x8000000d) && error.pluginID == 0,
          "bad pack format global error");
}
void Unpack() {
    struct Case {
        int format;
        RwUInt32 pixel;
        RwRGBA rgba;
    };
    const Case cases[] = {{rwRASTERFORMAT565, 0xffff, {248, 252, 248, 255}},
                          {rwRASTERFORMAT555, 0xffff, {248, 248, 248, 255}},
                          {rwRASTERFORMAT1555, 0x7fff, {248, 248, 248, 0}},
                          {rwRASTERFORMAT1555, 0xffff, {248, 248, 248, 255}},
                          {rwRASTERFORMAT4444, 0xefff, {240, 240, 240, 224}},
                          {rwRASTERFORMAT4444, 0xffff, {240, 240, 240, 255}},
                          {rwRASTERFORMAT8888, 0x12abcdef, {171, 205, 239, 18}},
                          {rwRASTERFORMAT888, 0x12abcdef, {171, 205, 239, 255}},
                          {rwRASTERFORMATLUM8, 0x12345678, {120, 120, 120, 255}},
                          {rwRASTERFORMATPAL8, 0x12abcdef, {239, 205, 171, 18}}};
    for (const auto &c : cases) {
        RwRGBA out{};
        Check(RwRGBASetFromPixel(&out, c.pixel, c.format) == &out, "public unpack return");
        Equal(out, c.rgba);
    }
    RwRGBA unchanged{1, 2, 3, 4};
    ClearError();
    Check(RwRGBASetFromPixel(&unchanged, 123, rwRASTERFORMATDEFAULT) == &unchanged,
          "public unpack returns destination even when standard fails");
    Equal(unchanged, {1, 2, 3, 4});
    Check(Error().errorCode == static_cast<RwInt32>(0x8000000d), "default unpack error");
    RwUInt32 pixel = 123;
    Check(!RWSRCGLOBAL(stdFunc)[rwSTANDARDPIXELTORGB](&unchanged, &pixel, rwRASTERFORMATDEFAULT),
          "standard3 rejects default format");
    ClearError();
}
void Sizes() {
    struct Restore {
        D3DCAPS9 saved = _RwD3D9DeviceCaps;
        ~Restore() { _RwD3D9DeviceCaps = saved; }
    } restore;
    auto run = [](DWORD caps, int flags, int w, int h, int ew, int eh) {
        _RwD3D9DeviceCaps.TextureCaps = caps;
        _RwD3D9DeviceCaps.MaxTextureWidth = 100;
        _RwD3D9DeviceCaps.MaxTextureHeight = 70;
        _rwD3D9CheckRasterSize(&w, &h, flags);
        Check(w == ew && h == eh, "capability size policy");
    };
    run(0, 0, 120, 90, 100, 70);
    run(D3DPTEXTURECAPS_POW2, 0, 99, 65, 64, 64);
    run(D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_NONPOW2CONDITIONAL, 0, 99, 65, 99, 65);
    run(D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_NONPOW2CONDITIONAL,
        rwRASTERTYPETEXTURE | rwRASTERFORMATMIPMAP, 99, 65, 64, 64);
    run(D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_NONPOW2CONDITIONAL, rwRASTERFORMATMIPMAP, 99, 65, 99, 65);
    run(D3DPTEXTURECAPS_SQUAREONLY, 0, 90, 40, 40, 40);
    run(D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_SQUAREONLY, 0, 90, 40, 32, 32);
    run(D3DPTEXTURECAPS_POW2, 0, 0, 0, static_cast<int>(0x80000000u), static_cast<int>(0x80000000u));
}

// Synthetic prelocked rasters exercise SDK conversion on CPU buffers, including
// formats unavailable on modern GPUs. No fabricated object is destroyed by SDK.
struct CpuRaster {
    std::vector<RwUInt8> storage;
    std::array<RwUInt8, 64> pixels{};
    std::array<RwRGBA, 256> palette{};
    RwRaster *r;
    CpuRaster(int depth, int rwformat, D3DFORMAT format, RwUInt32 value)
        : storage(_RwD3D9RasterExtOffset + 64), r(::new(storage.data()) RwRaster{}) {
        r->parent = r;
        r->width = 2;
        r->height = 2;
        r->depth = depth;
        r->stride = 2 * ((depth + 7) / 8) + 4;
        r->cpPixels = pixels.data();
        r->cFormat = static_cast<RwUInt8>(rwformat >> 8);
        r->privateFlags = rwRASTERPIXELLOCKEDREAD | rwRASTERPALETTELOCKEDREAD;
        std::memcpy(storage.data() + _RwD3D9RasterExtOffset + 24, &format, 4);
        for (int y = 0; y < 2; ++y)
            for (int x = 0; x < 2; ++x)
                std::memcpy(pixels.data() + y * r->stride + x * ((depth + 7) / 8), &value,
                            (depth + 7) / 8);
    }
};
struct CpuImage {
    RwImage image{};
    std::array<RwUInt8, 24> pixels;
    std::array<RwRGBA, 256> palette{};
    CpuImage(int depth = 32) {
        pixels.fill(0xa7);
        image.width = 2;
        image.height = 2;
        image.depth = depth;
        image.stride = 12;
        image.cpPixels = pixels.data();
        image.palette = palette.data();
    }
    void Verify(RwRGBA value) {
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                RwRGBA c;
                std::memcpy(&c, pixels.data() + y * 12 + x * 4, 4);
                Equal(c, value);
            }
            for (int i = 8; i < 12; ++i)
                Check(pixels[y * 12 + i] == 0xa7, "destination row padding untouched");
        }
    }
};
struct FormatCase {
    D3DFORMAT format;
    int depth;
    RwUInt32 pixel;
    RwRGBA expected;
};
const FormatCase formats[] = {{D3DFMT_X1R5G5B5, 16, 0xffff, {248, 248, 248, 255}},
                              {D3DFMT_R5G6B5, 16, 0xffff, {248, 252, 248, 255}},
                              {D3DFMT_L6V5U5, 16, 0xffff, {248, 248, 252, 255}},
                              {D3DFMT_A1R5G5B5, 16, 0xffff, {248, 248, 248, 255}},
                              {D3DFMT_A4R4G4B4, 16, 0xefff, {240, 240, 240, 224}},
                              {D3DFMT_A8R3G3B2, 16, 0x12ff, {224, 224, 192, 18}},
                              {D3DFMT_R3G3B2, 8, 0xff, {224, 224, 192, 255}},
                              {D3DFMT_X4R4G4B4, 16, 0xffff, {240, 240, 240, 255}},
                              {D3DFMT_X8R8G8B8, 32, 0x12abcdef, {171, 205, 239, 255}},
                              {D3DFMT_X8B8G8R8, 32, 0x12abcdef, {239, 205, 171, 255}},
                              {D3DFMT_V8U8, 16, 0x80ff, {255, 128, 0, 255}},
                              {D3DFMT_A8R8G8B8, 32, 0x12abcdef, {171, 205, 239, 18}},
                              {D3DFMT_A8B8G8R8, 32, 0x12abcdef, {239, 205, 171, 18}},
                              {D3DFMT_L8, 8, 0x7d, {125, 125, 125, 255}},
                              {D3DFMT_A8, 8, 0x7d, {255, 255, 255, 125}},
                              {D3DFMT_A4L4, 8, 0xef, {240, 240, 240, 224}},
                              {D3DFMT_A8L8, 16, 0x127d, {125, 125, 125, 18}},
                              {D3DFMT_L16, 16, 0x12ff, {18, 18, 18, 255}},
                              {D3DFMT_Q8W8V8U8, 32, 0x12abcdef, {239, 205, 171, 18}},
                              {D3DFMT_X8L8V8U8, 32, 0x12abcdef, {239, 205, 171, 255}},
                              {D3DFMT_CxV8U8, 16, 0x80ff, {255, 128, 0, 255}},
                              {D3DFMT_A4L4, 8, 0xff, {240, 240, 240, 255}}};
void Fallback() {
    for (const auto &f : formats) {
        CpuRaster r(f.depth, 0, f.format, f.pixel);
        CpuImage out;
        const auto original = r.pixels;
        Check(RwImageSetFromRaster(&out.image, r.r) == &out.image,
              "native D3D format converter dispatch");
        out.Verify(f.expected);
        Check(r.pixels == original && r.r->cpPixels == r.pixels.data(), "prelocked source retained");
    }
}
void RasterFormats() {
    for (auto [rwformat, d3dformat] :
         std::array<std::pair<int, D3DFORMAT>, 7>{{{rwRASTERFORMAT1555, D3DFMT_A1R5G5B5},
                                                   {rwRASTERFORMAT565, D3DFMT_R5G6B5},
                                                   {rwRASTERFORMAT4444, D3DFMT_A4R4G4B4},
                                                   {rwRASTERFORMAT555, D3DFMT_X1R5G5B5},
                                                   {rwRASTERFORMAT8888, D3DFMT_A8R8G8B8},
                                                   {rwRASTERFORMAT888, D3DFMT_X8R8G8B8},
                                                   {rwRASTERFORMAT8888, D3DFMT_A2R10G10B10}}}) {
        const bool wide = rwformat == rwRASTERFORMAT8888 || rwformat == rwRASTERFORMAT888;
        CpuRaster r(wide ? 32 : 16, rwformat, d3dformat, 0xffffffff);
        CpuImage out;
        Check(RwImageSetFromRaster(&out.image, r.r) == &out.image, "RW raster format dispatch");
        const RwRGBA expected = d3dformat == D3DFMT_A2R10G10B10  ? RwRGBA{255, 255, 255, 255}
                                : wide                           ? RwRGBA{255, 255, 255, 255}
                                : rwformat == rwRASTERFORMAT4444 ? RwRGBA{240, 240, 240, 255}
                                : rwformat == rwRASTERFORMAT565  ? RwRGBA{248, 252, 248, 255}
                                                                 : RwRGBA{248, 248, 248, 255};
        out.Verify(expected);
    }
    for (int depth : {4, 8}) {
        CpuRaster r(depth, rwRASTERFORMATLUM8, D3DFMT_L8, 3);
        r.r->palette = reinterpret_cast<RwUInt8 *>(r.palette.data());
        r.palette[3] = {11, 22, 33, 44};
        CpuImage out;
        Check(RwImageSetFromRaster(&out.image, r.r) == &out.image, "byte-per-index palette expansion");
        out.Verify({11, 22, 33, 44});
        r.r->palette = nullptr;
        Check(RwImageSetFromRaster(&out.image, r.r) == &out.image, "no-palette grayscale expansion");
        out.Verify({3, 3, 3, 255});
    }
    CpuRaster lum(8, rwRASTERFORMATLUM8, D3DFMT_L8, 7);
    CpuImage indexed(8);
    Check(RwImageSetFromRaster(&indexed.image, lum.r) == &indexed.image, "luminance indexed copy");
    Check(indexed.pixels[0] == 7 && indexed.pixels[1] == 7 && indexed.pixels[2] == 0xa7 &&
              indexed.pixels[12] == 7,
          "indexed independent row strides");
    Equal(indexed.palette[173], {173, 173, 173, 255});
}
DWORD Probe(RwImage *image, RwRaster *raster) {
    __try {
        RwImageSetFromRaster(image, raster);
        return 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}
void Errors() {
    CpuRaster r(32, rwRASTERFORMAT8888, D3DFMT_A8R8G8B8, 0x12345678);
    CpuImage indexed(4);
    const auto before = indexed.pixels;
    ClearError();
    Check(RwImageSetFromRaster(&indexed.image, r.r) == &indexed.image && indexed.pixels == before,
          "unsupported image depth reports success without copying");
    auto e = Error();
    Check(e.pluginID == 1 && e.errorCode == 2, "unsupported conversion error2");
    r.storage[_RwD3D9RasterExtOffset + 10] = 0x10;
    ClearError();
    Check(!RwImageSetFromRaster(&indexed.image, r.r), "compressed image read explicitly fails");
    Check(Error().errorCode == 2, "compressed conversion error");
    CpuRaster invalid(16, 0, D3DFMT_UNKNOWN, 0);
    CpuImage out;
    Check(Probe(&out.image, invalid.r) == EXCEPTION_ACCESS_VIOLATION,
          "unmapped D3D format calls null converter");
    CpuRaster unsupported(24, 0, D3DFMT_R8G8B8, 0xffffff);
    ClearError();
    Check(RwImageSetFromRaster(&out.image, unsupported.r) == &out.image,
          "unsupported fallback24 reports outer success");
    Check(Error().errorCode == static_cast<RwInt32>(0x8000000c), "fallback depth error");
}
struct Guard {
    unsigned char *p{};
    size_t size{};
    Guard() {
        SYSTEM_INFO i{};
        GetSystemInfo(&i);
        size = i.dwPageSize;
        p = static_cast<unsigned char *>(
            VirtualAlloc(nullptr, 2 * size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        Check(p, "guard allocation");
        DWORD old;
        Check(VirtualProtect(p + size, size, PAGE_NOACCESS, &old), "guard protect");
    }
    ~Guard() { VirtualFree(p, 0, MEM_RELEASE); }
};
void ZeroWidth() {
    CpuRaster r(32, 0, D3DFMT_A8R8G8B8, 0x12345678);
    CpuImage out;
    Guard destination;
    r.r->width = 0;
    r.r->height = 1;
    out.image.width = 0;
    out.image.height = 1;
    out.image.cpPixels = destination.p + destination.size - 4;
    Check(Probe(&out.image, r.r) == EXCEPTION_ACCESS_VIOLATION,
          "zero width executes first pixel then overruns");
    Equal(First(&out.image), {52, 86, 120, 18});
}
void Native() {
    for (auto f : {D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, D3DFMT_R5G6B5, D3DFMT_A1R5G5B5}) {
        Raster r(RwD3D9RasterCreate(4, 2, f, rwRASTERTYPETEXTURE));
        Check(r != nullptr, "actual D3D texture");
        auto *data = RwRasterLock(r.get(), 0, rwRASTERLOCKWRITE);
        Check(data, "GPU write lock");
        std::memset(data, 255, r->stride * r->height);
        RwRasterUnlock(r.get());
        auto image = NewImage(4, 2);
        Check(RwImageSetFromRaster(image.get(), r.get()) == image.get() && !r->cpPixels,
              "readback acquires/releases its own lock");
        Equal(First(image.get()), f == D3DFMT_R5G6B5     ? RwRGBA{248, 252, 248, 255}
                                  : f == D3DFMT_A1R5G5B5 ? RwRGBA{248, 248, 248, 255}
                                                         : RwRGBA{255, 255, 255, 255});
        auto *locked = RwRasterLock(r.get(), 0, rwRASTERLOCKREAD);
        Check(locked, "GPU read lock");
        Check(RwImageSetFromRaster(image.get(), r.get()) == image.get() && r->cpPixels == locked,
              "existing caller read lock retained");
        RwRasterUnlock(r.get());
    }
}
struct ScreenshotRuntime final : pal4::IScreenshotCaptureRuntime {
    RwCamera *camera{};
    std::filesystem::path directory, path;
    const std::string filename = "snapshoot\\2026_10_10__1_2_3.bmp";
    int cleaned{};
    ScreenshotRuntime() {
        directory = std::filesystem::temp_directory_path() /
                    ("pal4-conversion-" + std::to_string(GetCurrentProcessId()));
        Check(std::filesystem::create_directory(directory), "isolated screenshot directory");
        Check(std::filesystem::create_directory(directory / "snapshoot"), "fixture screenshot folder");
        path = directory / filename;
        Check(RtFSManagerOpen(1), "screenshot filesystem manager");
        auto root = directory.string() + "\\";
        char name[] = "snap";
        auto* fs = RtWinFSystemInit(8, root.data(), name);
        Check(fs && RtFSManagerRegister(fs), "screenshot filesystem registration");
        RtFSManagerSetDefaultFileSystem(fs);
    }
    ~ScreenshotRuntime() {
        RtFSManagerClose();
        std::error_code e;
        std::filesystem::remove(path, e);
        std::filesystem::remove(directory / "snapshoot", e);
        std::filesystem::remove(directory, e);
    }
    std::uint32_t InitializeCameraSubsystem() override { return 1; }
    std::uint32_t ActiveCameraObject(std::uint32_t s) override {
        Check(s == 1, "camera subsystem");
        return reinterpret_cast<std::uint32_t>(camera);
    }
    std::uint32_t ReadWord(std::uint32_t p) override {
        std::uint32_t value;
        std::memcpy(&value, reinterpret_cast<void *>(p), 4);
        return value;
    }
    std::uint32_t CurrentTime() override { return 123; }
    pal4::ScreenshotTime LocalTime(std::uint32_t t) override {
        Check(t == 123, "time");
        return {126, 9, 10, 1, 2, 3};
    }
    std::uint32_t InitializeFilename() override { return 456; }
    std::string FormatFilename(std::uint32_t id, const char *format, std::int32_t y, std::int32_t m,
                               std::int32_t d, std::int32_t h, std::int32_t n, std::int32_t s) override {
        Check(id == 456 && std::string_view(format) == "snapshoot\\%d_%d_%d__%d_%d_%d.bmp" &&
                  y == 2026 && m == 10 && d == 10 && h == 1 && n == 2 && s == 3,
              "game filename/date policy");
        return filename;
    }
    std::string CurrentDirectory(std::uint32_t n) override {
        Check(n == 256, "cwd buffer");
        return "fixture";
    }
    bool SetCurrentDirectory(const std::string &d) override {
        Check(d == "snapshoot" || d == "fixture", "directory policy");
        return true;
    }
    void CreateDirectory(const std::string &) override {
        throw std::runtime_error("unexpected create directory");
    }
    void DeleteFile(const std::string &p) override {
        Check(p == filename, "delete path");
        std::error_code e;
        std::filesystem::remove(path, e);
    }
    std::uint32_t CleanupFilename(std::uint32_t id) override {
        Check(id == 456, "filename lifetime");
        ++cleaned;
        return 789;
    }
};
void Game() {
    Check(RwImageRegisterImageFormat("bmp", RtBMPImageRead, RtBMPImageWrite), "BMP codec");
    Raster r(RwRasterCreate(4, 2, 32, rwRASTERTYPETEXTURE | rwRASTERFORMAT8888));
    Check(r != nullptr, "game camera raster");
    auto *data = RwRasterLock(r.get(), 0, rwRASTERLOCKWRITE);
    Check(data, "game raster lock");
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 4; ++x) {
            RwUInt32 color = 0xff315579;
            std::memcpy(data + y * r->stride + 4 * x, &color, 4);
        }
    RwRasterUnlock(r.get());
    RwCamera camera{};
    camera.frameBuffer = r.get();
    ScreenshotRuntime runtime;
    runtime.camera = &camera;
    pal4::NativeRenderWareImageApi api;
    Check(pal4::ScreenshotCapture::Capture(runtime, api) == 789 && runtime.cleaned == 1,
          "real game screenshot orchestration");
    Check(std::filesystem::exists(runtime.path), "game wrote actual BMP file");
    Image file(RtBMPImageRead(runtime.filename.c_str()));
    Check(file && file->width == 4 && file->height == 2, "actual BMP artifact dimensions");
    Equal(First(file.get()), {49, 85, 121, 255});
}
} // namespace
int main(int argc, char **argv) {
    try {
        Check(argc == 2, "one case");
        Session session;
        const std::string_view name = argv[1];
        if (name == "pack")
            Pack();
        else if (name == "unpack")
            Unpack();
        else if (name == "sizes")
            Sizes();
        else if (name == "fallback")
            Fallback();
        else if (name == "raster_formats")
            RasterFormats();
        else if (name == "errors")
            Errors();
        else if (name == "zero_width")
            ZeroWidth();
        else if (name == "native")
            Native();
        else if (name == "game")
            Game();
        else
            throw std::runtime_error("unknown conversion case");
        std::puts("PASS");
        return 0;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
}
