#include "pal4/base_object_lifecycle.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using Event = std::pair<std::string, std::uint32_t>;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::BaseObjectRuntimeStub {
    std::vector<Event> events;
    Event throw_at{"", 0u};

    void Record(const char* operation, std::uint32_t offset) {
        events.emplace_back(operation, offset);
        if (events.back() == throw_at) {
            throw std::runtime_error("subobject construction failed");
        }
    }
    void ConstructResource(pal4::BaseObjectState&,
                           std::uint32_t offset) override {
        Record("resource+", offset);
    }
    void DestroyResource(pal4::BaseObjectState&,
                         std::uint32_t offset) override {
        Record("resource-", offset);
    }
    void InitializeAnimation(pal4::BaseObjectState&,
                             std::uint32_t offset) override {
        Record("animation+", offset);
    }
    void CleanupAnimation(pal4::BaseObjectState&,
                          std::uint32_t offset) override {
        Record("animation-", offset);
    }
    void ConstructEffect(pal4::BaseObjectState&,
                         std::uint32_t offset) override {
        Record("effect+", offset);
    }
    void ResetEffect(pal4::BaseObjectState&,
                     std::uint32_t offset) override {
        Record("effect-", offset);
    }
    void ConstructText(pal4::BaseObjectState&,
                       std::uint32_t offset) override {
        Record("text+", offset);
    }
    void CleanupText(pal4::BaseObjectState&,
                     std::uint32_t offset) override {
        Record("text-", offset);
    }
    void ConstructCharacter(pal4::BaseObjectState&,
                            std::uint32_t offset) override {
        Record("character+", offset);
    }
    void DestroyCharacter(pal4::BaseObjectState&,
                          std::uint32_t offset) override {
        Record("character-", offset);
    }
};

void CheckNormalLifecycle() {
    pal4::BaseObjectState state{};
    state.words.fill(0xa5a5a5a5u);
    Runtime runtime;
    Check(pal4::BaseObjectLifecycle::Construct(state, 0x6000u, runtime) ==
              0x6000u,
          "constructor returns the original object address");
    const std::vector<Event> construction{
        {"resource+", 0u}, {"animation+", 0x170u},
        {"resource+", 0x218u}, {"resource+", 0x388u},
        {"effect+", 0x520u}, {"effect+", 0x528u},
        {"effect+", 0x534u}, {"effect+", 0x53cu},
        {"text+", 0x544u}, {"text+", 0x554u},
        {"character+", 0x564u}};
    Check(runtime.events == construction,
          "constructor invokes embedded subobjects in IDA order");
    Check(state.Read(0u) == 0x8433d8u &&
              state.Read(0x170u) == 0x8433c8u &&
              state.Read(0x1ecu) == 0x8433bcu &&
              state.Read(0x208u) == 0x843434u &&
              state.Read(0x210u) == 0x843428u,
          "constructor installs the observed vtable words");
    for (auto offset : {0x4f8u, 0x500u, 0x508u, 0x510u}) {
        Check(state.Read(offset) == 0xffffffffu &&
                  state.Read(offset + 4u) == 0u,
              "constructor initializes all four handle pairs");
    }
    for (auto offset : {0x1f0u, 0x200u, 0x204u, 0x518u,
                        0x51cu, 0x530u, 0x550u, 0x560u,
                        0x604u, 0x608u}) {
        Check(state.Read(offset) == 0u,
              "constructor clears each observed field");
    }
    Check(state.Read(0x20cu) == 0x6000u &&
              state.Read(0x214u) == 0x6000u &&
              state.Read(0x1f4u) == 0xa5a5a5a5u &&
              state.Read(0x5fcu) == 0xa5a5a5a5u,
          "self links and untouched neighboring fields are preserved");

    runtime.events.clear();
    pal4::BaseObjectLifecycle::Destroy(state, runtime);
    const std::vector<Event> destruction{
        {"character-", 0x564u}, {"text-", 0x554u},
        {"text-", 0x544u}, {"effect-", 0x53cu},
        {"effect-", 0x534u}, {"effect-", 0x528u},
        {"effect-", 0x520u}, {"resource-", 0x388u},
        {"resource-", 0x218u}, {"animation-", 0x170u},
        {"resource-", 0u}};
    Check(runtime.events == destruction,
          "destructor unwinds subobjects in reverse IDA order");
    Check(state.Read(0u) == 0x8433d8u &&
              state.Read(0x170u) == 0x8433c8u &&
              state.Read(0x1ecu) == 0x841e80u &&
              state.Read(0x208u) == 0x8425a0u &&
              state.Read(0x210u) == 0x8425a0u,
          "destructor applies the observed base-vtable transitions");
}

void CheckConstructionFailure() {
    pal4::BaseObjectState state{};
    Runtime runtime;
    runtime.throw_at = {"text+", 0x554u};
    bool threw = false;
    try {
        (void)pal4::BaseObjectLifecycle::Construct(state, 0x6000u, runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw, "failed embedded construction propagates its exception");
    const std::vector<Event> tail{
        {"text-", 0x544u}, {"effect-", 0x53cu},
        {"effect-", 0x534u}, {"effect-", 0x528u},
        {"effect-", 0x520u}, {"resource-", 0x388u},
        {"resource-", 0x218u}, {"animation-", 0x170u},
        {"resource-", 0u}};
    Check(runtime.events.size() >= tail.size() &&
              std::vector<Event>(runtime.events.end() - tail.size(),
                                 runtime.events.end()) == tail,
          "constructor unwinds only completed subobjects in reverse order");
}

}  // namespace

int main() {
    CheckNormalLifecycle();
    CheckConstructionFailure();
}
