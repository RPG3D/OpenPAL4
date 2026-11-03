#include "support/native_test_display.h"
#include <windows.h>
#include <rwcore.h>
#include <d3d9.h>
#include "support/renderware_format_probe.h"
#include "pal4/minimap_raster_upload_native.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
#include <set>
#include <stdexcept>
#include <string_view>
#include <vector>

extern "C" {
extern RwInt32 _RwD3D9RasterExtOffset;
extern unsigned char _RwD3D9AdapterInformation[];
RwInt32 __cdecl _rwD3D9ImageFindFormat(const RwImage *);
}
namespace {
void Check(bool b, const char *message) {
    if (!b)
        throw std::runtime_error(message);
}
std::set<RwImage *> images;
void *Born(void *p, RwInt32, RwInt32) {
    images.insert(static_cast<RwImage *>(p));
    return p;
}
void *Died(void *p, RwInt32, RwInt32) {
    images.erase(static_cast<RwImage *>(p));
    return p;
}
struct Session {
    HWND window{};
    Session() {
        Check(RwEngineInit(nullptr, 0, 0), "engine init");
        Check(RwImageRegisterPlugin(0, 0x12347a, Born, Died, nullptr) >= 0, "image lifetime observer");
        window = pal4::test::CreateTestWindow("STATIC", "PAL4 upload", WS_OVERLAPPEDWINDOW, 64, 64, nullptr,
                               nullptr, GetModuleHandleA(nullptr), nullptr);
        Check(window, "window");
        RwEngineOpenParams p{window};
        Check(RwEngineOpen(&p) && RwEngineStart(), "engine start");
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
Image MakeImage(int w = 2, int h = 2, int depth = 32) {
    Image p(RwImageCreate(w, h, depth));
    Check(p && RwImageAllocatePixels(p.get()), "image storage");
    std::memset(p->cpPixels, 0, p->stride * h);
    if (p->palette)
        for (int i = 0; i < (1 << depth); ++i)
            p->palette[i] = {0, 0, 0, 255};
    return p;
}
void Fill(RwImage *image, RwRGBA color) {
    for (int y = 0; y < image->height; ++y)
        for (int x = 0; x < image->width; ++x)
            std::memcpy(image->cpPixels + y * image->stride + 4 * x, &color, 4);
}
struct DisplayDepth {
    RwInt32 saved{};
    explicit DisplayDepth(RwInt32 d) {
        std::memcpy(&saved, _RwD3D9AdapterInformation + 532, 4);
        std::memcpy(_RwD3D9AdapterInformation + 532, &d, 4);
    }
    ~DisplayDepth() { std::memcpy(_RwD3D9AdapterInformation + 532, &saved, 4); }
};
void AlphaFormats() {
    auto image = MakeImage();
    for (int depth : {16, 32}) {
        DisplayDepth display(depth);
        for (unsigned a : {0, 15, 16, 254, 255}) {
            Fill(image.get(), {10, 20, 30, static_cast<RwUInt8>(a)});
            const auto expected = depth == 32 ? (a == 255 ? rwRASTERFORMAT888 : rwRASTERFORMAT8888)
                                              : (a == 255  ? rwRASTERFORMAT565
                                                 : a <= 15 ? rwRASTERFORMAT1555
                                                           : rwRASTERFORMAT4444);
            Check(_rwD3D9ImageFindFormat(image.get()) == expected, "alpha thresholds/display depth");
        }
    }
    DisplayDepth display(32);
    pal4::test::FormatUnsupported denied({D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, D3DFMT_A4R4G4B4});
    for (auto [alpha, expected] : std::array<std::pair<int, int>, 3>{
             {{255, rwRASTERFORMAT565}, {0, rwRASTERFORMAT1555}, {128, rwRASTERFORMAT1555}}}) {
        Fill(image.get(), {10, 20, 30, static_cast<RwUInt8>(alpha)});
        Check(_rwD3D9ImageFindFormat(image.get()) == expected, "denied formats fall back to16-bit");
    }
    Check(denied.denied >= 3, "real capability calls intercepted");
}
void PaletteFormats() {
    for (int depth : {4, 8}) {
        auto image = MakeImage(3, 2, depth);
        auto opaque = (depth == 4 ? rwRASTERFORMATPAL4 : rwRASTERFORMATPAL8) | rwRASTERFORMAT888;
        auto alpha = (depth == 4 ? rwRASTERFORMATPAL4 : rwRASTERFORMATPAL8) | rwRASTERFORMAT8888;
        Check(_rwD3D9ImageFindFormat(image.get()) == opaque, "all palette opaque fast path");
        image->palette[3].alpha = 0;
        Check(_rwD3D9ImageFindFormat(image.get()) == opaque, "unused transparent palette ignored");
        image->cpPixels[image->stride + 2] = 3;
        Check(_rwD3D9ImageFindFormat(image.get()) == alpha, "used transparent palette selects alpha");
        image->palette[3].alpha = 254;
        Check(_rwD3D9ImageFindFormat(image.get()) == alpha, "indexed alpha has no15 threshold");
        image->cpPixels[image->stride + 2] = 0;
        image->cpPixels[3] = 3;
        Check(_rwD3D9ImageFindFormat(image.get()) == opaque, "row padding not scanned as pixels");
    }
}
void PublicFormat() {
    auto image = MakeImage(13, 7);
    Fill(image.get(), {10, 20, 30, 128});
    for (int type : {rwRASTERTYPETEXTURE, rwRASTERTYPECAMERATEXTURE}) {
        int w = -1, h = -1, depth = -1, format = -1;
        Check(RwImageFindRasterFormat(image.get(), type | rwRASTERFORMAT565 | rwRASTERFORMATMIPMAP, &w,
                                      &h, &depth, &format) == image.get(),
              "public image format");
        Check(w == 13 && h == 7 && depth == 32 && (format & 0xf00) == rwRASTERFORMAT8888 &&
                  (format & 7) == type && (format & rwRASTERFORMATMIPMAP),
              "image policy overrides requested pixel format and preserves mip/type");
    }
    int w = 17, h = 18, depth = 19, format = 20;
    Check(!RwImageFindRasterFormat(image.get(), 3, &w, &h, &depth, &format) && w == 17 && h == 18 &&
              depth == 19 && format == 20,
          "invalid raster type leaves public outputs untouched");
}
struct CpuRaster {
    std::vector<RwUInt8> storage;
    std::array<RwUInt8, 24> pixels;
    RwRaster *r;
    CpuRaster(D3DFORMAT format, int depth = 32)
        : storage(_RwD3D9RasterExtOffset + 64), r(::new(storage.data()) RwRaster{}) {
        pixels.fill(0xa7);
        r->parent = r;
        r->width = 2;
        r->height = 2;
        r->depth = depth;
        r->stride = 12;
        r->cType = rwRASTERTYPETEXTURE;
        r->privateFlags = rwRASTERPIXELLOCKEDWRITE;
        r->cpPixels = pixels.data();
        std::memcpy(storage.data() + _RwD3D9RasterExtOffset + 24, &format, 4);
    }
    void Verify(RwUInt32 value) {
        int n = r->depth / 8;
        if (std::memcmp(pixels.data(), &value, n)) {
            RwUInt32 actual = 0, format = 0;
            std::memcpy(&actual, pixels.data(), n);
            std::memcpy(&format, storage.data() + _RwD3D9RasterExtOffset + 24, 4);
            std::fprintf(stderr, "format %u expected %08x actual %08x\n", format, value, actual);
        }
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x)
                Check(!std::memcmp(pixels.data() + y * 12 + x * n, &value, n), "packed upload bytes");
            for (int i = 2 * n; i < 12; ++i)
                Check(pixels[y * 12 + i] == 0xa7, "destination row padding untouched");
        }
    }
};
struct FormatCase {
    D3DFORMAT format;
    RwUInt32 packed;
};
const FormatCase formats[] = {{D3DFMT_X1R5G5B5, 0x573d},
                              {D3DFMT_R5G6B5, 0xae7d},
                              {D3DFMT_A1R5G5B5, 0xd73d},
                              {D3DFMT_A4R4G4B4, 0x8ace},
                              {D3DFMT_A8R3G3B2, 0x80bb},
                              {D3DFMT_X4R4G4B4, 0xface},
                              {D3DFMT_L6V5U5, 0xef35},
                              {D3DFMT_X8R8G8B8, 0xffabcdef},
                              {D3DFMT_X8B8G8R8, 0xffefcdab},
                              {D3DFMT_A8R8G8B8, 0x80abcdef},
                              {D3DFMT_A8B8G8R8, 0x80efcdab},
                              {D3DFMT_V8U8, 0xcdab},
                              {D3DFMT_L8, 99},
                              {D3DFMT_A8L8, 0x80c6},
                              {D3DFMT_L16, 0x629d},
                              {D3DFMT_Q8W8V8U8, 0x80efcdab},
                              {D3DFMT_X8L8V8U8, 0xffefcdab},
                              {D3DFMT_CxV8U8, 0xcdab}};
void PackedConverters() {
    auto image = MakeImage();
    Fill(image.get(), {0xab, 0xcd, 0xef, 0x80});
    for (auto f : formats) {
        CpuRaster raster(f.format);
        Check(RwRasterSetFromImage(raster.r, image.get()) == raster.r, "public upload selector");
        raster.Verify(f.packed);
        Check(raster.r->cpPixels == raster.pixels.data(), "preexisting write lock retained");
    }
    Fill(image.get(), {255, 255, 255, 255});
    CpuRaster white(D3DFMT_L16);
    Check(RwRasterSetFromImage(white.r, image.get()) == white.r, "L16 white upload");
    white.Verify(0xfe01); // Original multiplies the integral luminance by255, not257.
    Fill(image.get(), {0, 3, 2, 129});
    CpuRaster lum16(D3DFMT_L16), lum8(D3DFMT_L8);
    Check(RwRasterSetFromImage(lum16.r, image.get()) == lum16.r &&
              RwRasterSetFromImage(lum8.r, image.get()) == lum8.r,
          "luminance rounding probes");
    lum16.Verify(255);
    lum8.Verify(0); // L8 truncates weighted luminance before applying alpha.
}
void IndexedConverters() {
    for (int depth : {4, 8}) {
        auto image = MakeImage(2, 2, depth);
        image->palette[1] = {0xab, 0xcd, 0xef, 0x80};
        for (int y = 0; y < 2; ++y)
            std::memset(image->cpPixels + y * image->stride, 1, 2);
        for (auto f : formats) {
            CpuRaster raster(f.format, 16);
            Check(RwRasterSetFromImage(raster.r, image.get()) == raster.r,
                  "indexed palette preconversion through selector");
            raster.Verify(f.packed & 0xffff);
        }
    }
}
// These fixtures deliberately supply locked CPU memory. They test the SDK's
// upload dispatch, not device support for legacy palette/byte texture formats.
void ByteFormats() {
    for (int depth : {4, 8, 32}) {
        auto image = MakeImage(2, 2, depth);
        if (depth == 32)
            Fill(image.get(), {0xab, 0xcd, 0xef, 0x80});
        else {
            image->palette[1] = {0xab, 0xcd, 0xef, 0x80};
            for (int y = 0; y < 2; ++y)
                std::memset(image->cpPixels + y * image->stride, 1, 2);
        }
        for (auto [format, value] : std::array<FormatCase, 4>{
                 {{D3DFMT_R3G3B2, 0xbb}, {D3DFMT_A4L4, 0x8c}, {D3DFMT_A8, 0x80}, {D3DFMT_L8, 99}}}) {
            CpuRaster raster(format, 8);
            raster.r->cFormat = rwRASTERFORMATLUM8 >> 8;
            Check(RwRasterSetFromImage(raster.r, image.get()) == raster.r, "byte upload");
            raster.Verify(value);
        }
    }
}
void PaletteCopy() {
    for (int depth : {4, 8}) {
        for (bool contiguous : {false, true}) {
            auto image = MakeImage(2, 2, depth);
            if (contiguous)
                image->stride = 2; // Within the allocated buffer; exercise the bulk-copy path.
            image->palette[1] = {10, 20, 30, 40};
            image->cpPixels[0] = 1;
            image->cpPixels[1] = 2;
            image->cpPixels[image->stride] = 3;
            image->cpPixels[image->stride + 1] = 4;
            CpuRaster raster(D3DFMT_P8, depth);
            std::array<RwUInt8, 1028> palette;
            palette.fill(0xa7);
            raster.r->palette = palette.data();
            raster.r->cFormat =
                (rwRASTERFORMAT8888 | (depth == 4 ? rwRASTERFORMATPAL4 : rwRASTERFORMATPAL8)) >> 8;
            raster.r->privateFlags |= rwRASTERPALETTELOCKEDWRITE;
            if (contiguous)
                raster.r->stride = 2;
            auto *pixelLock = raster.r->cpPixels;
            Check(RwRasterSetFromImage(raster.r, image.get()) == raster.r, "palette copy");
            for (int y = 0; y < 2; ++y)
                Check(!std::memcmp(raster.pixels.data() + y * raster.r->stride,
                                   image->cpPixels + y * image->stride, 2),
                      "byte indices even for PAL4");
            const int paletteBytes = 4 * (1 << depth);
            Check(!std::memcmp(palette.data(), image->palette, paletteBytes), "RGBA palette copied");
            for (int i = paletteBytes; i < static_cast<int>(palette.size()); ++i)
                Check(palette[i] == 0xa7, "palette extent guard");
            if (!contiguous)
                for (int y = 0; y < 2; ++y)
                    for (int x = 2; x < 12; ++x)
                        Check(raster.pixels[y * 12 + x] == 0xa7, "palette row padding");
            Check(raster.r->cpPixels == pixelLock && raster.r->palette == palette.data(),
                  "caller pixel/palette locks retained");
            // +11 is the observed guard; do not infer a semantic field name here.
            raster.storage[_RwD3D9RasterExtOffset + 11] = 1;
            image->palette[1].red = 99;
            Check(RwRasterSetFromImage(raster.r, image.get()) == raster.r && palette[4] == 10,
                  "nonzero extension byte11 suppresses palette copy");
        }
    }
}
void Quantize() {
    for (int depth : {4, 8}) {
        auto image = MakeImage();
        Fill(image.get(), {255, 255, 255, 255});
        CpuRaster raster(D3DFMT_P8, depth);
        std::array<RwUInt8, 1024> palette{};
        raster.r->palette = palette.data();
        raster.r->cFormat =
            (rwRASTERFORMAT8888 | (depth == 4 ? rwRASTERFORMATPAL4 : rwRASTERFORMATPAL8)) >> 8;
        raster.r->privateFlags |= rwRASTERPALETTELOCKEDWRITE;
        const auto before = images;
        Check(RwRasterSetFromImage(raster.r, image.get()) == raster.r, "RGBA palette quantization");
        Check(images == before, "quantizer temporary destroyed");
        for (int y = 0; y < 2; ++y)
            for (int x = 0; x < 2; ++x) {
                const auto index = raster.pixels[y * 12 + x];
                Check(index < (1 << depth), "quantized palette index range");
                const auto *color = palette.data() + 4 * index;
                Check(color[0] == 255 && color[1] == 255 && color[2] == 255 && color[3] == 255,
                      "uniform white quantized color");
            }
        Check((raster.r->cFormat & 15) == (rwRASTERFORMAT888 >> 8) &&
                  raster.storage[_RwD3D9RasterExtOffset + 8] == 0,
              "opaque quantization downgrades alpha pixel format");
    }
}
void Linear() {
    for (int depth : {4, 8, 32}) {
        auto image = MakeImage(2, 2, depth);
        if (depth == 32)
            Fill(image.get(), {0xab, 0xcd, 0xef, 0x80});
        else {
            image->palette[1] = {0xab, 0xcd, 0xef, 0x80};
            for (int y = 0; y < 2; ++y)
                std::memset(image->cpPixels + y * image->stride, 1, 2);
        }
        for (auto f : formats) {
            CpuRaster raster(f.format);
            Check(RwRasterSetFromImage(raster.r, image.get()) == raster.r, "linear indexed/native");
            raster.Verify(f.packed);
        }
        for (auto [rwFormat, packed] : std::array<std::pair<int, RwUInt32>, 2>{
                 {{rwRASTERFORMAT888, 0xffabcdef}, {rwRASTERFORMAT8888, 0x80abcdef}}}) {
            CpuRaster raster(D3DFMT_A8B8G8R8);
            raster.r->cFormat = rwFormat >> 8;
            Check(RwRasterSetFromImage(raster.r, image.get()) == raster.r, "RW format fast path");
            raster.Verify(packed); // RW format takes precedence over native ABGR in this fixture.
        }
    }
    auto image = MakeImage();
    Fill(image.get(), {0xab, 0xcd, 0xef, 0x80});
    CpuRaster nominal24(D3DFMT_A8R8G8B8, 24);
    Check(RwRasterSetFromImage(nominal24.r, image.get()) == nominal24.r, "nominal24 dispatch");
    for (int y = 0; y < 2; ++y) {
        const std::array<RwUInt8, 8> expected{0xef, 0xcd, 0xab, 0x80, 0xef, 0xcd, 0xab, 0x80};
        Check(!std::memcmp(nominal24.pixels.data() + 12 * y, expected.data(), 8),
              "24 branch still writes four bytes per pixel");
        for (int x = 8; x < 12; ++x)
            Check(nominal24.pixels[12 * y + x] == 0xa7, "nominal24 guard");
    }
}
void AlphaParent() {
    auto image = MakeImage();
    Fill(image.get(), {1, 2, 3, 255});
    CpuRaster parent(D3DFMT_X8R8G8B8), child(D3DFMT_A8R8G8B8);
    child.r->parent = parent.r;
    child.r->cFormat = rwRASTERFORMAT888 >> 8;
    Check(RwRasterSetFromImage(child.r, image.get()) == child.r,
          "opaque fast path with alpha native format");
    Check(child.storage[_RwD3D9RasterExtOffset + 8] == 0 &&
              parent.storage[_RwD3D9RasterExtOffset + 8] == 1,
          "parent alpha set before conversion is not cleared when child becomes opaque");
    child.r->cFormat = rwRASTERFORMAT8888 >> 8;
    Check(RwRasterSetFromImage(child.r, image.get()) == child.r &&
              child.storage[_RwD3D9RasterExtOffset + 8] == 1,
          "8888 alpha metadata follows format even with opaque pixels");
}
struct PaletteLockProbe {
    RwStandardFunc pixel = RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERLOCK];
    RwStandardFunc palette = RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERLOCKPALETTE];
    RwStandardFunc unlock = RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERUNLOCK];
    static inline int locks{}, unlocks{}, paletteLocks{};
    static RwBool Pixel(void *out, void *input, RwInt32 flags) {
        auto *r = static_cast<RwRaster *>(input);
        Check(flags == 5, "write nofetch lock flags");
        ++locks;
        *static_cast<RwUInt8 **>(out) = r->cpPixels;
        r->privateFlags |= rwRASTERPIXELLOCKEDWRITE;
        return TRUE;
    }
    static RwBool Palette(void *, void *, RwInt32) {
        ++paletteLocks;
        return FALSE;
    }
    static RwBool Unlock(void *, void *, RwInt32) {
        ++unlocks;
        return TRUE;
    }
    PaletteLockProbe() {
        locks = unlocks = paletteLocks = 0;
        RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERLOCK] = Pixel;
        RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERLOCKPALETTE] = Palette;
        RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERUNLOCK] = Unlock;
    }
    ~PaletteLockProbe() {
        RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERLOCK] = pixel;
        RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERLOCKPALETTE] = palette;
        RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERUNLOCK] = unlock;
    }
};
void PaletteFailure() {
    auto image = MakeImage(4, 4);
    Fill(image.get(), {255, 255, 255, 255});
    CpuRaster raster(D3DFMT_P8, 8);
    raster.r->cFormat = (rwRASTERFORMATPAL8 | rwRASTERFORMAT8888) >> 8;
    raster.r->privateFlags = 0;
    const auto before = images;
    {
        PaletteLockProbe probe;
        Check(!RwRasterSetFromImage(raster.r, image.get()), "palette lock failure propagates");
        Check(probe.locks == 1 && probe.paletteLocks == 1 && probe.unlocks == 0 &&
                  (raster.r->privateFlags & rwRASTERPIXELLOCKEDWRITE),
              "palette failure leaves newly acquired pixel lock held");
    }
    Check(images.size() == before.size() + 1, "palette failure leaks resampled image");
    for (auto *p : std::set<RwImage *>(images))
        if (!before.contains(p))
            RwImageDestroy(p); // Diagnostic cleanup only, not a production behavior change.
}
Raster NewRaster(int w = 2, int h = 2, int format = rwRASTERFORMAT8888) {
    Raster r(RwRasterCreate(w, h, 0, rwRASTERTYPETEXTURE | format));
    Check(r != nullptr, "native raster");
    return r;
}
RwUInt32 ReadPixel(RwRaster *r) {
    auto *p = RwRasterLock(r, 0, rwRASTERLOCKREAD);
    Check(p, "pixel read");
    RwUInt32 v{};
    std::memcpy(&v, p, r->depth / 8);
    RwRasterUnlock(r);
    return v;
}
void NativeUpload() {
    auto source = MakeImage();
    Fill(source.get(), {0xab, 0xcd, 0xef, 0x80});
    for (auto [format, expected] :
         std::array<std::pair<int, RwUInt32>, 4>{{{rwRASTERFORMAT8888, 0x80abcdef},
                                                  {rwRASTERFORMAT888, 0xffabcdef},
                                                  {rwRASTERFORMAT565, 0xae7d},
                                                  {rwRASTERFORMAT1555, 0xd73d}}}) {
        auto r = NewRaster(2, 2, format);
        Check(RwRasterSetFromImage(r.get(), source.get()) == r.get() && !r->cpPixels,
              "native upload unlocks acquired lock");
        Check(ReadPixel(r.get()) == expected, "real GPU packed data");
        auto *locked = RwRasterLock(r.get(), 0, rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH);
        Check(locked && RwRasterSetFromImage(r.get(), source.get()) == r.get() && r->cpPixels == locked,
              "native caller write lock retained");
        RwRasterUnlock(r.get());
    }
}
void Resample() {
    auto source = MakeImage(4, 4);
    Fill(source.get(), {40, 80, 120, 200});
    auto r = NewRaster();
    const auto before = images;
    Check(RwRasterSetFromImage(r.get(), source.get()) == r.get(), "implicit upload resample");
    Check(images == before && ReadPixel(r.get()) == 0xc8285078,
          "resample temporary destroyed, uniform color preserved");
}
RwBool RejectLock(void *, void *, RwInt32) { return FALSE; }
void Failure() {
    auto source = MakeImage(4, 4);
    Fill(source.get(), {40, 80, 120, 200});
    auto r = NewRaster();
    const auto before = images;
    struct Restore {
        RwStandardFunc saved = RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERLOCK];
        Restore() { RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERLOCK] = RejectLock; }
        ~Restore() { RWSRCGLOBAL(stdFunc)[rwSTANDARDRASTERLOCK] = saved; }
    };
    {
        Restore reject;
        Check(!RwRasterSetFromImage(r.get(), source.get()), "lock failure propagates");
    }
    Check(images.size() == before.size() + 1, "failed lock leaks resampled image");
    auto current = images;
    for (auto *p : current)
        if (!before.contains(p))
            RwImageDestroy(p);
    Check(images == before, "diagnostic cleanup of leaked temporary");
    auto same = MakeImage();
    Fill(same.get(), {1, 2, 3, 4});
    CpuRaster compressed(D3DFMT_DXT1);
    compressed.storage[_RwD3D9RasterExtOffset + 10] = 0x10;
    Check(!RwRasterSetFromImage(compressed.r, same.get()),
          "compressed upload rejected before conversion");
    CpuRaster wide(D3DFMT_A16B16G16R16, 64);
    wide.r->stride = 24;
    const auto bytes = wide.pixels;
    Check(RwRasterSetFromImage(wide.r, same.get()) == wide.r && wide.pixels == bytes,
          "64-bit raster silently left unchanged with success");
}
void Game() {
    auto image = MakeImage();
    Fill(image.get(), {0xab, 0xcd, 0xef, 0x80});
    auto raster = NewRaster();
    pal4::NativeMinimapRasterUpload upload;
    pal4::MinimapTextureRecord record{};
    record.image = reinterpret_cast<std::uint32_t>(image.get());
    record.raster = reinterpret_cast<std::uint32_t>(raster.get());
    record.enabled = 1;
    pal4::MinimapTextureState state(record);
    state.CommitRaster(upload);
    Check(state.Record().enabled == 0 && ReadPixel(raster.get()) == 0x80abcdef,
          "game dirty commit reaches real native upload");
    Fill(image.get(), {1, 2, 3, 4});
    state.CommitRaster(upload);
    Check(ReadPixel(raster.get()) == 0x80abcdef, "clean game state does not repeat upload");
    CpuRaster compressed(D3DFMT_DXT1);
    compressed.storage[_RwD3D9RasterExtOffset + 10] = 0x10;
    record.raster = reinterpret_cast<std::uint32_t>(compressed.r);
    pal4::MinimapTextureState failed(record);
    failed.CommitRaster(upload);
    Check(failed.Record().enabled == 0, "game consumes dirty flag even when native upload fails");
}
} // namespace
int main(int argc, char **argv) {
    try {
        Check(argc == 2, "one case");
        {
            Session session;
            const auto before = images;
            const std::string_view name = argv[1];
            if (name == "alpha")
                AlphaFormats();
            else if (name == "palette")
                PaletteFormats();
            else if (name == "public_format")
                PublicFormat();
            else if (name == "converters")
                PackedConverters();
            else if (name == "indexed")
                IndexedConverters();
            else if (name == "native")
                NativeUpload();
            else if (name == "resample")
                Resample();
            else if (name == "failure")
                Failure();
            else if (name == "game")
                Game();
            else if (name == "byte_formats")
                ByteFormats();
            else if (name == "palette_copy")
                PaletteCopy();
            else if (name == "quantize")
                Quantize();
            else if (name == "linear")
                Linear();
            else if (name == "alpha_parent")
                AlphaParent();
            else if (name == "palette_failure")
                PaletteFailure();
            else
                throw std::runtime_error("unknown upload case");
            Check(images == before, "image fixture lifetime");
        }
        Check(images.empty(), "engine image teardown");
        std::puts("PASS");
        return 0;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what());
        return 1;
    }
}
