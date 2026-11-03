#include "pal4/change_color_task.h"
#include "support/effect_duration_x87_reference.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
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

void SetFloat(pal4::ChangeColorTaskState& state,
              const std::uint32_t offset, const float value) {
    state.words[offset / 4u] = Bits(value);
}

float FloatAt(const pal4::ChangeColorTaskState& state,
              const std::uint32_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}

std::uint8_t ByteAt(const pal4::ChangeColorTaskState& state,
                    const std::uint32_t offset) {
    return static_cast<std::uint8_t>(state.words[offset / 4u] >>
                                     (8u * (offset % 4u)));
}

void SetByte(pal4::ChangeColorTaskState& state,
             const std::uint32_t offset, const std::uint8_t value) {
    const auto shift = 8u * (offset % 4u);
    const auto mask = 0xffu << shift;
    auto& word = state.words[offset / 4u];
    word = (word & ~mask) | (static_cast<std::uint32_t>(value) << shift);
}

struct Write final {
    std::uint32_t facet;
    std::array<std::uint8_t, 4u> channels;
};

struct Runtime final : pal4::IChangeColorAnimationRuntime {
    std::vector<Write> writes;
    std::uint32_t random_calls = 0u;
    std::int32_t random = 0;

    std::uint32_t WriteColor(
        const std::uint32_t facet,
        const std::array<std::uint8_t, 4u>& channels) override {
        writes.push_back({facet, channels});
        return 0xabcd0055u;
    }
    std::int32_t RandomInt() override {
        ++random_calls;
        return random;
    }
};

void TestOrdinaryProgress() {
    pal4::ChangeColorTaskState state{};
    Runtime runtime;
    state.words[35u] = 0x6000u;
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        SetFloat(state, 120u + 4u * i,
                 std::array<float, 4u>{0.0f, 0.25f, 0.5f, 1.0f}[i]);
        SetFloat(state, 104u + 4u * i,
                 std::array<float, 4u>{0.25f, 0.25f, -0.25f, -0.25f}[i]);
    }
    Check(pal4::ChangeColorTask::UpdateAnimation(state, 1.0f, runtime) ==
              0xabcd0001u && runtime.writes.size() == 1u &&
              runtime.writes[0u].facet == 0x6000u &&
              runtime.writes[0u].channels ==
                  std::array<std::uint8_t, 4u>{64u, 128u, 64u, 191u} &&
              FloatAt(state, 120u) == 0.25f &&
              FloatAt(state, 132u) == 0.75f &&
              runtime.random_calls == 0u,
          "nonrollback integrates four colors and retains callback EAX upper bytes");

    runtime.writes.clear();
    SetFloat(state, 120u, -0.5f);
    SetFloat(state, 104u, 0.0f);
    SetFloat(state, 124u, 2.0f);
    SetFloat(state, 108u, 0.0f);
    Check((pal4::ChangeColorTask::UpdateAnimation(state, 0.0f, runtime) &
           0xffu) == 1u &&
              runtime.writes[0u].channels[0u] == 129u &&
              runtime.writes[0u].channels[1u] == 254u,
          "native conversion truncates signed integers before taking low byte");
}

void TestRollbackWaits() {
    pal4::ChangeColorTaskState state{};
    Runtime runtime;
    state.words[35u] = 0x6000u;
    SetByte(state, 144u, 1u);
    SetByte(state, 160u, 1u);
    SetFloat(state, 164u, 1.0f);
    SetFloat(state, 168u, 0.25f);
    Check(pal4::ChangeColorTask::UpdateAnimation(state, 0.5f, runtime) ==
              0x101u && FloatAt(state, 168u) == 0.75f &&
              runtime.writes.empty(),
          "first wait checks prior timer and consumes frame");
    SetFloat(state, 168u, 1.0f);
    SetByte(state, 172u, 1u);
    SetFloat(state, 176u, 2.0f);
    SetFloat(state, 180u, 0.5f);
    Check(pal4::ChangeColorTask::UpdateAnimation(state, 0.25f, runtime) ==
              0x101u && ByteAt(state, 160u) == 0u &&
              FloatAt(state, 168u) == 0.0f &&
              FloatAt(state, 180u) == 0.75f && runtime.writes.empty(),
          "expired first wait clears and second wait takes precedence");
}

