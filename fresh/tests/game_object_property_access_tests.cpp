#include "pal4/game_object_property_access.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::GameObjectPropertyRuntimeStub {
    pal4::GameObjectPropertyState& state;
    bool getter_called{};
    bool setter_called{};
    std::uint32_t health_notifications{};
    std::uint32_t getter_slot{0x1cu};
    std::uint32_t setter_slot{0x20u};

    explicit Runtime(pal4::GameObjectPropertyState& object) : state(object) {}
    std::uint32_t CallGetter(std::uint32_t vtable,
                             std::uint32_t slot,
                             std::uint32_t object,
                             std::uint32_t index) override {
        Check(vtable == 0x1234u && slot == getter_slot &&
                  object == 0x5000u && index == 4u,
              "indexed getter uses cached initial vtable");
        getter_called = true;
        state.words[0] = 0x5678u;
        return 0xfffffffdu;
    }
    std::uint32_t CallSetter(std::uint32_t vtable,
                             std::uint32_t slot,
                             std::uint32_t object,
                             std::uint32_t index,
                             std::uint32_t value) override {
        Check(getter_called && vtable == 0x1234u &&
                  state.words[0] == 0x5678u && slot == setter_slot &&
                  object == 0x5000u && index == 4u && value == 2u,
              "indexed setter reuses saved vtable and wrapped sum");
        setter_called = true;
        return 0xfeedu;
    }
    void NotifyHealthStatus(std::uint32_t object) override {
        Check(object == 0x5000u, "health notification receives object");
        ++health_notifications;
    }
};

void TestFixedFields() {
    pal4::GameObjectPropertyState state{};
    state.words.fill(0x77777777u);
    Check(pal4::GameObjectPropertyAccess::Set85(state, 11u) == 11u &&
              pal4::GameObjectPropertyAccess::Get85(state) == 11u &&
              pal4::GameObjectPropertyAccess::Set168(state, 22u) == 22u &&
              pal4::GameObjectPropertyAccess::Get168(state) == 22u &&
              pal4::GameObjectPropertyAccess::Set167And83(state, 33u) == 33u &&
              pal4::GameObjectPropertyAccess::Get167(state) == 33u &&
              state.words[83] == 33u && state.words[84] == 0x77777777u,
          "fixed getters and setters touch only observed words");
}

void TestIndexedFields() {
    pal4::GameObjectPropertyState state{};
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        Check(pal4::GameObjectPropertyAccess::SetIndexed116(
                  state, index, 100u + index) == 100u + index &&
                  pal4::GameObjectPropertyAccess::GetIndexed116(
                      state, index) == 100u + index,
              "all five indexed slots round trip");
    }
    const auto before = state.words;
    Check(pal4::GameObjectPropertyAccess::GetIndexed116(state, 5u) == 5u &&
              pal4::GameObjectPropertyAccess::SetIndexed116(
                  state, 0xffffffffu, 999u) == 0xffffffffu &&
              state.words == before,
          "invalid indexed access passes index through and writes nothing");
}

void TestVirtualAdjustment() {
    pal4::GameObjectPropertyState state{};
    state.words[0] = 0x1234u;
    Runtime runtime(state);
    Check(pal4::GameObjectPropertyAccess::AdjustIndexed(
              state, 0x5000u, 4u, 5u, runtime) == 0xfeedu &&
              runtime.getter_called && runtime.setter_called,
          "virtual setter result is forwarded after wrapped adjustment");
    state.words[0] = 0x1234u;
    runtime.getter_slot = 0x34u;
    runtime.setter_slot = 0x44u;
    runtime.getter_called = false;
    runtime.setter_called = false;
    Check(pal4::GameObjectPropertyAccess::AdjustProperty(
              state, 0x5000u, 4u, 5u, runtime) == 0xfeedu &&
              runtime.getter_called && runtime.setter_called,
          "second adjustment uses slots +0x34/+0x44 and cached vtable");
    state.words[0] = 0x1234u;
    runtime.getter_slot = 0x30u;
    runtime.setter_slot = 0x40u;
    runtime.getter_called = false;
    runtime.setter_called = false;
    Check(pal4::GameObjectPropertyAccess::AdjustRecordValue(
              state, 0x5000u, 4u, 5u, runtime) == 0xfeedu &&
              runtime.getter_called && runtime.setter_called,
          "third adjustment uses slots +0x30/+0x40 and cached vtable");
}

