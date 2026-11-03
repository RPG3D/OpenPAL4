#include "pal4/dialog_object.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IDialogObjectFrameRuntime {
    std::vector<std::string> calls;
    std::size_t rotation = 0u;
    void SetIdentity(std::uint32_t frame) override {
        Check(frame == 0x1000u, "identity frame");
        calls.emplace_back("identity");
    }
    void Scale(std::uint32_t frame,
               const std::array<float, 3u>& value,
               std::uint32_t combine) override {
        Check(frame == 0x1000u && combine == 0u &&
              value == std::array<float, 3u>{2.0f, 3.0f, 4.0f},
              "scale vector and combine mode");
        calls.emplace_back("scale");
    }
    void Rotate(std::uint32_t frame,
                const std::array<float, 3u>& axis,
                float angle, std::uint32_t combine) override {
        const std::array<std::array<float, 3u>, 3u> axes{{
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f}}};
        const std::array<float, 3u> angles{10.0f, 20.0f, 30.0f};
        Check(frame == 0x1000u && combine == 2u && rotation < 3u &&
              axis == axes[rotation] && angle == angles[rotation],
              "native X/Y/Z rotation axes and order");
        calls.emplace_back("rotate:" + std::to_string(rotation++));
    }
    void Translate(std::uint32_t frame,
                   const std::array<float, 3u>& value,
                   std::uint32_t combine) override {
        Check(frame == 0x1000u && combine == 2u &&
              value == std::array<float, 3u>{5.0f, 6.0f, 7.0f},
              "translation vector and combine mode");
        calls.emplace_back("translate");
    }
    int UpdateObjects(std::uint32_t frame) override {
        Check(frame == 0x1000u, "frame object update");
        calls.emplace_back("update"); return 73;
    }
};
}  // namespace

int main() {
    pal4::DialogObjectState state{};
    Runtime runtime;
    Check(pal4::DialogObject::RefreshTransform(state, runtime) == 0 &&
          runtime.calls.empty(), "null frame skips RenderWare calls");
    state.words[43u] = 0x1000u;
    for (std::size_t i = 0u; i < 3u; ++i) {
        state.words[44u + i] = std::bit_cast<std::uint32_t>(5.0f + i);
        state.words[47u + i] = std::bit_cast<std::uint32_t>(2.0f + i);
        state.words[50u + i] = std::bit_cast<std::uint32_t>(10.0f * (i + 1u));
    }
    const auto before = state.words;
    Check(pal4::DialogObject::RefreshTransform(state, runtime) == 73 &&
          runtime.rotation == 3u && state.words == before &&
          runtime.calls == std::vector<std::string>{
              "identity", "scale", "rotate:0", "rotate:1", "rotate:2",
              "translate", "update"},
          "native frame transform pipeline and return value");
}
