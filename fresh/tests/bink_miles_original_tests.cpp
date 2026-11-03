#include "pal4/native_bink_decoder.h"
#include "pal4/native_bink_package_source.h"
#include "pal4/native_miles_digital_driver.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Check(argc == 2, "provide original game root");
        const std::string root = argv[1];
        pal4::NativeMilesDigitalDriver miles(root);
        Check(miles.Open(), "open original Miles digital driver");
        Check(miles.Open() && miles.Handle() != nullptr,
              "opening Miles twice retains the same driver");

        pal4::NativeBinkDecoder decoder((root + "\\binkw32.dll").c_str());
        Check(decoder.ConfigureMilesSound(miles.Handle()),
              "configure original Bink with Miles driver");
        pal4::NativeBinkPackageSource source(decoder);
        Check(source.Open((root + "\\gamedata\\VideoA.cpk").c_str(),
                          "1A.bik"),
              "open original 1A.bik with Miles audio");
        const auto info = decoder.FrameInfo();
        Check(info.width == 800u && info.height == 600u &&
                  info.frame_count == 855u,
              "original 1A.bik stream metadata");
        std::vector<std::uint8_t> pixels(
            static_cast<std::size_t>(info.width) * info.height * 4u);
        for (int frame = 0; frame < 3; ++frame) {
            Check(decoder.DecodeFrame(pixels), "decode original Bink with Miles");
            Check(decoder.NextFrame(), "advance original Bink with Miles");
        }
        source.Close();
        miles.Close();
        Check(miles.Handle() == nullptr, "close Miles after Bink");
        std::cout << "Original Miles/Bink decoded 3 frames\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
