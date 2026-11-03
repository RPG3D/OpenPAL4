#include "pal4/role_static_elements.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IRoleStaticElementsRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, float> floats;
    std::vector<std::string> names;
    std::vector<std::string> formatted_names;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> width_windows;
    std::uint32_t prefix_read = 0u;
    std::uint32_t prefix_released = 0u;
    std::uint32_t throw_on_window = 0u;

    std::string ReadPrefix(std::uint32_t address) override {
        calls.emplace_back("prefix");
        prefix_read = address;
        return "Status/";
    }
    std::string ConvertToUtf8(
        const std::string& formatted_name) override {
        calls.emplace_back("convert");
        formatted_names.push_back(formatted_name);
        return formatted_name;
    }
    std::uint32_t GetWindow(const std::string& name) override {
        calls.emplace_back("window");
        names.push_back(name);
        if (names.size() == throw_on_window)
            throw std::runtime_error("window lookup failed");
        return 0x5000u +
            static_cast<std::uint32_t>(names.size() - 1u) * 0x100u;
    }
    float GetWidth(std::uint32_t window) override {
        calls.emplace_back("width");
        width_windows.push_back(window);
        return static_cast<float>(width_windows.size() * 10u);
    }
    std::uint32_t CleanupFormatBuffer() override {
        calls.emplace_back("cleanup");
        return 0xabcdu;
    }
    std::uint32_t ReleasePrefix(std::uint32_t address) override {
        calls.emplace_back("release");
        prefix_released = address;
        return 0xfeedu;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write-word");
        words[address] = value;
    }
    void WriteFloat(std::uint32_t address,
                    float value) override {
        calls.emplace_back("write-float");
        floats[address] = value;
    }
};

void CheckCreate(bool has_prefix, std::int32_t role) {
    Runtime runtime;
    const auto prefix = has_prefix ? 0x9000u : 0u;
    const auto returned = pal4::RoleStaticElements::Create(
        0x1000u, 0x1111u, prefix, 0x3333u, 0x4444u,
        role, runtime);
    const auto start = has_prefix ? std::string{"Status/"} :
                                    std::string{};
    const auto number = std::to_string(role);
    Check(runtime.names == std::vector<std::string>{
              start + "StaticHp" + number,
              start + "StaticMp" + number,
              start + "StaticRage" + number,
              start + "ImgHp" + number,
              start + "ImgMp" + number,
              start + "ImgRage" + number,
              start + "BtnRole" + number,
              start + "PanelRole" + number,
              start + "ImgRole" + number},
          "nine exact window names in lookup order");
    Check(runtime.formatted_names == runtime.names,
          "each formatted name is converted before lookup");
    constexpr std::uint32_t offsets[]{
        4u, 8u, 12u, 16u, 20u, 24u, 28u, 0u, 32u};
    for (std::uint32_t index = 0u; index < 9u; ++index)
        Check(runtime.words.at(0x1000u + offsets[index]) ==
                  0x5000u + index * 0x100u,
              "window field mapping");
    Check(runtime.words.at(0x5000u + 6u * 0x100u + 280u) ==
              static_cast<std::uint32_t>(role),
          "button role index");
    Check(runtime.width_windows ==
              std::vector<std::uint32_t>{0x5300u, 0x5400u, 0x5500u} &&
          runtime.floats.at(0x1024u) == 10.0f &&
          runtime.floats.at(0x1028u) == 20.0f &&
          runtime.floats.at(0x102cu) == 30.0f,
          "image widths stored in three float fields");
    Check(runtime.calls[runtime.calls.size() -
                        (has_prefix ? 2u : 1u)] == "cleanup" &&
          runtime.calls.back() ==
              (has_prefix ? "release" : "cleanup") &&
          returned == (has_prefix ? 0xfeedu : 0xabcdu),
          "cleanup and prefix lifetime order");
    Check(runtime.prefix_read == prefix &&
          runtime.prefix_released == prefix,
          "prefix ownership boundary");
}

void CheckExceptionalCleanup(bool has_prefix) {
    Runtime runtime;
    runtime.throw_on_window = 3u;
    bool caught = false;
    try {
        (void)pal4::RoleStaticElements::Create(
            0x1000u, 0u, has_prefix ? 0x9000u : 0u,
            0u, 0u, 1, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.names.size() == 3u &&
          runtime.calls[runtime.calls.size() -
                        (has_prefix ? 2u : 1u)] == "cleanup" &&
          runtime.calls.back() ==
              (has_prefix ? "release" : "cleanup") &&
          runtime.prefix_released ==
              (has_prefix ? 0x9000u : 0u),
          "lookup exception releases format buffer and copied prefix");
}

}  // namespace

int main() {
    try {
        CheckCreate(false, 0);
        CheckCreate(true, 3);
        CheckCreate(true, -2);
        CheckExceptionalCleanup(false);
        CheckExceptionalCleanup(true);
        std::cout << "role_static_elements_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