void TestRollbackEndpoints() {
    pal4::ChangeColorTaskState state{};
    Runtime runtime;
    state.words[35u] = 0x6000u;
    SetByte(state, 144u, 1u);
    SetFloat(state, 20u, 2.0f);
    SetFloat(state, 152u, 1.0f);
    SetFloat(state, 156u, 0.5f);
    SetFloat(state, 148u, 0.75f);
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        SetFloat(state, 72u + 4u * i, 0.0f);
        SetFloat(state, 88u + 4u * i, 1.0f);
        SetFloat(state, 104u + 4u * i, 1.0f);
        SetFloat(state, 120u + 4u * i, 0.75f);
    }
    Check(pal4::ChangeColorTask::UpdateAnimation(state, 0.5f, runtime) ==
              0x3f800001u && runtime.writes.size() == 2u &&
              runtime.writes[0u].channels ==
                  std::array<std::uint8_t, 4u>{255u, 255u, 255u, 255u} &&
              runtime.writes[1u].channels ==
                  std::array<std::uint8_t, 4u>{255u, 255u, 255u, 255u} &&
              ByteAt(state, 145u) == 1u && ByteAt(state, 160u) == 1u &&
              FloatAt(state, 148u) == 0.0f &&
              FloatAt(state, 168u) == 0.0f &&
              FloatAt(state, 104u) == -1.0f &&
              FloatAt(state, 120u) == 1.0f &&
              runtime.random_calls == 0u,
          "first endpoint snaps end color, reverses slopes and starts wait");

    runtime.writes.clear();
    SetByte(state, 160u, 0u);
    SetFloat(state, 148u, 0.75f);
    Check(pal4::ChangeColorTask::UpdateAnimation(state, 0.5f, runtime) ==
              1u && runtime.writes.size() == 2u &&
              runtime.writes[0u].channels ==
                  std::array<std::uint8_t, 4u>{191u, 191u, 191u, 191u} &&
              runtime.writes[1u].channels ==
                  std::array<std::uint8_t, 4u>{0u, 0u, 0u, 0u} &&
              ByteAt(state, 145u) == 0u && ByteAt(state, 172u) == 1u &&
              FloatAt(state, 152u) == 1.5f &&
              FloatAt(state, 180u) == 0.0f &&
              FloatAt(state, 120u) == 0.0f &&
              FloatAt(state, 104u) > 0.66f &&
              runtime.random_calls == 1u,
          "second endpoint snaps start, rerolls duration once and resumes forward");
}

void TestFinish() {
    pal4::ChangeColorTaskState state{};
    Runtime runtime;
    state.words[35u] = 0x6000u;
    const std::array<float, 4u> end{0.0f, 0.25f, 0.5f, 1.0f};
    for (std::uint32_t i = 0u; i < 4u; ++i)
        SetFloat(state, 88u + 4u * i, end[i]);
    Check(pal4::ChangeColorTask::FinishAnimation(state, runtime) == 1u &&
              runtime.writes.size() == 1u &&
              runtime.writes[0u].facet == 0x6000u &&
              runtime.writes[0u].channels ==
                  std::array<std::uint8_t, 4u>{0u, 64u, 128u, 255u} &&
              state.words[35u] == 0u,
          "finish writes end RGBA then clears facet handle");
}

void TestRollbackDurationPc24() {
    for(const float initial:{0.2f,0.5f,1.f,4.f})
        for(const float bias:{0.1f,0.25f,1.f})
            for(const int sample:{0,33,1572,15687,10985,2002,8923,32767}) {
                pal4::ChangeColorTaskState state{};Runtime runtime;
                state.words[35]=0x6000u;SetByte(state,144,1);SetByte(state,145,1);
                SetFloat(state,20,initial);SetFloat(state,156,bias);
                SetFloat(state,152,1.f);SetFloat(state,148,1.f);runtime.random=sample;
                (void)pal4::ChangeColorTask::UpdateAnimation(state,0.1f,runtime);
                Check(state.words[38]==Bits(pal4::test::EffectDurationPc24(sample,initial,bias)),
                    "rollback reroll duration agrees with original x87 PC24 sequence");
                Check(runtime.random_calls==1u && runtime.writes.size()==2u,
                    "reroll consumes one supplied sample and retains endpoint writes");
            }
}

}  // namespace

int main() {
    TestOrdinaryProgress();
    TestRollbackWaits();
    TestRollbackEndpoints();
    TestFinish();
    TestRollbackDurationPc24();
}