void TestEightValueRecord() {
    pal4::GameObjectPropertyState state{};
    for (std::uint32_t index = 0u; index < 8u; ++index)
        Check(pal4::GameObjectPropertyAccess::SetIndexed86(
                  state, index, 200u + index) == 200u + index &&
                  pal4::GameObjectPropertyAccess::GetIndexed86(
                      state, index) == 200u + index,
              "eight inline record values round trip");
    const auto before = state.words;
    Check(pal4::GameObjectPropertyAccess::GetIndexed86(state, 8u) == 8u &&
              pal4::GameObjectPropertyAccess::SetIndexed86(
                  state, 0xffffffffu, 5u) == 0xffffffffu &&
              state.words == before,
          "invalid eight-value indexes pass through without writing");
}

void TestSecondIndexedGroup() {
    pal4::GameObjectPropertyState state{};
    Runtime runtime(state);
    state.words[86] = 1000u;
    state.words[88] = 1000u;
    for (std::uint32_t index = 0u; index < 10u; ++index) {
        Check(pal4::GameObjectPropertyAccess::SetIndexed136(
                  state, 0x5000u, index, static_cast<std::int32_t>(index),
                  runtime) ==
                  index,
              "ordinary second-group property writes raw index value");
        Check(pal4::GameObjectPropertyAccess::GetIndexed136(
                  state, index) == index,
              "second-group getter reads each valid index");
    }
    Check(state.words[56] == 4u,
          "index four also writes the special word 56");
    const auto before = state.words;
    Check(pal4::GameObjectPropertyAccess::SetIndexed136(
              state, 0x5000u, 0xffffffffu, 33, runtime) ==
              0xffffffffu &&
              pal4::GameObjectPropertyAccess::GetIndexed136(
                  state, 10u) == 10u && state.words == before,
          "invalid indexes pass through without writing");
}

void TestSecondGroupClamps() {
    pal4::GameObjectPropertyState state{};
    Runtime runtime(state);
    state.words[86] = 80u;
    state.words[88] = 40u;
    Check(pal4::GameObjectPropertyAccess::SetIndexed136(
              state, 0x5000u, 7u, 81, runtime) == 80u &&
              pal4::GameObjectPropertyAccess::SetIndexed136(
                  state, 0x5000u, 7u, -1, runtime) == 0u &&
              pal4::GameObjectPropertyAccess::SetIndexed136(
                  state, 0x5000u, 7u, 80, runtime) == 80u &&
              runtime.health_notifications == 3u,
          "index seven clamps and always updates health status");
    Check(pal4::GameObjectPropertyAccess::SetIndexed136(
              state, 0x5000u, 8u, 101, runtime) == 100u &&
              pal4::GameObjectPropertyAccess::SetIndexed136(
                  state, 0x5000u, 8u, -2, runtime) == 0u &&
              pal4::GameObjectPropertyAccess::SetIndexed136(
                  state, 0x5000u, 9u, 41, runtime) == 40u &&
              pal4::GameObjectPropertyAccess::SetIndexed136(
                  state, 0x5000u, 9u, -1, runtime) == 0u,
          "indexes eight and nine use their observed limits");
    state.words[86] = 0xfffffffeu;
    Check(pal4::GameObjectPropertyAccess::SetIndexed136(
              state, 0x5000u, 7u, -1, runtime) == 0xfffffffeu,
          "negative maximum is checked before zero floor");
}

}  // namespace

int main() {
    pal4::GameObjectPropertyState height_state{};
    height_state.words[104] = 0x76543210u;
    Check(pal4::GameObjectPropertyAccess::GetHeightLimitFlag(height_state) ==
              0x76543210u,
          "inherited height-limit field at +0x1a0");
    TestFixedFields();
    TestIndexedFields();
    TestVirtualAdjustment();
    TestSecondIndexedGroup();
    TestSecondGroupClamps();
    TestEightValueRecord();
}
