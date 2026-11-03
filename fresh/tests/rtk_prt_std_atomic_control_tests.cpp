#include "pal4/rtk_prt_std_atomic_control.h"
#include "support/color_byte_x87_reference.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint8_t Byte(const pal4::RtkPrtStdAtomicObjectState& state,
                  std::uint32_t offset) {
    return static_cast<std::uint8_t>(state.words[offset / 4u] >>
                                     ((offset % 4u) * 8u));
}
void SetByte(pal4::RtkPrtStdAtomicObjectState& state,
             std::uint32_t offset, std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

struct Runtime final : pal4::IRtkPrtStdAtomicControlRuntime {
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::uint32_t manager = 0x9000u;
    std::uint8_t child_visible = 0u;
    std::uint8_t validate_result = 1u;
    std::uint8_t state_result = 0x7bu;
    std::uint8_t update_result = 0x5au;
    std::uint32_t delete_result = 0xabcdefu;
    std::uint32_t frame_calls = 0u;

    void ResetPlaybackFacet(std::uint32_t facet) override {
        Check(facet == 0x1014u, "reset calls owner plus 20 facet");
        calls.emplace_back("reset-facet");
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        writes.emplace_back(address, value);
        calls.emplace_back("write");
    }
    void WriteFloat(std::uint32_t address, float value) override {
        WriteWord(address, std::bit_cast<std::uint32_t>(value));
    }
    void UpdateFrame(std::uint32_t resource) override {
        Check(resource == 0x3000u, "frame update resource");
        ++frame_calls;
        calls.emplace_back("frame");
    }
    void DeactivateResource(std::uint32_t resource) override {
        Check(resource == 0x3000u, "deactivate resource");
        calls.emplace_back("deactivate");
    }
    void DestroyResource(std::uint32_t resource) override {
        Check(resource == 0x3000u, "destroy resource");
        calls.emplace_back("destroy");
    }
    void FreeResource(std::uint32_t resource) override {
        Check(resource == 0x3000u, "free resource");
        calls.emplace_back("free");
    }
    void ProcessEvent(std::uint32_t system, std::uint32_t owner) override {
        Check(system == 0x5000u && owner == 0x1000u,
              "system event system and owner");
        calls.emplace_back("event");
    }
    std::uint32_t DeleteObject(std::uint32_t owner,
                                std::uint32_t flags) override {
        Check(owner == 0x1000u && flags == 1u,
              "stop invokes deleting destructor with flag one");
        calls.emplace_back("delete");
        return delete_result;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x2053u, "child visibility byte +83");
        calls.emplace_back("read-child");
        return child_visible;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read-word");
        if (address == 0x5030u) return 0x7000u;
        Check(address == 0x302cu, "resource object word +44");
        return 0x8000u;
    }
    std::uint8_t UpdateState(std::uint32_t resource, double elapsed,
                              std::uint32_t argument, std::uint8_t mode) override {
        Check(elapsed == 2.5 && resource == 0x3000u && argument == 0x7000u && mode == 0x34u,
              "advance forwards own resource, elapsed, context argument and mode");
        calls.emplace_back("update");
        return update_result;
    }
    std::uint8_t HandleStateChange(std::uint32_t resource,
                                    std::uint32_t context) override {
        Check(resource == 0x3000u && context == 0x8888u,
              "pending state change arguments");
        calls.emplace_back("state-change");
        return state_result;
    }
    std::uint32_t GetObjectTransform(std::uint32_t object) override {
        Check(object == 0x8000u, "native object from particle resource");
        calls.emplace_back("transform");
        return 0x8100u;
    }
    std::uint32_t EffectManager() override {
        calls.emplace_back("manager");
        return manager;
    }
    std::uint8_t ValidateTransform(std::uint32_t target,
                                    std::uint32_t context,
                                    std::uint32_t transform) override {
        Check(target == 0x9000u && context == 0x8888u &&
                  transform == 0x8130u,
              "manager validates transform plus 48");
        calls.emplace_back("validate");
        return validate_result;
    }
};

