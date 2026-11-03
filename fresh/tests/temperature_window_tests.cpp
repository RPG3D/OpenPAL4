#include "pal4/temperature_window.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ITemperatureWindowRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words{
        {0x7024u, 0x8000u}};
    std::vector<std::string> calls;
    std::vector<std::string> names;
    std::uint32_t released = 0u;
    std::uint32_t positioned_window = 0u;
    std::uint32_t metrics_mode = 0u;
    float x = 0.0f;
    bool enabled = true;
    bool throw_lookup = false;

    void LoadLayout(std::uint32_t object,
                    std::uint32_t name) override {
        calls.emplace_back("load");
        Check(object == 0x7000u && name == 0x9000u,
              "layout arguments");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.emplace_back("write");
        words[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    float ReadFloat(std::uint32_t address) override {
        calls.emplace_back("read-float");
        const auto bits = words.at(address);
        if (bits == 0x436b0000u) return 235.0f;
        if (bits == 0x42040000u) return 33.0f;
        throw std::runtime_error("unexpected float bits");
    }
    std::string_view WindowName(std::uint32_t object) override {
        calls.emplace_back("name");
        Check(object == 0x7000u, "window name object");
        return "TemperatureWindow/";
    }
    std::uint32_t GetWindow(std::uint32_t object,
                            std::string_view name) override {
        calls.emplace_back("get-window");
        Check(object == 0x7000u, "window lookup object");
        names.emplace_back(name);
        if (throw_lookup) throw std::runtime_error("lookup");
        return names.size() == 1u ? 0xa000u : 0xa100u;
    }
    void SetEnabled(std::uint32_t window,
                    bool value) override {
        calls.emplace_back("enabled");
        Check(window == 0x8000u, "base window");
        enabled = value;
    }
    std::uint32_t ReleaseNameBuffer() override {
        calls.emplace_back("release-name");
        ++released;
        return 0x55u;
    }
    std::uint32_t SetXPosition(std::uint32_t window,
                               std::uint32_t mode,
                               float value) override {
        calls.emplace_back("set-x");
        positioned_window = window;
        metrics_mode = mode;
        x = value;
        return 0x77u;
    }
};

void CheckConstruction() {
    Runtime runtime;
    Check(pal4::TemperatureWindow::Construct(0x7000u, 0x9000u,
                                              runtime) == 0x7000u &&
              runtime.words.at(0x7000u) ==
                  pal4::TemperatureWindow::kVtable &&
              runtime.words.at(0x702cu) == 0xa000u &&
              runtime.words.at(0x7028u) == 0xa100u &&
              runtime.words.at(0x7030u) == 0x436b0000u &&
              runtime.words.at(0x7034u) == 0x42040000u &&
              runtime.names == (std::vector<std::string>{
                  "TemperatureWindow/ImgDial",
                  "TemperatureWindow/ImgPointer"}) &&
              !runtime.enabled && runtime.released == 1u &&
              runtime.positioned_window == 0xa100u &&
              runtime.metrics_mode == 1u && runtime.x == 33.0f,
          "constructor composes layout, image lookup and initial position");
    Check(runtime.calls == (std::vector<std::string>{
              "load", "write", "name", "get-window", "write",
              "get-window", "write", "write", "write", "read",
              "enabled", "release-name", "read-float", "read-float",
              "read", "set-x"}),
          "constructor operation order");
    Check(pal4::TemperatureWindow::SetPosition(
              0x7000u, 0.5f, runtime) == 0x77u &&
              runtime.x == 150.5f,
          "position interpolation and native return forwarding");
}

void CheckFailures() {
    Runtime runtime;
    runtime.throw_lookup = true;
    bool caught = false;
    try {
        (void)pal4::TemperatureWindow::SetImages(0x7000u, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && runtime.released == 1u &&
              runtime.words.find(0x702cu) == runtime.words.end(),
          "failed image lookup releases temporary name");
    Runtime standalone;
    Check(pal4::TemperatureWindow::SetImages(0x7000u, standalone) ==
              0x55u && standalone.released == 1u,
          "image helper forwards name cleanup result");
}

}  // namespace

int main() {
    try {
        CheckConstruction();
        CheckFailures();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
