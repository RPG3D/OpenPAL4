#include "pal4/game_arena_manager_lifecycle.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Event {
    pal4::GameArenaLifecycleCall operation;
    std::uint32_t address;
    std::uint32_t value = 0u;
    bool operator==(const Event&) const = default;
};

constexpr auto kWriteWord = static_cast<pal4::GameArenaLifecycleCall>(0xfeu);
constexpr auto kWriteByte = static_cast<pal4::GameArenaLifecycleCall>(0xfdu);
constexpr auto kReadWord = static_cast<pal4::GameArenaLifecycleCall>(0xfcu);
constexpr auto kAllocate = static_cast<pal4::GameArenaLifecycleCall>(0xfbu);

struct LifecycleRuntime final : pal4::IGameArenaLifecycleRuntime {
    std::vector<Event> events;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t allocation = 0x5000u;

    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back({kAllocate, bytes});
        return allocation;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.push_back({kReadWord, address});
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        events.push_back({kWriteWord, address, value});
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        events.push_back({kWriteByte, address, value});
    }
    std::uint32_t Call(pal4::GameArenaLifecycleCall operation,
                       std::uint32_t address,
                       std::uint32_t auxiliary) override {
        events.push_back({operation, address, auxiliary});
        if (operation == pal4::GameArenaLifecycleCall::ReleaseArena)
            return 91u;
        return operation == pal4::GameArenaLifecycleCall::DestroyWorld
                   ? 77u : 0u;
    }
};

struct SingletonRuntime final : pal4::IGameArenaSingletonRuntime {
    pal4::GameArenaSingletonState* state = nullptr;
    std::uint32_t result = 0x6000u;
    std::uint32_t calls = 0u;

    std::uint32_t GetSingletonManager() override {
        Check(state != nullptr && (state->guard & 1u) != 0u,
              "guard is set before singleton-manager lookup");
        ++calls;
        return 0x7000u;
    }
    std::uint32_t Register(std::uint32_t manager, const char* name,
                            std::uint32_t id, std::uint32_t factory,
                            std::uint32_t destructor) override {
        Check(manager == 0x7000u && std::string_view(name) == "palGameArena" &&
                  id == 1043u && factory == 0x5184a0u &&
                  destructor == 0x7de5d0u,
              "singleton identity matches IDA");
        ++calls;
        return result;
    }
};

void SingletonRegistration() {
    pal4::GameArenaSingletonState state{0x80u, 0u};
    pal4::GameArenaManagerSingleton singleton(state);
    SingletonRuntime runtime;
    runtime.state = &state;
    Check(singleton.GetInstance(runtime) == 0x6000u &&
              state.guard == 0x81u && state.instance == 0x6000u &&
              singleton.GetInstance(runtime) == 0x6000u &&
              runtime.calls == 2u,
          "arena singleton preserves other guard bits and caches instance");
    pal4::GameArenaSingletonState failed{};
    pal4::GameArenaManagerSingleton failed_singleton(failed);
    SingletonRuntime failed_runtime;
    failed_runtime.state = &failed;
    failed_runtime.result = 0u;
    Check(failed_singleton.GetInstance(failed_runtime) == 0u &&
              failed_singleton.GetInstance(failed_runtime) == 0u &&
              failed_runtime.calls == 2u,
          "failed singleton result is still guarded");
}

void TextPairLifecycle() {
    using pal4::GameArenaLifecycleCall;
    LifecycleRuntime runtime;
    Check(pal4::GameArenaTextPair::Construct(0x8000u, runtime) == 0x8000u &&
              runtime.events == std::vector<Event>{
                  {GameArenaLifecycleCall::InitializeText, 0x8000u},
                  {GameArenaLifecycleCall::InitializeText, 0x800cu}},
          "36-byte pair constructs both 12-byte text members");
    runtime.events.clear();
    pal4::GameArenaTextPair::Destroy(0x8000u, runtime);
    Check(runtime.events == std::vector<Event>{
              {GameArenaLifecycleCall::CleanupText, 0x800cu},
              {GameArenaLifecycleCall::CleanupText, 0x8000u}},
          "pair destroys text members in reverse order");
}

