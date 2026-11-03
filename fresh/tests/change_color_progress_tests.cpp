#include "pal4/change_color_task.h"
#include "support/color_byte_x87_reference.h"
#include "support/effect_duration_x87_reference.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Bits(const float value) {
    return std::bit_cast<std::uint32_t>(value);
}

struct Runtime final : pal4::IChangeColorProgressRuntime {
    std::vector<std::string> calls;
    bool complete = false;
    std::int32_t random = 0;
    std::array<std::uint8_t, 4u> color{64u, 128u, 192u, 255u};

    bool DurationComplete(const pal4::ChangeColorTaskState&) override {
        calls.emplace_back("duration");
        return complete;
    }
    std::uint32_t ResolveColorFacet(
        const std::uint32_t object,
        const std::string_view class_name) override {
        Check(object == 0x5000u && class_name == "ChangeColorTask",
              "facet lookup uses owner +64 and class name");
        calls.emplace_back("resolve");
        return 0x6000u;
    }
    void SetColorFacetEnabled(const std::uint32_t facet,
                              const std::uint8_t enabled) override {
        Check(facet == 0x6000u && enabled == 1u,
              "facet enable uses owner byte +137");
        calls.emplace_back("enable");
    }
    void ReadColor(const std::uint32_t facet,
                   std::array<std::uint8_t, 4u>& channels) override {
        Check(facet == 0x6000u, "color read uses resolved facet");
        calls.emplace_back("read");
        channels = color;
    }
    std::int32_t RandomInt() override {
        calls.emplace_back("random");
        return random;
    }
};

}  // namespace

int main() {
    pal4::ChangeColorTaskState state{};
    Runtime runtime;
    state.words[16u] = 0x5000u;
    state.words[34u] = 0x101u;
    state.words[5u] = Bits(4.0f);
    state.words[25u] = Bits(0.5f);
    state.words[36u] = 0xaabbcc00u;
    state.words[40u] = 0x12345678u;
    state.words[43u] = 0x11223344u;
    runtime.complete = true;
    Check(pal4::ChangeColorTask::IsComplete(state, runtime) &&
              runtime.calls == std::vector<std::string>{"duration"},
          "nonrollback delegates completion to native duration");
    runtime.calls.clear();
    Check(pal4::ChangeColorTask::InitializeProgress(state, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{
                                   "resolve", "enable", "read"} &&
              state.words[35u] == 0x6000u && state.words[38u] == Bits(4.0f),
          "start resolves facet, enables it, reads color and retains duration");
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        const auto normalized = pal4::test::ColorBytePc24(runtime.color[i]);
        Check(state.words[18u + i] == Bits(normalized) &&
                  state.words[30u + i] == Bits(normalized),
              "start and current colors match normalized input");
        if (i < 3u)
            Check(state.words[22u + i] == Bits(normalized) &&
                      state.words[26u + i] == Bits(0.0f),
                  "alpha mode overwrites destination RGB");
    }
    const auto start_alpha = std::bit_cast<float>(state.words[21u]);
    Check(state.words[29u] == Bits((0.5f - start_alpha) / 4.0f) &&
              state.words[37u] == 0u && state.words[42u] == 0u &&
              state.words[45u] == 0u &&
              state.words[36u] == 0xaabb0000u &&
              state.words[40u] == 0x12345600u &&
              state.words[43u] == 0x11223300u,
          "delta and progress counters reset at native offsets");

    runtime.calls.clear();
    runtime.complete = true;
    state.words[36u] = 0x1u;
    state.words[34u] = 0x101u;
    state.words[39u] = Bits(1.0f);
    runtime.random = 0;
    Check(!pal4::ChangeColorTask::IsComplete(state, runtime) &&
              runtime.calls.empty(),
          "rollback skips duration-complete check");
    Check(pal4::ChangeColorTask::InitializeProgress(state, runtime) == 1u &&
              state.words[38u] == Bits(3.0f) &&
              runtime.calls == std::vector<std::string>{
                                   "resolve", "enable", "read", "random"},
          "rollback randomizes duration with lower bound at random zero");
    state.words[34u]=0x100u;
    state.words[36u]=0u;
    state.words[5u]=Bits(1.f);
    for(unsigned n=0;n<256u;++n) {
        runtime.color.fill(static_cast<std::uint8_t>(n));
        (void)pal4::ChangeColorTask::InitializeProgress(state,runtime);
        for(unsigned channel=0;channel<4u;++channel)
            Check(state.words[18u+channel]==Bits(pal4::test::ColorBytePc24(n)),
                "all starting color bytes agree with original x87 conversion");
    }

    for(const float initial:{0.2f,0.5f,1.f,4.f})
        for(const float bias:{0.1f,0.25f,1.f})
            for(const int sample:{0,33,1572,15687,10985,2002,8923,32767}) {
                runtime.calls.clear();runtime.random=sample;
                state.words[36]=1u;state.words[5]=Bits(initial);
                state.words[39]=Bits(bias);
                (void)pal4::ChangeColorTask::InitializeProgress(state,runtime);
                Check(state.words[38]==Bits(pal4::test::EffectDurationPc24(sample,initial,bias)),
                    "rollback initial duration agrees with original x87 PC24 sequence");
                Check(runtime.calls==std::vector<std::string>{"resolve","enable","read","random"},
                    "initial duration still consumes exactly one supplied sample");
            }

}
