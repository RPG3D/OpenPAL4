#include "pal4/actor_manager_owned_slot_array.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t array = 0x1000u;
constexpr std::uint32_t begin = 0x2000u;
constexpr std::uint32_t source = 0x3000u;

struct Runtime final : pal4::IActorManagerOwnedSlotArrayRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {array + 4u, begin}, {array + 8u, begin}, {array + 12u, begin}};
    std::map<std::uint32_t, std::uint8_t> flags;
    std::vector<std::string> trace;
    bool allocation_failure = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return flags[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        flags[address] = value;
        trace.emplace_back("flag:" + std::to_string(address));
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        trace.emplace_back("write:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        trace.emplace_back("allocate:" + std::to_string(bytes));
        return allocation_failure ? 0u : 0x4000u;
    }
    void Free(std::uint32_t address) override {
        trace.emplace_back("free:" + std::to_string(address));
    }
    void DeleteActor(std::uint32_t actor,
                     std::uint32_t flags_value) override {
        Check(flags_value == 1u, "deleting actor flag");
        trace.emplace_back("release:" + std::to_string(actor));
    }
};

void Add(Runtime& runtime, std::uint32_t index, std::uint32_t actor) {
    const auto slot = begin + index * 8u;
    runtime.flags[slot] = 1u;
    runtime.words[slot + 4u] = actor;
    runtime.words[array + 8u] = slot + 8u;
}

std::vector<std::uint32_t> Actors(const Runtime& runtime) {
    std::vector<std::uint32_t> result;
    for (auto slot = runtime.words.at(array + 4u);
         slot != runtime.words.at(array + 8u); slot += 8u)
        result.push_back(runtime.words.at(slot + 4u));
    return result;
}

void ZeroCount() {
    Runtime runtime;
    Add(runtime, 0u, 11u);
    runtime.words[array + 12u] = begin + 32u;
    runtime.trace.clear();
    Check(pal4::ActorManagerOwnedSlotArray::InsertCopies(
              array, begin, 0u, source, runtime) == 0u &&
              runtime.trace.empty() && Actors(runtime) ==
                  std::vector<std::uint32_t>{11u},
          "zero-count in-place insert is a no-op");
}

void InPlaceLongTail() {
    Runtime runtime;
    for (std::uint32_t index = 0u; index < 4u; ++index)
        Add(runtime, index, 11u + index);
    runtime.words[array + 12u] = begin + 64u;
    runtime.flags[source] = 1u;
    runtime.words[source + 4u] = 99u;
    runtime.trace.clear();
    Check(pal4::ActorManagerOwnedSlotArray::InsertCopies(
              array, begin + 8u, 1u, source, runtime) == 8u &&
              Actors(runtime) == std::vector<std::uint32_t>{
                  11u, 99u, 12u, 13u, 14u} &&
              runtime.flags[source] == 0u &&
              runtime.flags[begin + 8u] == 1u,
          "long tail shifts backward and transfers source ownership");
    Check(runtime.trace.front() == "flag:8224" &&
              runtime.trace.back() == "write:4104",
          "long tail constructs last element before shifting");
}

void InPlaceShortTail() {
    Runtime runtime;
    Add(runtime, 0u, 11u);
    Add(runtime, 1u, 12u);
    runtime.words[array + 12u] = begin + 64u;
    runtime.flags[source] = 1u;
    runtime.words[source + 4u] = 99u;
    runtime.trace.clear();
    Check(pal4::ActorManagerOwnedSlotArray::InsertCopies(
              array, begin + 8u, 3u, source, runtime) == 24u &&
              Actors(runtime) == std::vector<std::uint32_t>{
                  11u, 99u, 99u, 99u, 12u} &&
              runtime.flags[source] == 0u &&
              runtime.flags[begin + 16u] == 1u &&
              runtime.flags[begin + 8u] == 0u,
          "short tail moves old suffix and copies source count times");
    Check(runtime.trace.front() == "flag:8224" &&
              runtime.trace.back() == "write:4104",
          "short tail moves suffix before filling gap");
}

