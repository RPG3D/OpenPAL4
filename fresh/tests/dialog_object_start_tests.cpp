#include "pal4/dialog_object.h"

#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {
void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
struct Runtime final : pal4::IDialogObjectStartRuntime {
    std::vector<std::string> calls;
    int random = 0;
    bool resume = false;
    bool prepare = false;
    std::uint8_t expected_flag = 0u;
    int Random15() override {
        calls.emplace_back("random"); return random;
    }
    bool ResumePlayback(pal4::DialogObjectState& state) override {
        Check((state.words[22u] & 0xffu) == expected_flag,
              "selection flag set before resume");
        calls.emplace_back("resume"); return resume;
    }
    bool PreparePlayback(pal4::DialogObjectState& state) override {
        Check((state.words[22u] & 0xffu) == expected_flag,
              "selection flag preserved for prepare");
        calls.emplace_back("prepare"); return prepare;
    }
};
pal4::DialogObjectState State(float probability) {
    pal4::DialogObjectState state{};
    state.words[21u] = std::bit_cast<std::uint32_t>(probability);
    state.words[22u] = 0xaabbccddu;
    return state;
}
}  // namespace

int main() {
    auto state = State(1.0f);
    Runtime runtime;
    runtime.resume = true;
    runtime.expected_flag = 1u;
    Check(pal4::DialogObject::StartPlayback(state, runtime) &&
          state.words[22u] == 0xaabbcc01u &&
          runtime.calls == std::vector<std::string>{"resume"},
          "unit probability avoids RNG and short-circuits on resume");

    state = State(0.0f);
    runtime = Runtime{};
    runtime.expected_flag = 1u;
    Check(!pal4::DialogObject::StartPlayback(state, runtime) &&
          state.words[22u] == 0xaabbcc01u &&
          runtime.calls == std::vector<std::string>{
              "random", "resume", "prepare"},
          "zero probability accepts zero sample but both playback attempts fail");

    state = State(0.0f);
    runtime = Runtime{};
    runtime.random = 1;
    runtime.prepare = true;
    Check(pal4::DialogObject::StartPlayback(state, runtime) &&
          state.words[22u] == 0xaabbcc00u &&
          runtime.calls == std::vector<std::string>{
              "random", "resume", "prepare"},
          "positive sample rejects zero probability and prepare can succeed");

    state = State(0.5f);
    runtime = Runtime{};
    runtime.random = 100;
    runtime.expected_flag = 1u;
    runtime.prepare = true;
    Check(pal4::DialogObject::StartPlayback(state, runtime) &&
          state.words[22u] == 0xaabbcc01u,
          "sample below fractional threshold is selected");
    state = State(0.5f);
    runtime = Runtime{};
    runtime.random = 20000;
    Check(!pal4::DialogObject::StartPlayback(state, runtime) &&
          state.words[22u] == 0xaabbcc00u,
          "sample above fractional threshold is rejected");

    state = State(-1.0f);
    runtime = Runtime{};
    Check(!pal4::DialogObject::StartPlayback(state, runtime) &&
          state.words[22u] == 0xaabbcc00u &&
          runtime.calls.front() == "random",
          "negative probability still draws RNG but never selects");
    state = State(std::numeric_limits<float>::quiet_NaN());
    runtime = Runtime{};
    Check(!pal4::DialogObject::StartPlayback(state, runtime) &&
          state.words[22u] == 0xaabbcc00u &&
          runtime.calls.front() == "random",
          "unordered probability is rejected after RNG");
    state = State(std::numeric_limits<float>::infinity());
    runtime = Runtime{};
    runtime.expected_flag = 1u;
    Check(!pal4::DialogObject::StartPlayback(state, runtime) &&
          state.words[22u] == 0xaabbcc01u,
          "positive infinity accepts finite sample");
}