void TestReset() {
    pal4::RtkPrtStdAtomicObjectState state{};
    Runtime runtime;
    state.words[23u] = 0x3000u;
    Check(pal4::RtkPrtStdAtomicControl::Reset(state, 0x1000u, runtime) == 0u &&
              runtime.calls.empty(), "inactive reset skips all native work");
    SetByte(state, 4u, 1u);
    SetByte(state, 11u, 7u);
    for (std::uint32_t i = 0u; i != 9u; ++i)
        state.words[25u + i] = 0x11111111u + i;
    state.words[24u] = 0x80402010u;
    Check(pal4::RtkPrtStdAtomicControl::Reset(state, 0x1000u, runtime) == 1u &&
              runtime.frame_calls == 3u && Byte(state, 11u) == 0u &&
              runtime.writes.size() == 13u,
          "active reset restores transforms and four color channels");
    const std::vector<std::string> expected_calls{
        "reset-facet", "write", "write", "write", "frame",
        "write", "write", "write", "frame",
        "write", "write", "write", "frame",
        "write", "write", "write", "write", "deactivate"};
    Check(runtime.calls == expected_calls,
          "reset facet, scale, rotation, position, color, deactivate order");
    for (std::uint32_t i = 0u; i != 3u; ++i) {
        Check(runtime.writes[i] == std::pair{0x3064u + 4u * i,
                                              state.words[28u + i]} &&
                  runtime.writes[3u + i] == std::pair{0x3058u + 4u * i,
                                                       state.words[25u + i]} &&
                  runtime.writes[6u + i] == std::pair{0x3070u + 4u * i,
                                                       state.words[31u + i]},
              "reset uses cached scale, rotation and position raw words");
    }
    for (std::uint32_t i = 0u; i != 4u; ++i) {
        const auto [address, bits] = runtime.writes[9u + i];
        const auto expected = pal4::test::ColorBytePc24((state.words[24u] >> (i * 8u)) & 0xffu);
        Check(address == 0x30d4u + 4u * i &&
                  std::bit_cast<float>(bits) == expected,
              "reset normalizes owner packed RGBA into resource floats");
    }
    for(unsigned n=0;n<256u;++n) {
        runtime.writes.clear();runtime.calls.clear();
        state.words[24u]=n*0x01010101u;
        (void)pal4::RtkPrtStdAtomicControl::Reset(state,0x1000u,runtime);
        for(unsigned channel=0;channel<4u;++channel)
            Check(runtime.writes[9u+channel].second==std::bit_cast<std::uint32_t>(pal4::test::ColorBytePc24(n)),
                "reset all256 bytes matches independent original x87 reference");
    }
}

void TestStop() {
    pal4::RtkPrtStdAtomicObjectState state{};
    state.words.fill(0xaaaaaaaau);
    Runtime runtime;
    SetByte(state, 4u, 0u);
    Check(pal4::RtkPrtStdAtomicControl::Stop(state, 0x1000u, runtime) ==
                  runtime.delete_result &&
              runtime.calls == std::vector<std::string>{"delete"},
          "inactive stop only deletes owner");
    runtime.calls.clear();
    SetByte(state, 4u, 1u);
    state.words[4u] = 0x5000u;
    state.words[23u] = 0x3000u;
    Check(pal4::RtkPrtStdAtomicControl::Stop(state, 0x1000u, runtime) ==
                  runtime.delete_result &&
              runtime.calls == std::vector<std::string>{
                  "destroy", "free", "event", "delete"} &&
              Byte(state, 4u) == 0u && Byte(state, 6u) == 0u &&
              Byte(state, 7u) == 0u && Byte(state, 84u) == 0u &&
              state.words[23u] == 0u && state.words[15u] == 0u &&
              state.words[16u] == 0x3f800000u &&
              state.words[17u] == 0x3f800000u &&
              state.words[18u] == 0u && state.words[19u] == 0u &&
              state.words[20u] == 0u && state.words[22u] == 0u,
          "active stop resets fields, releases resource, notifies and deletes");
    runtime.calls.clear();
    SetByte(state, 4u, 1u);
    Check(pal4::RtkPrtStdAtomicControl::Stop(state, 0x1000u, runtime) ==
                  runtime.delete_result &&
              runtime.calls == std::vector<std::string>{"event", "delete"},
          "active stop with absent resource still notifies and deletes");
}