void ReallocateAndFailure() {
    Runtime runtime;
    Add(runtime, 0u, 11u);
    Add(runtime, 1u, 12u);
    runtime.words[array + 12u] = begin + 16u;
    runtime.flags[source] = 1u;
    runtime.words[source + 4u] = 99u;
    runtime.trace.clear();
    Check(pal4::ActorManagerOwnedSlotArray::InsertCopies(
              array, begin + 8u, 1u, source, runtime) == 0x4018u &&
              runtime.words[array + 4u] == 0x4000u &&
              runtime.words[array + 8u] == 0x4018u &&
              runtime.words[array + 12u] == 0x4020u &&
              Actors(runtime) == std::vector<std::uint32_t>{11u, 99u, 12u} &&
              runtime.flags[source] == 0u,
          "full array doubles capacity and returns new end address");
    Check(runtime.trace.front() == "allocate:32" &&
              runtime.trace[runtime.trace.size() - 4u] == "free:8192",
          "old slots are destroyed before old storage is freed");

    Runtime failed;
    Add(failed, 0u, 11u);
    failed.words[array + 12u] = begin + 8u;
    failed.words[source + 4u] = 99u;
    failed.flags[source] = 1u;
    failed.allocation_failure = true;
    bool caught = false;
    try {
        (void)pal4::ActorManagerOwnedSlotArray::InsertCopies(
            array, begin + 8u, 1u, source, failed);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && failed.words[array + 8u] == begin + 8u &&
              failed.flags[source] == 1u &&
              failed.trace == std::vector<std::string>{"allocate:16"},
          "allocation failure leaves old array and source untouched");
}

void OwnedSlotPrimitives() {
    Runtime runtime;
    constexpr std::uint32_t first = 0x5000u;
    constexpr std::uint32_t second = 0x5010u;
    constexpr std::uint32_t third = 0x5020u;
    runtime.flags[first] = 1u;
    runtime.words[first + 4u] = 17u;
    Check(pal4::ActorManagerOwnedSlot::MoveConstructNullable(
              0u, first, runtime) == 0u &&
              runtime.flags[first] == 1u && runtime.trace.empty(),
          "nullable move constructor skips null destination");
    Check(pal4::ActorManagerOwnedSlot::MoveConstructNullable(
              second, first, runtime) == second &&
              runtime.flags[first] == 0u &&
              runtime.flags[second] == 1u &&
              runtime.words[second + 4u] == 17u,
          "move constructor transfers actor ownership");
    runtime.flags[third] = 1u;
    runtime.words[third + 4u] = 17u;
    runtime.trace.clear();
    Check(pal4::ActorManagerOwnedSlot::MoveAssign(
              second, third, runtime) == second &&
              runtime.flags[second] == 1u &&
              runtime.flags[third] == 0u &&
              runtime.trace == std::vector<std::string>{
                  "flag:20496", "flag:20512", "write:20500"},
          "same-pointer move assignment merges ownership without deletion");
    Check(pal4::ActorManagerOwnedSlot::MoveAssign(
              second, second, runtime) == second &&
              runtime.flags[second] == 1u,
          "self assignment is a no-op");
    runtime.flags[third] = 0u;
    runtime.words[third + 4u] = 29u;
    runtime.trace.clear();
    (void)pal4::ActorManagerOwnedSlot::MoveAssign(second, third, runtime);
    Check(runtime.trace.front() == "release:17" &&
              runtime.flags[second] == 0u &&
              runtime.words[second + 4u] == 29u,
          "different-pointer assignment deletes prior owned actor");
    runtime.flags[third] = 1u;
    runtime.words[third + 4u] = 41u;
    runtime.trace.clear();
    Check(pal4::ActorManagerOwnedSlot::Destroy(third, 2u, runtime) == third &&
              runtime.trace == std::vector<std::string>{"release:41"},
          "nondeleting destructor releases owned actor only");
    runtime.trace.clear();
    Check(pal4::ActorManagerOwnedSlot::Destroy(third, 3u, runtime) == third &&
              runtime.trace == std::vector<std::string>{
                  "release:41", "free:20512"},
          "deleting destructor releases actor before slot memory");
    runtime.trace.clear();
    Check(pal4::ActorManagerOwnedSlot::DestroyEventPayload(
              third, 1u, runtime) == third &&
          runtime.trace == std::vector<std::string>{
              "release:41", "free:20512"},
          "event payload thunk forwards release and delete flag");
}

}  // namespace

int main() {
    try {
        ZeroCount();
        InPlaceLongTail();
        InPlaceShortTail();
        ReallocateAndFailure();
        OwnedSlotPrimitives();
        std::cout << "actor_manager_owned_slot_array_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
