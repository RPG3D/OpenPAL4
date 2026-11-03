#include "pal4/player_manager_slot.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::size_t IndexOf(const std::vector<std::string>& events,
                    const std::string& name) {
    for (std::size_t i = 0; i < events.size(); ++i)
        if (events[i] == name) return i;
    Check(false, "expected runtime event is missing");
    return events.size();
}

struct Runtime final : pal4::PlayerManagerSlotRuntimeStub {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::string throw_at;

    void Event(const std::string& name) {
        events.push_back(name);
        if (name == throw_at) throw std::runtime_error(name);
    }
    void BaseConstruct(std::uint32_t object) override {
        Check(object == 0x5000u, "base constructor object");
        Event("base-construct");
    }
    void BaseReset(std::uint32_t object) override {
        Check(object == 0x5000u, "base reset object");
        Event("base-reset");
    }
    std::uint32_t CleanupWeaponEffects(
        std::uint32_t object) override {
        Check(object == 0x5000u, "weapon cleanup object");
        Event("cleanup-weapon-effects");
        return 0x5555u;
    }
    std::uint32_t BaseDestroy(std::uint32_t object) override {
        Check(object == 0x5000u, "base destructor object");
        Event("base-destroy");
        return 0x9876u;
    }
    void InitializeSixWords(std::uint32_t address) override {
        Check(address == 0x5920u, "six-word member address");
        Event("six-words");
    }
    void ConstructBlockArray(std::uint32_t address, std::uint32_t count,
                             std::uint32_t bytes_per_element) override {
        Check((address == 0x5938u || address == 0x5988u) &&
                  count == 5u && bytes_per_element == 16u,
              "block array construction operands");
        Event(address == 0x5938u ? "construct-first" : "construct-second");
    }
    std::uint32_t DestroyBlockArray(std::uint32_t address,
                                    std::uint32_t count,
                                    std::uint32_t bytes_per_element) override {
        Check((address == 0x5938u || address == 0x5988u) &&
                  count == 5u && bytes_per_element == 16u,
              "block array destruction operands");
        Event(address == 0x5938u ? "destroy-first" : "destroy-second");
        return address == 0x5938u ? 0x1111u : 0x2222u;
    }
    void InitializeScript(std::uint32_t address) override {
        Check(address == 0x5654u, "script member address");
        Event("initialize-script");
    }
    void CleanupScriptOnUnwind(std::uint32_t address) override {
        Check(address == 0x5654u, "script unwind member address");
        Event("unwind-script");
    }
    void ResetScriptVtable(std::uint32_t address) override {
        Check(address == 0x5654u, "script vtable reset address");
        Event("reset-script-vtable");
    }
    void InitializeResourceArrays(std::uint32_t address) override {
        Check(address == 0x59f8u, "resource arrays member address");
        Event("initialize-resources");
    }
    void CleanupResourceArraysOnUnwind(std::uint32_t address) override {
        Check(address == 0x59f8u, "resource array unwind address");
        Event("unwind-resources");
    }
    void DestroyResourceArray(std::uint32_t address,
                              std::uint32_t count,
                              std::uint32_t bytes_per_element) override {
        Check((address == 0x5a58u && count == 5u) ||
                  (address == 0x59f8u && count == 4u),
              "resource array address and count");
        Check(bytes_per_element == 16u, "resource array record width");
        Event(address == 0x5a58u ? "destroy-resources-second"
                                 : "destroy-resources-first");
    }
    void InitializeTail(std::uint32_t address) override {
        Check(address == 0x5aa8u, "tail member address");
        Event("initialize-tail");
    }
    void CleanupTail(std::uint32_t address) override {
        Check(address == 0x5aa8u, "tail cleanup address");
        Event("cleanup-tail");
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        Event("read:" + std::to_string(address));
        return words[address];
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        words[address] = value;
        Event("write:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        Event("byte:" + std::to_string(address));
    }
    void Free(std::uint32_t address) override {
        Event("free:" + std::to_string(address));
    }
};

void ConstructorSuccess() {
    Runtime runtime;
    runtime.words[0x5b04u] = 0xdeadu;
    Check(pal4::PlayerManagerSlot::kObjectBytes == 0xb14u &&
              pal4::PlayerManagerSlot::Construct(
                  0x5000u, 0x5au, runtime) == 0x5000u,
          "array element constructor returns its 0xb14-byte object");
    Check(runtime.events[0] == "base-construct" &&
              runtime.events[1] == "six-words" &&
              runtime.events[2] == "construct-first" &&
              runtime.events[3] == "construct-second" &&
              IndexOf(runtime.events, "initialize-script") <
                  IndexOf(runtime.events, "initialize-resources") &&
              IndexOf(runtime.events, "initialize-resources") <
                  IndexOf(runtime.events, "initialize-tail"),
          "constructor initializes members in IDA order");
    Check(runtime.bytes[0x59d8u] == 0x5au &&
              runtime.bytes[0x59e8u] == 0x5au &&
              runtime.words[0x59dcu] == 0u &&
              runtime.words[0x59e0u] == 0u &&
              runtime.words[0x59e4u] == 0u &&
              runtime.words[0x59ecu] == 0u &&
              runtime.words[0x59f0u] == 0u &&
              runtime.words[0x59f4u] == 0u,
          "stack byte and six adjacent pointer fields are written exactly");
    Check(runtime.words[0x5000u] == 0x842f10u &&
              runtime.words[0x5170u] == 0x842f00u &&
              runtime.words[0x51ecu] == 0x842ef4u &&
              runtime.words[0x5654u] == 0x842f60u &&
              runtime.words[0x5b08u] == 0u &&
              runtime.words[0x5b0cu] == 1u &&
              runtime.words[0x5b10u] == 0u &&
              runtime.words[0x5b04u] == 0xdeadu,
          "constructor installs four vtables and final flags");
}

void ConstructorUnwind() {
    struct Case {
        const char* failure;
        std::vector<std::string> cleanup;
    };
    const std::vector<Case> cases{
        {"base-construct", {}},
        {"six-words", {"base-destroy"}},
        {"construct-first", {"base-destroy"}},
        {"construct-second", {"destroy-first", "base-destroy"}},
        {"initialize-script", {"destroy-second", "destroy-first",
                                "base-destroy"}},
        {"initialize-resources", {"unwind-script", "destroy-second",
                                   "destroy-first", "base-destroy"}},
        {"initialize-tail", {"unwind-resources", "unwind-script",
                              "destroy-second", "destroy-first",
                              "base-destroy"}},
    };
    for (const auto& scenario : cases) {
        Runtime runtime;
        runtime.throw_at = scenario.failure;
        try {
            static_cast<void>(pal4::PlayerManagerSlot::Construct(
                0x5000u, 0x5au, runtime));
            Check(false, "injected constructor failure must propagate");
        } catch (const std::runtime_error&) {
            Check(IndexOf(runtime.events, scenario.failure) <
                      runtime.events.size(),
                  "failure happened at the selected construction stage");
            std::vector<std::string> cleanup;
            for (const auto& event : runtime.events) {
                if (event == "unwind-resources" ||
                    event == "unwind-script" ||
                    event == "destroy-second" ||
                    event == "destroy-first" ||
                    event == "base-destroy")
                    cleanup.push_back(event);
            }
            Check(cleanup == scenario.cleanup,
                  "constructor follows IDA unwind state map");
        }
    }
}

void CollectionCleanup() {
    Runtime runtime;
    runtime.words[0x59ecu] = 0x8000u;
    runtime.words[0x59dcu] = 0x9000u;
    Check(pal4::PlayerManagerSlot::DestroyCollections(
              0x5938u, runtime) == 0x1111u,
          "collection cleanup returns the first array destructor result");
    Check(IndexOf(runtime.events, "free:32768") <
              IndexOf(runtime.events, "free:36864") &&
              IndexOf(runtime.events, "free:36864") <
              IndexOf(runtime.events, "destroy-second") &&
              IndexOf(runtime.events, "destroy-second") <
              IndexOf(runtime.events, "destroy-first"),
          "collection cleanup frees two buffers before reverse array teardown");
    for (const auto offset : {0x9dcu, 0x9e0u, 0x9e4u,
                              0x9ecu, 0x9f0u, 0x9f4u})
        Check(runtime.words[0x5000u + offset] == 0u,
              "collection cleanup clears both pointer triples");
}

void DestructorSuccess() {
    Runtime runtime;
    runtime.words[0x590cu] = 0x8000u;
    runtime.words[0x58fcu] = 0x8100u;
    runtime.words[0x59ecu] = 0x8200u;
    runtime.words[0x59dcu] = 0x8300u;
    Check(pal4::PlayerManagerSlot::Destroy(0x5000u, runtime) == 0x9876u,
          "slot destructor forwards base destructor result");
    Check(runtime.words[0x5000u] == 0x842f10u &&
              runtime.words[0x5170u] == 0x842f00u &&
              runtime.words[0x51ecu] == 0x842ef4u &&
              runtime.words[0x5654u] == 0x8430e8u,
          "slot destructor restores vtables before member cleanup");
    Check(IndexOf(runtime.events, "cleanup-tail") <
              IndexOf(runtime.events, "destroy-resources-second") &&
              IndexOf(runtime.events, "destroy-resources-second") <
              IndexOf(runtime.events, "destroy-resources-first") &&
              IndexOf(runtime.events, "destroy-resources-first") <
              IndexOf(runtime.events, "free:32768") &&
              IndexOf(runtime.events, "free:32768") <
              IndexOf(runtime.events, "free:33024") &&
              IndexOf(runtime.events, "free:33024") <
              IndexOf(runtime.events, "reset-script-vtable") &&
              IndexOf(runtime.events, "reset-script-vtable") <
              IndexOf(runtime.events, "destroy-second") &&
              runtime.events.back() == "base-destroy",
          "slot destructor follows tail, arrays, script, collections, base order");
    for (const auto offset : {0x8fcu, 0x900u, 0x904u,
                              0x90cu, 0x910u, 0x914u,
                              0x9dcu, 0x9e0u, 0x9e4u,
                              0x9ecu, 0x9f0u, 0x9f4u})
        Check(runtime.words[0x5000u + offset] == 0u,
              "slot destructor clears owned buffer triples");
}

void DeletingDestructor() {
    for (const auto flags : {0u, 1u, 2u, 3u}) {
        Runtime runtime;
        const auto result = pal4::PlayerManagerSlot::DestroyDeleting(
            0x5000u, flags, runtime);
        Check(result == 0x5000u &&
                  IndexOf(runtime.events, "base-destroy") <
                      runtime.events.size(),
              "deleting destructor returns original object after cleanup");
        const bool should_free = (flags & 1u) != 0u;
        bool freed = false;
        for (const auto& event : runtime.events)
            if (event == "free:20480") freed = true;
        Check(freed == should_free,
              "only bit zero requests final object free");
        if (should_free)
            Check(runtime.events.back() == "free:20480",
                  "object free follows the complete destructor");
    }
    Runtime runtime;
    Check(pal4::PlayerManagerSlot::DestroyDeletingFromScript(
              0x5000u + pal4::PlayerManagerSlot::kScriptOffset,
              1u, runtime) == 0x5000u &&
              runtime.events.back() == "free:20480",
          "0x440e70 adjusts script this before shared deleting destructor");
}

void ResetState() {
    Runtime runtime;
    for (std::uint32_t offset = 0xae8u; offset <= 0xb0cu;
         offset += 4u)
        runtime.words[0x5000u + offset] = 0x12345678u;
    Check(pal4::PlayerManagerSlot::Reset(0x5000u, runtime) == 0u,
          "reset returns zero regardless of base reset");
    Check(runtime.events.front() == "base-reset" &&
              runtime.events.size() == 9u,
          "base reset precedes eight zero writes");
    for (std::uint32_t offset = 0xaecu; offset <= 0xb08u;
         offset += 4u)
        Check(runtime.words[0x5000u + offset] == 0u,
              "reset clears seven DWORDs and final state word");
    Check(runtime.words[0x5ae8u] == 0x12345678u &&
              runtime.words[0x5b0cu] == 0x12345678u &&
              runtime.events[1] == "write:23276" &&
              runtime.events.back() == "write:23304",
          "reset preserves neighboring words and writes in address order");
}

void WeaponCleanupThunk() {
    Runtime runtime;
    Check(pal4::PlayerManagerSlot::ClearWeaponEffects(
              0x5000u, runtime) == 0x5555u &&
              runtime.events == std::vector<std::string>{
                  "cleanup-weapon-effects"},
          "weapon cleanup thunk forwards object and return unchanged");
}

}  // namespace

int main() {
    ConstructorSuccess();
    ConstructorUnwind();
    CollectionCleanup();
    DestructorSuccess();
    DeletingDestructor();
    ResetState();
    WeaponCleanupThunk();
    std::cout << "7 player-manager slot test groups passed\n";
}