void TestAdvanceAndPending() {
    pal4::RtkPrtStdAtomicObjectState state{};
    Runtime runtime;
    Check(pal4::RtkPrtStdAtomicControl::Advance(state, 2.5, runtime) == 0u &&
              runtime.calls.empty(), "inactive update gate");
    SetByte(state, 4u, 1u);
    Check(pal4::RtkPrtStdAtomicControl::Advance(state, 2.5, runtime) == 0u &&
              runtime.calls.empty(), "not-running update gate");
    SetByte(state, 6u, 1u);
    SetByte(state, 11u, 0x34u);
    state.words[4u] = 0x5000u;
    state.words[23u] = 0x3000u;
    Check(pal4::RtkPrtStdAtomicControl::Advance(state, 2.5, runtime) ==
                  0x5au && runtime.calls ==
                  std::vector<std::string>{"read-word", "update"},
          "running update forwards native resource and byte mode");

    runtime.calls.clear();
    Check(pal4::RtkPrtStdAtomicControl::ActivatePending(
              state, 0x8888u, runtime) == 0u && runtime.calls.empty(),
          "not-pending activation gate");
    SetByte(state, 7u, 1u);
    state.words[23u] = 0x3000u;
    Check(pal4::RtkPrtStdAtomicControl::ActivatePending(
              state, 0x8888u, runtime) == 0x7bu &&
              runtime.calls == std::vector<std::string>{"state-change"},
          "missing child activates resource directly");
    runtime.calls.clear();
    state.words[3u] = 0x2000u;
    Check(pal4::RtkPrtStdAtomicControl::ActivatePending(
              state, 0x8888u, runtime) == 0x7bu &&
              runtime.calls == std::vector<std::string>{
                  "read-child", "state-change"},
          "hidden child bypasses validation");

    runtime.calls.clear();
    runtime.child_visible = 1u;
    runtime.validate_result = 0u;
    Check(pal4::RtkPrtStdAtomicControl::ActivatePending(
              state, 0x8888u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read-child", "read-word", "transform", "manager", "validate"},
          "visible child with invalid transform does not activate");
    runtime.calls.clear();
    runtime.validate_result = 1u;
    Check(pal4::RtkPrtStdAtomicControl::ActivatePending(
              state, 0x8888u, runtime) == 0x7bu &&
              runtime.calls.back() == "state-change",
          "validated transform activates pending resource");
    runtime.calls.clear();
    runtime.manager = 0u;
    try {
        (void)pal4::RtkPrtStdAtomicControl::ActivatePending(
            state, 0x8888u, runtime);
        Check(false, "missing manager must throw");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) ==
                  "\xb5\xa5\xbc\xfe\xb6\xd4\xcf\xf3\xce\xb4\xb1\xbb"
                  "\xb4\xb4\xbd\xa8\xa3\xac\xc7\xeb\xd4\xda\xca\xb9"
                  "\xd3\xc3\xd6\xae\xc7\xb0\xb4\xb4\xbd\xa8\xa3\xa1" &&
                  runtime.calls.back() == "manager",
              "missing manager preserves native GBK exception text");
    }
}

void TestPlaybackFlags() {
    pal4::RtkPrtStdAtomicObjectState state{};
    state.words.fill(0xaaaaaaaau);
    SetByte(state, 4u, 0u);
    Check(pal4::RtkPrtStdAtomicControl::EnablePlayback(state) == 0u &&
              pal4::RtkPrtStdAtomicControl::DisablePlayback(state) == 0u &&
              Byte(state, 6u) == 0xaau && Byte(state, 7u) == 0xaau,
          "inactive playback facets preserve status bytes");
    SetByte(state, 4u, 1u);
    SetByte(state, 6u, 0u);
    Check(pal4::RtkPrtStdAtomicControl::EnablePlayback(state) == 1u &&
              Byte(state, 6u) == 1u && Byte(state, 7u) == 1u,
          "enable facet starts and schedules active object");
    SetByte(state, 6u, 0x7bu);
    Check(pal4::RtkPrtStdAtomicControl::EnablePlayback(state) == 0x7bu &&
              Byte(state, 7u) == 1u,
          "already-running enable returns original byte");
    Check(pal4::RtkPrtStdAtomicControl::DisablePlayback(state) == 0u &&
              Byte(state, 6u) == 0u && Byte(state, 7u) == 0u,
          "disable facet clears running and pending bytes");
}

}  // namespace

int main() {
    TestReset();
    TestStop();
    TestAdvanceAndPending();
    TestPlaybackFlags();
}
