#include "pal4/game_object_animation_event_array.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::IGameObjectAnimationEventInsertRuntime {
public:
    using Event = std::array<std::uint8_t, 0x14c>;
    std::map<std::uint32_t, Event> events;
    std::vector<std::string> calls;
    std::uint32_t allocation = 0x5000u;

    void Put(std::uint32_t address, std::uint8_t marker) {
        Event event{};
        event[0] = marker;
        event.back() = static_cast<std::uint8_t>(marker + 100u);
        events[address] = event;
    }
    void Expect(std::uint32_t begin,
                std::initializer_list<std::uint8_t> markers) const {
        std::uint32_t address = begin;
        for (auto marker : markers) {
            const auto found = events.find(address);
            Check(found != events.end() && found->second[0] == marker &&
                      found->second.back() == marker + 100u,
                  "event payload survives insertion");
            address += 0x14cu;
        }
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("alloc:" + std::to_string(bytes));
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
    }
    void CopyRaw(std::uint32_t destination,
                 std::uint32_t source) override {
        Check(events.contains(source), "raw copy source exists");
        calls.emplace_back("raw:" + std::to_string(destination) + ":" +
                           std::to_string(source));
        events[destination] = events.at(source);
    }
    void AssignEvent(std::uint32_t destination,
                     std::uint32_t source) override {
        Check(events.contains(source), "assigned event source exists");
        calls.emplace_back("assign:" + std::to_string(destination) + ":" +
                           std::to_string(source));
        events[destination] = events.at(source);
    }
    void DestroyEvent(std::uint32_t address) override {
        calls.emplace_back("destroy:" + std::to_string(address));
    }
};

pal4::GameObjectAnimationEventArrayState ArrayState(
    std::uint32_t size, std::uint32_t capacity) {
    return {0, 0x2000u, 0x2000u + size * 0x14cu,
            0x2000u + capacity * 0x14cu};
}

void InPlaceLongTail() {
    Runtime runtime;
    runtime.Put(0x9000u, 9);
    auto state = ArrayState(3, 5);
    for (std::uint32_t index = 0; index < 3; ++index)
        runtime.Put(state.begin + index * 0x14cu,
                    static_cast<std::uint8_t>(index + 1));
    Check(pal4::GameObjectAnimationEventArray::Insert(
              state, state.begin + 0x14cu, 1, 0x9000u, runtime) ==
              0x2000u + 4u * 0x14cu &&
              state.end == 0x2000u + 4u * 0x14cu &&
              state.capacity == 0x2000u + 5u * 0x14cu &&
              runtime.calls == std::vector<std::string>{
                  "raw:9188:8856", "assign:8856:8524",
                  "assign:8524:36864"},
          "long tail copies final event, shifts backward and assigns gap");
    runtime.Expect(state.begin, {1, 9, 2, 3});
    runtime.calls.clear();
    Check(pal4::GameObjectAnimationEventArray::Insert(
              state, state.begin, 0, 0x9000u, runtime) == 0 &&
              runtime.calls.empty(), "zero-count in-place insertion is inert");
}

void InPlaceShortTail() {
    Runtime runtime;
    runtime.Put(0x9000u, 9);
    auto state = ArrayState(3, 7);
    for (std::uint32_t index = 0; index < 3; ++index)
        runtime.Put(state.begin + index * 0x14cu,
                    static_cast<std::uint8_t>(index + 1));
    Check(pal4::GameObjectAnimationEventArray::Insert(
              state, state.begin + 2u * 0x14cu, 3, 0x9000u, runtime) ==
              0x2000u + 6u * 0x14cu &&
              runtime.calls == std::vector<std::string>{
                  "raw:9852:8856", "raw:9188:36864",
                  "raw:9520:36864", "assign:8856:36864"},
          "short tail moves suffix, constructs extra slots and fills old gap");
    runtime.Expect(state.begin, {1, 2, 9, 9, 9, 3});
}

void ReallocationAndFailure() {
    Runtime runtime;
    runtime.Put(0x9000u, 9);
    auto state = ArrayState(2, 2);
    runtime.Put(state.begin, 1);
    runtime.Put(state.begin + 0x14cu, 2);
    Check(pal4::GameObjectAnimationEventArray::Insert(
              state, state.begin + 0x14cu, 1, 0x9000u, runtime) == 123u &&
              state.begin == 0x5000u &&
              state.end == 0x5000u + 3u * 0x14cu &&
              state.capacity == 0x5000u + 4u * 0x14cu &&
              runtime.calls == std::vector<std::string>{
                  "alloc:1328", "raw:20480:8192", "raw:20812:36864",
                  "raw:21144:8524", "destroy:8192", "destroy:8524",
                  "free:8192"},
          "reallocation copies prefix/template/suffix then frees old range");
    runtime.Expect(state.begin, {1, 9, 2});
    runtime.calls.clear();
    Check(pal4::GameObjectAnimationEventArray::Release(state, runtime) == 0 &&
              state.begin == 0 && state.end == 0 && state.capacity == 0 &&
              runtime.calls == std::vector<std::string>{
                  "destroy:20480", "destroy:20812", "destroy:21144",
                  "free:20480"},
          "array release destroys each event, frees begin and clears pointers");

    Runtime empty;
    empty.Put(0x9000u, 9);
    pal4::GameObjectAnimationEventArrayState zero;
    Check(pal4::GameObjectAnimationEventArray::Insert(
              zero, 0, 2, 0x9000u, empty) == 82u &&
              zero.begin == 0x5000u &&
              zero.end == 0x5000u + 2u * 0x14cu &&
              zero.capacity == zero.end &&
              empty.calls.back() == "free:0",
          "empty array grows to insertion count and frees null old begin");
    empty.Expect(zero.begin, {9, 9});
    empty.calls.clear();
    pal4::GameObjectAnimationEventArrayState empty_state;
    Check(pal4::GameObjectAnimationEventArray::Release(
              empty_state, empty) == 0 &&
              empty.calls == std::vector<std::string>{"free:0"},
          "empty array release still forwards null pointer to MemFree");

    Runtime failed;
    failed.Put(0x9000u, 9);
    failed.allocation = 0;
    auto full = ArrayState(2, 2);
    const auto original = full;
    Check(pal4::GameObjectAnimationEventArray::Insert(
              full, full.begin, 1, 0x9000u, failed) == 0 &&
              full.begin == original.begin && full.end == original.end &&
              full.capacity == original.capacity &&
              failed.calls == std::vector<std::string>{"alloc:1328"},
          "failed reallocation preserves old vector");

    failed.calls.clear();
    failed.allocation = 0xfffffff0u;
    Check(pal4::GameObjectAnimationEventArray::Insert(
              full, full.begin, 1, 0x9000u, failed) == 0 &&
              full.begin == original.begin && full.end == original.end &&
              failed.calls == std::vector<std::string>{
                  "alloc:1328", "free:4294967280"},
          "address overflow releases new allocation and preserves old vector");
}

}  // namespace

int main() {
    InPlaceLongTail();
    InPlaceShortTail();
    ReallocationAndFailure();
    std::cout << "game_object_animation_event_insert_tests passed\n";
}
