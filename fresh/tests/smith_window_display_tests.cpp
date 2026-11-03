#include "pal4/smith_window.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowDisplayRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0x90u, 0x9000u},
        {object + 0x94u, 0x9400u},
        {object + 0x98u, 0x9800u},
        {object + 0x84u, 0x8400u},
        {object + 0x88u, 0x8800u},
        {object + 0x8cu, 0x8c00u},
        {object + 0x78u, 0x7800u},
    };
    std::vector<std::pair<std::uint32_t, std::uint8_t>> writes;
    std::vector<std::pair<std::uint32_t, bool>> visibility;
    std::vector<std::string> calls;
    bool fail_image = false;

    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        writes.emplace_back(address, value);
        calls.emplace_back("write");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void SetVisible(std::uint32_t window, bool value) override {
        visibility.emplace_back(window, value);
        calls.emplace_back("visible");
    }
    void SetImage(std::uint32_t window, std::string_view imageset,
                  std::string_view image) override {
        Check(window == 0x7800u && imageset == "zi0" &&
                  image == "zhuzao", "Smith image selection");
        calls.emplace_back("image");
        if (fail_image) throw std::runtime_error("image unavailable");
    }
    void HandleEffect(std::uint32_t handle,
                      std::uint32_t event) override {
        Check(handle == object + 0xa4u && event == 1u,
              "effect event arguments");
        calls.emplace_back("effect");
    }
    std::uint32_t UpdateModelPreview(std::uint32_t owner,
                                std::uint32_t first,
                                std::uint32_t second,
                                std::uint32_t third) override {
        Check(owner == object && first == 0u && second == 0u &&
                  third == 0u, "save arguments");
        calls.emplace_back("save");
        return 0x76543210u;
    }
};

void CheckDisplayReset() {
    Runtime runtime;
    Check(pal4::SmithWindow::UpdateDisplay(Runtime::object, runtime) ==
              0x76543210u, "save result forwarding");
    Check(runtime.writes ==
              (std::vector<std::pair<std::uint32_t, std::uint8_t>>{
                  {Runtime::object + 0x81u, 0u},
                  {Runtime::object + 0x80u, 0u}}),
          "selection flags are cleared in machine order");
    Check(runtime.visibility ==
              (std::vector<std::pair<std::uint32_t, bool>>{
                  {0x9000u, false}, {0x9400u, false},
                  {0x9800u, false}, {0x8400u, false},
                  {0x8800u, false}, {0x8c00u, false}}),
          "all six panels hide in machine order");
    Check(runtime.calls ==
              (std::vector<std::string>{
                  "write", "write", "read", "visible",
                  "read", "visible", "read", "visible",
                  "read", "visible", "read", "visible",
                  "read", "visible", "read", "image",
                  "effect", "save"}),
          "panel, image, effect and persistence call order");
}

void CheckImageFailure() {
    Runtime runtime;
    runtime.fail_image = true;
    bool threw = false;
    try {
        (void)pal4::SmithWindow::UpdateDisplay(Runtime::object, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.calls.back() == "image" &&
              runtime.visibility.size() == 6u,
          "image failure stops before effect and save");
}

}  // namespace

int main() {
    try {
        CheckDisplayReset();
        CheckImageFailure();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