void Construction() {
    using pal4::GameArenaLifecycleCall;
    LifecycleRuntime runtime;
    Check(pal4::GameArenaManagerLifecycle::Create(runtime) == 0x5000u &&
              runtime.events.size() == 63u &&
              runtime.events[0] == Event{kAllocate, 0x2b8u} &&
              runtime.events[1] ==
                  Event{GameArenaLifecycleCall::ConstructWorld, 0x5000u},
          "factory allocates 696 bytes before base construction");
    for (const auto offset : {200u, 212u, 224u, 240u, 252u})
        Check(std::find(runtime.events.begin(), runtime.events.end(),
                        Event{GameArenaLifecycleCall::InitializeText,
                              0x5000u + offset}) != runtime.events.end(),
              "all five top-level text slots are constructed");
    for (const auto offset : {268u, 284u, 300u, 616u, 632u, 648u,
                              664u, 680u}) {
        Check(std::find(runtime.events.begin(), runtime.events.end(),
                        Event{kWriteByte, 0x5000u + offset}) !=
                  runtime.events.end(),
              "indeterminate legacy block flag is defined as zero");
        for (const auto word : {4u, 8u, 12u})
            Check(runtime.words.at(0x5000u + offset + word) == 0u,
                  "embedded block words are zeroed");
    }
    Check(runtime.words.at(0x5000u) == 0x844894u &&
              runtime.words.at(0x5000u + 196u) == 1u &&
              runtime.words.at(0x5000u + 440u) == 0x3e99999au &&
              runtime.words.at(0x5000u + 428u) == 0x3f800000u &&
              runtime.words.at(0x5000u + 432u) == 0x3f800000u &&
              runtime.words.at(0x5000u + 436u) == 0x3f800000u &&
              runtime.events.back() == Event{
                  GameArenaLifecycleCall::InitializeObjectList, 0x5000u},
          "vtable, scalar defaults and final object-list call match IDA");

    LifecycleRuntime failed;
    failed.allocation = 0u;
    Check(pal4::GameArenaManagerLifecycle::Create(failed) == 0u &&
              failed.events == std::vector<Event>{{kAllocate, 0x2b8u}},
          "factory does not construct after failed allocation");
}

void Destruction() {
    using pal4::GameArenaLifecycleCall;
    LifecycleRuntime runtime;
    constexpr std::uint32_t object = 0x5000u;
    runtime.words[object + 504u] = 0x9000u;
    runtime.words[object + 316u] = 0x9100u;
    for (std::uint32_t i = 0u; i < 5u; ++i)
        runtime.words[object + 680u - 16u * i + 4u] = 0xa000u + i;
    runtime.words[object + 304u] = 0xb000u;
    runtime.words[object + 308u] = 0xb010u;
    runtime.words[object + 288u] = 0xc000u;
    runtime.words[object + 292u] = 0xc010u;

    Check(pal4::GameArenaManagerLifecycle::Destroy(object, runtime) == 77u &&
              runtime.events.front() == Event{kWriteWord, object, 0x844894u} &&
              runtime.events.back() ==
                  Event{GameArenaLifecycleCall::DestroyWorld, object},
          "destructor restores vtable and returns base-destruction result");
    const auto find = [&](const Event event) {
        const auto it = std::find(runtime.events.begin(), runtime.events.end(),
                                  event);
        Check(it != runtime.events.end(), "expected destructor action exists");
        return static_cast<std::size_t>(it - runtime.events.begin());
    };
    Check(find({GameArenaLifecycleCall::InitializeDefaultValues, 0x9000u}) <
              find({GameArenaLifecycleCall::FreeMemory, 0x9000u}) &&
              find({GameArenaLifecycleCall::FreeMemory, 0x9000u}) <
              find({GameArenaLifecycleCall::DestroyScript, 0x9100u}) &&
              find({GameArenaLifecycleCall::FreeMemory, 0xa000u}) <
              find({GameArenaLifecycleCall::FreeMemory, 0xa004u}) &&
              find({GameArenaLifecycleCall::CleanupText, object + 592u}) <
              find({GameArenaLifecycleCall::CleanupText, object + 508u}) &&
              find({GameArenaLifecycleCall::DestroyCache, object + 448u}) <
              find({GameArenaLifecycleCall::ResetNameNode, object + 416u}) &&
              find({GameArenaLifecycleCall::DestroyConfig, object + 320u}) <
              find({GameArenaLifecycleCall::ReleaseRange,
                    0xb000u, 0xb010u}) &&
              find({GameArenaLifecycleCall::CleanupMemoryBlock,
                    object + 268u}) <
              find({GameArenaLifecycleCall::CleanupText, object + 252u}),
          "destructor releases resources in IDA reverse lifetime order");
    Check(runtime.words.at(object + 504u) == 0u &&
              runtime.words.at(object + 316u) == 0u &&
              runtime.words.at(object + 684u) == 0u &&
              runtime.words.at(object + 304u) == 0u &&
              runtime.words.at(object + 308u) == 0u,
          "freed pointers and range endpoints are cleared");

    LifecycleRuntime empty;
    Check(pal4::GameArenaManagerLifecycle::Destroy(object, empty) == 77u &&
              std::none_of(empty.events.begin(), empty.events.end(),
                           [](const Event& event) {
                               return event.operation ==
                                          GameArenaLifecycleCall::DestroyScript ||
                                      event.operation ==
                                          GameArenaLifecycleCall::InitializeDefaultValues;
                           }),
          "null optional objects skip their destructors");
}

void ReleaseForwarder() {
    LifecycleRuntime runtime;
    Check(pal4::GameArenaManagerLifecycle::ForwardRelease(0x5000u, runtime) ==
              91u &&
              runtime.events == std::vector<Event>{
                  {pal4::GameArenaLifecycleCall::ReleaseArena, 0x5000u}},
          "0x519950 forwards arena pointer and callee return unchanged");
}

}  // namespace

int main() {
    try {
        SingletonRegistration();
        TextPairLifecycle();
        Construction();
        Destruction();
        ReleaseForwarder();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
