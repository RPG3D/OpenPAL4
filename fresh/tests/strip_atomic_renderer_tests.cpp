#include "pal4/strip_atomic_renderer.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IStripAtomicRenderRuntime {
    std::map<std::uint32_t, std::uint32_t> render_states{
        {7u, 70u}, {14u, 140u}, {19u, 190u},
        {20u, 200u}, {27u, 270u}};
    std::uint32_t device_20 = 201u;
    std::uint32_t callback_result = 0x87654321u;
    std::uint32_t callback_atomic{};
    std::vector<std::string> calls;

    void GetRenderState(std::uint32_t state,
                        std::uint32_t& value) override {
        calls.push_back("get:" + std::to_string(state));
        value = render_states.at(state);
    }
    void SetRenderState(std::uint32_t state,
                        std::uint32_t value) override {
        calls.push_back("set:" + std::to_string(state) + ":" +
                        std::to_string(value));
        render_states[state] = value;
    }
    void GetDeviceRenderState(std::uint32_t state,
                              std::uint32_t& value) override {
        Check(state == 20u, "device get uses state 20");
        calls.emplace_back("device_get:20");
        value = device_20;
    }
    void SetDeviceRenderState(std::uint32_t state,
                              std::uint32_t value) override {
        Check(state == 20u, "device set uses state 20");
        calls.push_back("device_set:20:" + std::to_string(value));
        device_20 = value;
    }
    std::uint32_t RenderOriginalAtomic(std::uint32_t atomic) override {
        calls.emplace_back("original");
        callback_atomic = atomic;
        Check(render_states.at(7u) == 1u &&
                  render_states.at(14u) == 0u && device_20 == 1u,
              "original callback sees common overrides");
        return callback_result;
    }
};

void CheckRestored(const Runtime& runtime) {
    Check(runtime.render_states.at(7u) == 70u &&
              runtime.render_states.at(14u) == 140u &&
              runtime.render_states.at(19u) == 190u &&
              runtime.render_states.at(20u) == 200u &&
              runtime.render_states.at(27u) == 270u &&
              runtime.device_20 == 201u,
          "all render and device states restored");
    Check(runtime.callback_atomic == 0x1234u,
          "original callback receives atomic unchanged");
}

void Basic() {
    Runtime runtime;
    const auto result = pal4::StripAtomicRenderer::RenderBasic(0x1234u, runtime);
    Check(result == runtime.callback_result, "basic callback return forwarded");
    Check(runtime.calls == std::vector<std::string>{
        "get:7", "get:14", "set:7:1", "set:14:0",
        "device_get:20", "device_set:20:1", "original",
        "device_set:20:201", "set:7:70", "set:14:140"},
        "basic callback uses native state call order");
    CheckRestored(runtime);
}

void Blended() {
    Runtime runtime;
    runtime.callback_result = 0u;
    const auto result = pal4::StripAtomicRenderer::RenderBlended(
        0x1234u, runtime);
    Check(result == 0u, "zero callback return forwarded");
    Check(runtime.calls == std::vector<std::string>{
        "get:27", "get:19", "get:20", "set:27:1", "set:19:5",
        "set:20:2", "get:7", "get:14", "set:7:1", "set:14:0",
        "device_get:20", "device_set:20:1", "original",
        "device_set:20:201", "set:7:70", "set:14:140",
        "set:27:270", "set:19:190", "set:20:200"},
        "blended callback uses native state call order");
    CheckRestored(runtime);
}

}  // namespace

int main() {
    Basic();
    Blended();
    std::cout << "strip atomic renderer tests passed\n";
}
