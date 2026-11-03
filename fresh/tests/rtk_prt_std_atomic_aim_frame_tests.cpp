#include "pal4/rtk_prt_std_atomic_aim_frame.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IRtkPrtStdAtomicAimRuntime {
    std::vector<std::string> calls;
    pal4::RtkPrtStdAtomicAimMatrix matrix{};
    std::array<float, 3> angles{};
    std::uint32_t normalization = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        switch (address) {
        case 0x3070u: return std::bit_cast<std::uint32_t>(1.0f);
        case 0x3074u: return std::bit_cast<std::uint32_t>(2.0f);
        case 0x3078u: return std::bit_cast<std::uint32_t>(3.0f);
        case 0x3064u: return std::bit_cast<std::uint32_t>(2.0f);
        case 0x3068u: return std::bit_cast<std::uint32_t>(3.0f);
        case 0x306cu: return std::bit_cast<std::uint32_t>(4.0f);
        default: Check(false, "resource position or scale address"); return 0u;
        }
    }
    double Normalize(std::array<float, 3>& vector) override {
        const auto length = std::sqrt(static_cast<double>(vector[0]) * vector[0] +
                                      static_cast<double>(vector[1]) * vector[1] +
                                      static_cast<double>(vector[2]) * vector[2]);
        if (length != 0.0)
            for (auto& value : vector)
                value = static_cast<float>(value / length);
        calls.emplace_back("normalize");
        ++normalization;
        return length;
    }
    void UpdateMatrix(pal4::RtkPrtStdAtomicAimMatrix& value) override {
        matrix = value;
        calls.emplace_back("matrix-update");
    }
    std::uint32_t GetFrame(
        const pal4::RtkPrtStdAtomicObjectState&) override {
        calls.emplace_back("frame");
        return 0x4000u;
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == 0x4000u, "identity frame");
        calls.emplace_back("identity");
    }
    void ScaleFrame(std::uint32_t frame,
                    const std::array<std::uint32_t, 3>& scale,
                    std::uint32_t mode) override {
        Check(frame == 0x4000u && mode == 0u &&
                  scale == std::array<std::uint32_t, 3>{
                      std::bit_cast<std::uint32_t>(2.0f),
                      std::bit_cast<std::uint32_t>(3.0f),
                      std::bit_cast<std::uint32_t>(4.0f)},
              "scale uses native resource triple in mode zero");
        calls.emplace_back("scale");
    }
    void ApplyMatrix(std::uint32_t frame,
                     const pal4::RtkPrtStdAtomicAimMatrix& value,
                     std::uint32_t mode) override {
        Check(frame == 0x4000u && mode == 2u &&
                  value.right == matrix.right && value.at == matrix.at,
              "basis matrix applied in mode two");
        calls.emplace_back("apply-matrix");
    }
    void TranslateFrame(std::uint32_t frame,
                        const std::array<std::uint32_t, 3>& position,
                        std::uint32_t mode) override {
        Check(frame == 0x4000u && mode == 2u &&
                  position == std::array<std::uint32_t, 3>{
                      std::bit_cast<std::uint32_t>(1.0f),
                      std::bit_cast<std::uint32_t>(2.0f),
                      std::bit_cast<std::uint32_t>(3.0f)},
              "frame translation uses resource position in mode two");
        calls.emplace_back("translate");
    }
    void UpdateFrameRotation(
        double magnitude, std::uint32_t frame,
        const std::array<std::uint32_t, 3>& offset,
        std::array<float, 3>& output) override {
        Check(magnitude == 1.0 && frame == 0x4000u &&
                  offset == std::array<std::uint32_t, 3>{5u, 6u, 7u},
              "angle update receives up normalization and owner offset");
        output = {11.0f, 22.0f, 33.0f};
        calls.emplace_back("angles");
    }
    std::uint8_t SetRotation(
        pal4::RtkPrtStdAtomicObjectState&,
        const std::array<float, 3>& values,
        std::uint32_t mode) override {
        Check(mode == 0u &&
                  values == std::array<float, 3>{11.0f, 22.0f, 33.0f},
              "rotation facet receives computed angles in absolute mode");
        calls.emplace_back("set-rotation");
        return 0x6eu;
    }
};

void TestAim() {
    pal4::RtkPrtStdAtomicObjectState state{};
    state.words[23u] = 0x3000u;
    state.words[15u] = std::bit_cast<std::uint32_t>(1.0f);
    state.words[16u] = std::bit_cast<std::uint32_t>(2.0f);
    state.words[17u] = std::bit_cast<std::uint32_t>(4.0f);
    state.words[18u] = 5u;
    state.words[19u] = 6u;
    state.words[20u] = 7u;
    state.words[21u] = 0x12345678u;
    Runtime runtime;
    Check(pal4::RtkPrtStdAtomicAimFrame::Update(state, 1u, runtime) == 0u &&
              state.words[21u] == 0x12345678u && runtime.calls.empty(),
          "inactive facet preserves owner and skips native frame work");
    state.words[1u] = 1u;
    Check(pal4::RtkPrtStdAtomicAimFrame::Update(state, 0u, runtime) == 0u &&
              state.words[21u] == 0x12345600u && runtime.calls.empty(),
          "disabled aim writes only owner byte plus 84");
    Check(pal4::RtkPrtStdAtomicAimFrame::Update(state, 1u, runtime) == 0x6eu &&
              state.words[21u] == 0x12345601u &&
              runtime.normalization == 3u &&
              runtime.matrix.right == std::array<float, 3>{1.0f, 0.0f, 0.0f} &&
              runtime.matrix.up == std::array<float, 3>{0.0f, 1.0f, 0.0f} &&
              runtime.matrix.at == std::array<float, 3>{0.0f, 0.0f, 1.0f} &&
              runtime.matrix.flags == 0x20003u &&
              runtime.matrix.position == std::array<float, 3>{0.0f, 0.0f, 0.0f},
          "active aim constructs native 64-byte basis and returns rotation result");
    Check(runtime.calls == std::vector<std::string>{
              "read", "read", "read",
              "normalize", "normalize", "normalize", "matrix-update",
              "frame", "read", "read", "read", "identity", "scale",
              "apply-matrix", "translate", "angles", "set-rotation"},
          "resource reads and all native frame callbacks preserve IDA order");
}

}  // namespace

int main() {
    TestAim();
}
