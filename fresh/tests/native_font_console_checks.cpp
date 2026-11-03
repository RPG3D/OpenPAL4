#include <rwcore.h>
#include "pal4/native_pal_font_manager.h"
#include "pal4/native_pal_console_renderer.h"
#include "pal4/pal_console.h"
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
namespace {
void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
}
void VerifyNativeFontConsole(RwCamera* camera) {
// Reuse original PalFontManager algorithms and audited D3DX9 library.
// Isolate readback from scene/UI pixels so a successful call alone is
// insufficient: each original font must actually draw nonblack glyphs.
std::vector<std::uint32_t> font_logs;
pal4::NativePalFontManager native_fonts([&](std::uint32_t line) { font_logs.push_back(line); });
Check(native_fonts.FontCount() == 3u && font_logs == std::vector<std::uint32_t>{21u, 34u},
      "native PalFontManager creates all three original D3DX font slots");
for (unsigned pass = 0; pass < 2u; ++pass) {
    if (pass) { native_fonts.OnLostDevice(); native_fonts.OnResetDevice(); }
    RwRGBA black{0u, 0u, 0u, 255u};
    Check(RwCameraClear(camera, &black, rwCAMERACLEARIMAGE | rwCAMERACLEARZ) &&
              RwCameraBeginUpdate(camera), "begin isolated actual D3DX font GPU frame");
    for (std::uint32_t index = 0; index < 3u; ++index) {
        const auto top = 20 + static_cast<std::int32_t>(index) * 24;
        Check(native_fonts.Draw("PAL4 v1.1", {20, top, 220, top + 20}, index,
                  0xffefefefu, 16u) > 0,
              "original D3DX font draws with flags16 rather than treating it as font size");
    }
    Check(RwCameraEndUpdate(camera), "end isolated actual D3DX font GPU frame");
    auto* raster = RwCameraGetRaster(camera);
    std::unique_ptr<RwImage, decltype(&RwImageDestroy)> pixels(
        RwImageCreate(RwRasterGetWidth(raster), RwRasterGetHeight(raster), 32), &RwImageDestroy);
    Check(pixels && RwImageAllocatePixels(pixels.get()) && RwImageSetFromRaster(pixels.get(), raster),
          "read back actual font render target");
    for (unsigned index = 0; index < 3u; ++index) {
        unsigned glyph_pixels = 0;
        for (unsigned y = 20u + index * 24u; y < 40u + index * 24u; ++y) {
            const auto* row = reinterpret_cast<const RwRGBA*>(RwImageGetPixels(pixels.get()) +
                y * RwImageGetStride(pixels.get()));
            for (unsigned x = 20u; x < 220u; ++x)
                if (row[x].red || row[x].green || row[x].blue) ++glyph_pixels;
        }
        Check(glyph_pixels > 20u, "each native font produces actual glyph pixels before/after lost-reset");
    }
}
pal4::PalConsoleState console_state;
(void)pal4::PalConsole::InitializeRectangles(console_state, 800, 600);
std::memcpy(console_state.Bytes().data() + 0x2cu, ">PAL4", 6u);
std::memcpy(console_state.Bytes().data() + 0x608u, "PAL4 console", 13u);
{
    pal4::NativePalConsoleRenderer console(console_state, native_fonts);
    for (unsigned visible = 0; visible < 2u; ++visible) {
        console_state.WriteDword(0u, visible);
        RwRGBA black{0u, 0u, 0u, 255u};
        Check(RwCameraClear(camera, &black, rwCAMERACLEARIMAGE | rwCAMERACLEARZ) &&
                  RwCameraBeginUpdate(camera), "begin actual console hidden/visible frame");
        console.Render(reinterpret_cast<std::uint32_t>(&camera));
        Check(RwCameraEndUpdate(camera), "end actual console GPU frame");
        auto* raster = RwCameraGetRaster(camera);
        std::unique_ptr<RwImage, decltype(&RwImageDestroy)> pixels(
            RwImageCreate(RwRasterGetWidth(raster), RwRasterGetHeight(raster), 32), &RwImageDestroy);
        Check(pixels && RwImageAllocatePixels(pixels.get()) && RwImageSetFromRaster(pixels.get(), raster),
              "read back actual native console pixels");
        const auto* corner = reinterpret_cast<const RwRGBA*>(RwImageGetPixels(pixels.get()) +
            10 * RwImageGetStride(pixels.get())) + 10;
        Check(visible ? corner->red > 20u : corner->red == 0u,
              "console visibility gates actual translucent background draw");
        if (visible) {
            unsigned green_prompt = 0u, light_output = 0u;
            for (unsigned y = 280u; y < 322u; ++y) {
                const auto* row = reinterpret_cast<const RwRGBA*>(RwImageGetPixels(pixels.get()) +
                    y * RwImageGetStride(pixels.get()));
                for (unsigned x = 0; x < 140u; ++x) {
                    if (y >= 302u && row[x].green > row[x].red + 30u) ++green_prompt;
                    if (y < 300u && row[x].red > 100u) ++light_output;
                }
            }
            Check(green_prompt > 10u && light_output > 10u,
                  "actual console prompt and latest history line draw through native original font");
        }
    }
}
bool invalid_font_rejected = false;
try { (void)native_fonts.Draw("PAL4", {0,0,100,20}, 3u, 0xffffffffu, 16u); }
catch (const std::out_of_range&) { invalid_font_rejected = true; }
Check(invalid_font_rejected, "native font rejects out-of-range font index");
native_fonts.Release(); native_fonts.Release();
Check(!native_fonts.FontCount(), "native font release is repeatable and clears all slots");
std::cout << "Native PalFontManager: original three descriptors, actual D3DX9 glyph pixels, lost/reset and release verified\n";
}
