#include "pal4/combat_console_ui_initializer.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatConsoleUiRuntime {
    std::uint32_t allocation = 0xa000u;
    std::vector<std::string> calls;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 0x44u, "font element size");
        calls.emplace_back("allocate");
        return allocation;
    }
    std::uint32_t ConstructFontElement(
        std::uint32_t memory) override {
        Check(memory == allocation, "font element address");
        calls.emplace_back("construct");
        return memory + 4u;
    }
    void SetDimensions(std::uint32_t element,
                       float width, float height) override {
        Check(element == (allocation == 0u ? 0u : allocation + 4u) &&
                  width == 400.0f && height == 280.0f,
              "dimensions and target");
        calls.emplace_back("dimensions");
    }
    void SetImageset(std::uint32_t element,
                     std::string_view imageset,
                     std::string_view image, float scale) override {
        Check(element == (allocation == 0u ? 0u : allocation + 4u) &&
                  imageset == "zi0" && image == "zhangdoushengli" &&
                  scale == 1.0f, "imageset arguments");
        calls.emplace_back("imageset");
    }
    void SetFont(std::uint32_t element,
                 std::string_view text,
                 std::string_view font, float scale) override {
        Check(element == (allocation == 0u ? 0u : allocation + 4u) &&
                  text.empty() && font == "system" && scale == 1.0f,
              "font arguments");
        calls.emplace_back("font");
    }
    void SetFontProperties(std::uint32_t element,
                           float first, float second) override {
        Check(element == (allocation == 0u ? 0u : allocation + 4u) &&
                  first == 2.0f && second == 3.0f,
              "font properties");
        calls.emplace_back("font-properties");
    }
    std::uint32_t ElementList() override {
        calls.emplace_back("list");
        return 0xb000u;
    }
    void AddElement(std::uint32_t list,
                    bool constructed,
                    std::uint32_t element) override {
        Check(list == 0xb000u &&
                  constructed == (allocation != 0u) &&
                  element == (allocation == 0u ? 0u : allocation + 4u),
              "list insertion arguments");
        calls.emplace_back("add");
    }
};

void BothAllocationPaths() {
    for (const auto allocation : {0xa000u, 0u}) {
        Runtime runtime;
        runtime.allocation = allocation;
        Check(pal4::CombatConsoleUiInitializer::Initialize(runtime) == 1u,
              "initializer return byte");
        const std::vector<std::string> expected = allocation == 0u ?
            std::vector<std::string>{"allocate", "dimensions", "imageset",
                "font", "font-properties", "list", "add"} :
            std::vector<std::string>{"allocate", "construct", "dimensions",
                "imageset", "font", "font-properties", "list", "add"};
        Check(runtime.calls == expected,
              "construction, resource and registration order");
    }
}

}  // namespace

int main() {
    try {
        BothAllocationPaths();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
