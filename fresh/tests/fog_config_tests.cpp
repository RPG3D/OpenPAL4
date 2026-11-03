#include "pal4/fog_config.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IFogConfigFileRuntime {
public:
    std::uintptr_t Open(std::string_view path,
                        std::string_view mode) override {
        Check(path == "scene.fog" && mode == "r", "open path and mode");
        events.push_back("open");
        return available ? 17 : 0;
    }
    void Read(std::uintptr_t file, void* destination,
              std::uint32_t size, std::uint32_t count) override {
        Check(file == 17 && count == 1 && read_index < fields.size(),
              "read handle and count");
        Check(size == (read_index == 0 ? 1u : 4u), "read field width");
        events.push_back("read" + std::to_string(read_index));
        if (fill)
            std::memcpy(destination, fields[read_index].data(), size);
        ++read_index;
    }
    void Close(std::uintptr_t file) override {
        Check(file == 17, "close handle");
        events.push_back("close");
    }

    bool available = true;
    bool fill = true;
    std::size_t read_index = 0;
    std::vector<std::string> events;
    std::array<std::array<std::uint8_t, 4>, 6> fields{{
        {{1, 0, 0, 0}}, {{2, 0, 0, 0}}, {{3, 0, 0, 0}},
        {{0, 0, 0x80, 0x3f}}, {{0, 0, 0, 0x40}},
        {{0, 0, 0x40, 0x40}}}};
};

std::uint32_t Bits(float value) {
    return std::bit_cast<std::uint32_t>(value);
}

}  // namespace

int main() {
    pal4::FogConfigState state;
    state.active = 77;
    state.enabled = 88;
    state.reserved[0] = 0xaa;
    state.reserved[1] = 0xbb;
    Check(&pal4::FogConfig::Initialize(state) == &state &&
              state.active == 0 && state.enabled == 0 &&
              state.reserved[0] == 0xaa && state.reserved[1] == 0xbb &&
              state.word4 == 0xffffffffu && state.word8 == 1 &&
              Bits(state.float12) == 0x42480000u &&
              Bits(state.float16) == 0x43fa0ccdu &&
              Bits(state.float20) == 0x3a83126fu,
          "constructor defaults and untouched padding");

    state.word4 = 55;
    pal4::FogConfig::ResetWrapper(state);
    Check(state.word4 == 55, "inactive reset wrapper does nothing");
    Check(pal4::FogConfig::EnableWithDefaults(state) == 1 &&
              state.active == 1 && state.word4 == 55,
          "activation on inactive state only sets active byte");
    pal4::FogConfig::Reset(state);
    Check(state.active == 0 && state.word4 == 0xffffffffu &&
              Bits(state.float16) == 0x43fa0000u,
          "active reset uses distinct float default");

    Runtime runtime;
    Check(pal4::FogConfig::Load(state, "scene.fog", runtime),
          "load succeeds after open");
    Check(runtime.events == std::vector<std::string>{
              "open", "read0", "read1", "read2", "read3", "read4",
              "read5", "close"},
          "six reads and close occur in native order");
    Check(state.active == 1 && state.enabled == 1 && state.word4 == 2 &&
              state.word8 == 3 && state.float12 == 1.0f &&
              state.float16 == 2.0f && state.float20 == 3.0f,
          "loaded fields land at native offsets");

    runtime.events.clear();
    runtime.read_index = 0;
    runtime.available = false;
    Check(!pal4::FogConfig::Load(state, "scene.fog", runtime) &&
              runtime.events == std::vector<std::string>{"open"} &&
              state.active == 0 && Bits(state.float16) == 0x43fa0000u,
          "open failure retains reset defaults");

    runtime.events.clear();
    runtime.available = true;
    runtime.fill = false;
    Check(pal4::FogConfig::Load(state, "scene.fog", runtime) &&
              state.enabled == 0 && state.word4 == 0xffffffffu &&
              state.word8 == 1 && state.float12 == 0.0f &&
              state.float16 == 1.0f &&
              Bits(state.float20) == 0x3a83126fu,
          "short reads leave local field defaults intact");
}
