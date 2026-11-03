#include "pal4/combat_system_object.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <initializer_list>
#include <new>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kObject = 0x1000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::string Label(pal4::CombatSystemMember member) {
    return std::to_string(static_cast<int>(member));
}

struct Runtime final : pal4::ICombatSystemObjectRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;
    std::uint32_t next_allocation = 0x3000u;
    std::uint32_t allocation_count = 0u;
    std::uint32_t fail_allocation = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address));
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.emplace_back("allocate:" + std::to_string(size));
        ++allocation_count;
        if (allocation_count == fail_allocation) return 0u;
        const auto address = next_allocation;
        next_allocation += 0x1000u;
        return address;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.emplace_back("free:" + std::to_string(address));
        return 0x5000u;
    }
    void ConstructWorld(std::uint32_t address) override {
        Check(address == kObject, "world constructor offset");
        calls.emplace_back("world");
    }
    void ConstructTranslator(std::uint32_t address) override {
        Check(address == kObject + 380u, "translator constructor offset");
        calls.emplace_back("translator");
    }
    std::uint32_t ConstructMember(pal4::CombatSystemMember member,
                                  std::uint32_t address) override {
        Check(address != 0u, "member constructor requires allocation");
        calls.emplace_back("construct:" + Label(member));
        return address + 16u;
    }
    std::uint32_t ConstructDataStructure() override {
        calls.emplace_back("data-structure");
        return 0x9000u;
    }
    void InitializeTail(std::uint32_t address) override {
        Check(address == kObject + 432u, "tail constructor offset");
        calls.emplace_back("tail");
    }
    void DestroyMember(pal4::CombatSystemMember member,
                       std::uint32_t address) override {
        Check(address != 0u, "destroying nonnull member");
        calls.emplace_back("destroy:" + Label(member));
    }
    void ClearCombatEvents(std::uint32_t address) override {
        calls.emplace_back("clear-events:" + std::to_string(address));
    }
    void DestroyTimerQueue(std::uint32_t address) override {
        calls.emplace_back("timer:" + std::to_string(address));
    }
    void DestroyVirtual(std::uint32_t address,
                        std::uint32_t flag) override {
        Check(flag == 1u, "virtual deleting destructor flag");
        calls.emplace_back("virtual:" + std::to_string(address));
    }
    void DestructTranslator(std::uint32_t address) override {
        Check(address == kObject + 380u, "translator destructor offset");
        calls.emplace_back("destruct-translator");
    }
    std::uint32_t DestructWorld(std::uint32_t address) override {
        Check(address == kObject, "world destructor offset");
        calls.emplace_back("destruct-world");
        return 0x7777u;
    }
    std::uint8_t AreAllObjectsReady(
        std::uint32_t system_manager) override {
        Check(system_manager == 0x3000u,
              "readiness receives member at +396");
        calls.emplace_back("ready");
        return 0xabu;
    }
};

void Construction() {
    Runtime runtime;
    Check(pal4::CombatSystemObject::Construct(kObject, 0x5au, runtime) ==
              kObject, "constructor returns this");
    const std::array<std::uint32_t, 8> offsets{
        396u, 400u, 404u, 408u, 412u, 416u, 420u, 424u};
    const std::array<std::uint32_t, 8> storage{
        0x3000u, 0x4000u, 0x5000u, 0x6000u,
        0x7000u, 0x9000u, 0xa000u, 0xb000u};
    for (std::size_t i = 0; i < offsets.size(); ++i) {
        const auto expected = i == 4u ? storage[i] : storage[i] + 16u;
        Check(runtime.words[kObject + offsets[i]] == expected,
              "member field and allocation order");
    }
    Check(runtime.words[0x7004u] == 0x8000u &&
              runtime.words[0x8000u] == 0x8000u &&
              runtime.words[0x8004u] == 0x8000u &&
              runtime.words[0x7008u] == 0u &&
              runtime.words[0x7010u] == 0x9000u &&
              runtime.words[0x7014u] == 0u &&
              runtime.bytes[0x7000u] == 0x5au &&
              runtime.bytes[0x700cu] == 0x5au,
          "24-byte data object and 40-byte self-linked sentinel");
    Check(runtime.words[kObject + 428u] == 0u &&
              runtime.words[kObject + 504u] == 3u &&
              runtime.words[kObject + 508u] == 7u &&
              runtime.bytes[kObject + 516u] == 0u &&
              runtime.bytes[kObject + 517u] == 0u &&
              runtime.calls[0] == "world" &&
              runtime.calls[1] == "translator" &&
              runtime.calls.back() == "byte:4613",
          "outer initializer order and tail fields");
}

void AllocationFailures() {
    for (const auto failed : {1u, 2u, 3u, 4u, 5u, 7u, 8u, 9u}) {
        Runtime runtime;
        runtime.fail_allocation = failed;
        Check(pal4::CombatSystemObject::Construct(kObject, 0u, runtime) ==
                  kObject, "member allocation failure remains nullable");
        const auto offset = failed <= 4u ? 392u + failed * 4u :
                            failed == 5u ? 412u : 416u + (failed - 7u) * 4u;
        Check(runtime.words[kObject + offset] == 0u,
              "failed allocation stores null member");
    }
    Runtime sentinel_failure;
    sentinel_failure.fail_allocation = 6u;
    bool raised = false;
    try {
        static_cast<void>(pal4::CombatSystemObject::Construct(
            kObject, 0u, sentinel_failure));
    } catch (const std::bad_alloc&) {
        raised = true;
    }
    Check(raised, "invalid native sentinel allocation becomes explicit failure");
}

