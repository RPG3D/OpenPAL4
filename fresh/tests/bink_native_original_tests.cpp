#include "pal4/native_bink_decoder.h"
#include "pal4/native_bink_package_source.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void CheckMovie(pal4::NativeBinkPackageSource& source,
                pal4::NativeBinkDecoder& decoder,
                const std::string& archive_path, const char* name) {
    if (!source.Open(archive_path.c_str(), name))
        throw std::runtime_error(std::string("cannot open ") + name +
                                 ": " + decoder.LastError());
    const auto info = decoder.FrameInfo();
    Check(info.width > 0u && info.width <= 4096u &&
          info.height > 0u && info.height <= 4096u &&
          info.frame_count > 1u && info.frame_number == 1u,
          "read original Bink frame dimensions and count");
    std::vector<std::uint8_t> pixels(
        static_cast<std::size_t>(info.width) * info.height * 4u);
    bool visible_frame = false;
    std::uint32_t decoded_frames = 0u;
    for (std::uint32_t attempt = 0u; attempt < 40u; ++attempt) {
        (void)decoder.WaitForFrame();
        Check(decoder.DecodeFrame(pixels), "decode original frame");
        ++decoded_frames;
        for (std::size_t pixel = 0u; pixel < pixels.size(); pixel += 4u) {
            if ((pixels[pixel] | pixels[pixel + 1u] |
                 pixels[pixel + 2u]) != 0u) {
                visible_frame = true;
                break;
            }
        }
        if (decoder.FrameInfo().frame_number == info.frame_count) break;
        Check(decoder.NextFrame(), "advance original Bink movie");
    }
    Check(decoder.FrameInfo().frame_number > 1u,
          "advance beyond original first frame");
    Check(visible_frame, "decoded movie contains nonblack RGB pixels");
    source.Close();
    std::cout << name << ": " << info.width << 'x' << info.height
              << ", " << info.frame_count << " frames, "
              << decoded_frames << " frames decoded\n";
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Check(argc == 2, "provide original game root");
        const std::string root = argv[1];
        pal4::NativeBinkDecoder decoder((root + "\\binkw32.dll").c_str());
        pal4::NativeBinkPackageSource source(decoder);
        const auto archive_path = root + "\\gamedata\\VideoA.cpk";
        CheckMovie(source, decoder, archive_path, "1A.bik");
        CheckMovie(source, decoder, archive_path, "PAL4A.bik");
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
