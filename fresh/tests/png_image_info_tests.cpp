#include "pal4/png_image_info.h"

#include <cmath>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {

void Check(bool ok, const char* message) {
    if (ok) return;
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
}

struct Errors final : pal4::IPngImageInfoErrors {
    unsigned calls = 0;
    void* context = nullptr;
    const char* message = nullptr;
    void Emit(void* next_context, const char* next_message) override {
        ++calls;
        context = next_context;
        message = next_message;
    }
};

void TestGuardsAndFlags() {
    int context = 1;
    pal4::PngImageInfo info;
    Errors errors;
    auto original = info;
    Check(pal4::PngImageInfoWriter::SetGamma(nullptr, &info, 2.2) == nullptr,
          "gamma rejects null context");
    pal4::PngImageInfoWriter::SetImage(nullptr, &info, 4, 5, 8, 2, 0, 0, 0, errors);
    pal4::PngImageInfoWriter::SetBackground(nullptr, &info, 7, 8);
    pal4::PngImageInfoWriter::SetSrgbIntent(nullptr, &info, 9);
    Check(pal4::PngImageInfoWriter::SetSrgbGamma(nullptr, &info, 3) == nullptr &&
              std::memcmp(&info, &original, sizeof(info)) == 0 && errors.calls == 0,
          "null context preserves every field");
    pal4::PngImageInfoWriter::SetImage(&context, nullptr, 4, 5, 8, 2, 0, 0, 0, errors);
    Check(pal4::PngImageInfoWriter::SetGamma(&context, nullptr, 2.2) == nullptr &&
              pal4::PngImageInfoWriter::SetSrgbGamma(&context, nullptr, 1) == nullptr,
          "null info is rejected");

    info.valid = 0x10000;
    Check(pal4::PngImageInfoWriter::SetGamma(&context, &info, 2.2) == &info &&
              info.valid == 0x10001 && info.gamma == static_cast<float>(2.2),
          "gamma marks validity and narrows to float");
    pal4::PngImageInfoWriter::SetBackground(&context, &info, 0x11223344, 0x5566);
    Check(info.valid == 0x10009 && info.background == 0x11223344 &&
              info.background_index == 0x5566, "background fields and flag");
    pal4::PngImageInfoWriter::SetSrgbIntent(&context, &info, 7);
    Check(info.valid == 0x10809 && info.srgb_intent == 7,
          "sRGB intent writes high validity byte");
    Check(pal4::PngImageInfoWriter::SetSrgbGamma(&context, &info, 4) == &info &&
              info.srgb_intent == 4 &&
              info.gamma == static_cast<float>(0.4545499980449677) &&
              info.valid == 0x10809,
          "combined sRGB setter writes intent and constant gamma");
}

void TestImageFields() {
    int context = 1;
    Errors errors;
    for (const auto color : {0u, 2u, 3u, 4u, 6u}) {
        pal4::PngImageInfo info;
        pal4::PngImageInfoWriter::SetImage(
            &context, &info, 17, 23, 8, static_cast<std::uint8_t>(color),
            1, 2, 3, errors);
        const unsigned channels = (color == 3 || !(color & 2) ? 1 : 3) +
                                  ((color & 4) != 0);
        Check(info.width == 17 && info.height == 23 && info.bit_depth == 8 &&
                  info.color_type == color && info.interlace == 1 &&
                  info.compression == 2 && info.filter == 3 &&
                  info.channels == channels && info.pixel_depth == channels * 8 &&
                  info.rowbytes == 17 * channels && errors.calls == 0,
              "image geometry and color-channel selection");
    }
    pal4::PngImageInfo packed;
    pal4::PngImageInfoWriter::SetImage(&context, &packed, 9, 2, 1, 0, 0, 0, 0, errors);
    Check(packed.rowbytes == 2 && packed.pixel_depth == 1,
          "sub-byte scanline rounds up");
}

void TestImageErrors() {
    int context = 1;
    Errors errors;
    pal4::PngImageInfo info;
    info.rowbytes = 55;
    pal4::PngImageInfoWriter::SetImage(&context, &info, 0x80000000u, 2,
                                       8, 2, 0, 0, 0, errors);
    Check(info.rowbytes == 0 && errors.calls == 1 && errors.context == &context &&
              std::strcmp(errors.message,
                  "Width too large to process image data; rowbytes will overflow.") == 0,
          "overflow emits exact error and clears rowbytes");
    info.rowbytes = 55;
    bool trapped = false;
    try {
        pal4::PngImageInfoWriter::SetImage(&context, &info, 1, 2,
                                           0, 0, 0, 0, 0, errors);
    } catch (const std::domain_error&) {
        trapped = true;
    }
    Check(trapped && info.rowbytes == 55 && info.pixel_depth == 0 &&
              errors.calls == 1,
          "zero divisor traps before rowbytes or error callback changes");
}

void TestTransparency() {
    int context = 1;
    const std::array<std::uint8_t, 10> color{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    pal4::PngImageInfo info;
    auto original = info;
    pal4::PngImageInfoWriter::SetTransparency(nullptr, &info, 0x1234, 2, &color);
    Check(std::memcmp(&info, &original, sizeof(info)) == 0,
          "null context leaves transparency untouched");
    pal4::PngImageInfoWriter::SetTransparency(&context, nullptr, 0x1234, 2, &color);

    info.transparency_palette_address = 0x4321;
    info.transparency_color.fill(0xaa);
    pal4::PngImageInfoWriter::SetTransparency(&context, &info, 0, 0x10003, nullptr);
    Check(info.valid == 0x10 && info.transparency_palette_address == 0x4321 &&
              info.transparency_count == 3 &&
              info.transparency_color[0] == 0xaa &&
              info.transparency_color[9] == 0xaa,
          "null palette and color preserve old fields; count narrows to word");

    pal4::PngImageInfoWriter::SetTransparency(&context, &info, 0x8765, 0, &color);
    Check(info.valid == 0x10 && info.transparency_palette_address == 0x8765 &&
              info.transparency_count == 1 && info.transparency_color == color,
          "color copy forces zero transparency count to one");
    pal4::PngImageInfoWriter::SetTransparency(&context, &info, 0, 0, nullptr);
    Check(info.transparency_count == 0 && info.transparency_palette_address == 0x8765 &&
              info.transparency_color == color,
          "zero count remains zero without a color source");
}

}  // namespace

int main() {
    TestGuardsAndFlags();
    TestImageFields();
    TestImageErrors();
    TestTransparency();
}