void Destruction() {
    Runtime runtime;
    for (const auto offset : {396u, 400u, 404u, 408u,
                              412u, 416u, 420u, 424u})
        runtime.words[kObject + offset] = 0x6000u + offset;
    for (const auto offset : {488u, 472u, 456u})
        runtime.words[kObject + offset] = 0x9000u + offset;
    runtime.bytes[0x9000u + 488u - 1u] = 0u;
    runtime.bytes[0x9000u + 472u - 1u] = 2u;
    runtime.bytes[0x9000u + 456u - 1u] = 0xffu;
    Check(pal4::CombatSystemObject::Destruct(kObject, runtime) == 0x7777u,
          "destructor returns world cleanup result");
    const std::vector<std::string> expected{
        "destroy:0", "free:24972", "destroy:1", "free:24976",
        "free:24980", "clear-events:25000",
        "timer:24988", "free:24988", "free:24984",
        "free:24992", "free:24996", "virtual:25000",
        "free:37351", "word:4584", "word:4588", "word:4592",
        "byte:37335", "word:4568", "word:4572", "word:4576",
        "free:37319", "word:4552", "word:4556", "word:4560",
        "destruct-translator", "destruct-world"};
    Check(runtime.calls == expected, "destructor order and text refcounts");
    Check(runtime.bytes[0x9000u + 472u - 1u] == 1u,
          "shared text reference decremented");

    Runtime sparse;
    Check(pal4::CombatSystemObject::Destruct(kObject, sparse) == 0x7777u,
          "empty members still reach world destructor");
    Check(sparse.calls[0] == "free:0" &&
              sparse.calls[sparse.calls.size() - 2u] ==
                  "destruct-translator" &&
              sparse.calls.back() == "destruct-world",
          "null free at +416 remains unconditional");
}

void TextCleanup() {
    Runtime runtime;
    runtime.words[kObject + 56u] = 0x9000u;
    runtime.words[kObject + 40u] = 0xa000u;
    runtime.words[kObject + 24u] = 0xb000u;
    runtime.bytes[0x8fffu] = 0u;
    runtime.bytes[0x9fffu] = 3u;
    runtime.bytes[0xafffu] = 0xffu;
    Check(pal4::CombatSystemTextMembers::Cleanup(kObject, runtime) ==
              0x5000u, "text cleanup forwards final free result");
    Check(runtime.calls == std::vector<std::string>{
              "free:36863", "word:4152", "word:4156", "word:4160",
              "byte:40959", "word:4136", "word:4140", "word:4144",
              "free:45055", "word:4120", "word:4124", "word:4128"},
          "text cleanup visits records in reverse order");
    Check(runtime.bytes[0x9fffu] == 2u &&
              runtime.words[kObject + 56u] == 0u &&
              runtime.words[kObject + 40u] == 0u &&
              runtime.words[kObject + 24u] == 0u,
          "shared reference decremented and all records cleared");
    runtime.calls.clear();
    Check(pal4::CombatSystemTextMembers::Cleanup(kObject, runtime) == 0u &&
              runtime.calls.size() == 9u,
          "empty cleanup clears fields and returns zero");
}

void StateQueries() {
    Runtime runtime;
    runtime.bytes[kObject + 516u] = 0xf1u;
    runtime.words[kObject + 424u] = 0x1234u;
    Check(pal4::CombatSystemObject::IsInactive(kObject, runtime) == 0xf1u &&
              pal4::CombatSystemObject::GetCamera(kObject, runtime) ==
                  0x1234u, "byte flag and camera pointer offsets");
    for (const auto state : {-1, 0, 1, 2, 3, 4, 5, 6}) {
        runtime.words[kObject + 508u] =
            static_cast<std::uint32_t>(state);
        Check(pal4::CombatSystemObject::IsActive(kObject, runtime) ==
                  (state >= 2 && state <= 5),
              "active combat state interval");
        Check(pal4::CombatSystemObject::IsEnding(kObject, runtime) ==
                  (state >= 4 && state <= 5),
              "ending combat state interval");
    }
    runtime.calls.clear();
    Check(pal4::CombatSystemObject::AreAllObjectsReady(
              kObject, runtime) == 0u && runtime.calls.empty(),
          "missing system manager is not ready");
    runtime.words[kObject + 396u] = 0x3000u;
    Check(pal4::CombatSystemObject::AreAllObjectsReady(
              kObject, runtime) == 0xabu &&
              runtime.calls == std::vector<std::string>{"ready"},
          "readiness forwards nonnull system manager result");
    runtime.calls.clear();
    runtime.bytes[kObject + 519u] = 0xe5u;
    Check(pal4::CombatSystemObject::SetProperty110(
              kObject, 0x12345678u, runtime) == 0x12345678u &&
              runtime.words[kObject + 440u] == 0x12345678u &&
              runtime.calls == std::vector<std::string>{"word:4536"} &&
              pal4::CombatSystemObject::GetProperty519(
                  kObject, runtime) == 0xe5u,
          "property 110 writes word +440 and getter reads byte +519");
}

}  // namespace

int main() {
    try {
        Construction();
        AllocationFailures();
        Destruction();
        TextCleanup();
        StateQueries();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
