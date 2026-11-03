#include "pal4/script_enum_object_lifecycle.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IScriptEnumObjectLifecycleRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        calls.push_back("read8:" + std::to_string(address));
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("write8:" + std::to_string(address) + ":" +
                        std::to_string(value));
        bytes[address] = value;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("read32:" + std::to_string(address));
        return words.at(address);
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    void DestroyAdapter(std::uint32_t address,
                        std::uint32_t flags) override {
        calls.push_back("adapter:" + std::to_string(address) + ":" +
                        std::to_string(flags));
    }
    void CleanupEmptyTemporary() override {
        calls.push_back("temporary");
    }
    void DestroyArray(std::uint32_t address, std::uint32_t count,
                      std::uint32_t stride) override {
        calls.push_back("array:" + std::to_string(address) + ":" +
                        std::to_string(count) + ":" +
                        std::to_string(stride));
    }
};

struct ForwardRuntime final : pal4::IScriptEnumForwardRuntime {
    std::vector<std::string> calls;

    std::uint32_t Invoke(std::uint32_t target,
                         std::uint32_t vtable_slot,
                         std::uint32_t source_object) override {
        calls.push_back("invoke:" + std::to_string(target) + ":" +
                        std::to_string(vtable_slot) + ":" +
                        std::to_string(source_object));
        return vtable_slot + 0x100u;
    }
};

void TestObjectDestruction() {
    pal4::ScriptEnumObjectState state;
    state.words = {0x842ba4u, 0xabcdef01u, 0x2000u,
                   0x12345601u, 0x3000u, 0x99u,
                   0x4000u, 5u, 8u};
    Runtime runtime;
    runtime.bytes[0x3fffu] = 2u;
    pal4::ScriptEnumObjectLifecycle::Destroy(state, runtime);
    Check(state.words == std::array<std::uint32_t, 9>{
              0x842bd4u, 0xabcdef00u, 0u, 0x12345600u, 0u,
              0x99u, 0u, 0u, 0u} &&
              runtime.calls == std::vector<std::string>{
                  "read8:16383", "write8:16383:1", "adapter:8192:1",
                  "adapter:12288:1", "temporary"},
          "destructor releases string, both adapters and installs base vtable");
    Check(pal4::ScriptEnumObjectLifecycle::StringSubobjectAddress(
              0x5000u) == 0x5014u,
          "getter returns the 16-byte subobject at offset twenty");
}

void TestEntryConstruction() {
    pal4::ScriptEnumObjectState state;
    state.words = {0xaaaaaaaau, 0x111111ffu, 0x22222222u,
                   0x333333ffu, 0x44444444u, 0x55555555u,
                   0x66666666u, 0x77777777u, 0x88888888u};
    Check(pal4::ScriptEnumObjectLifecycle::ConstructEntry(
              state, 0xab123456u) == 0xab123456u &&
              state.words == std::array<std::uint32_t, 9>{
                  0x8481d0u, 0x11111100u, 0u,
                  0x33333300u, 0u, 0x555555abu,
                  0u, 0u, 0u},
          "36-byte constructor preserves untouched flag bytes and uses pointer high byte");

    pal4::ScriptEnumAdapterPairState pair{
        {0xabcdefffu, 0x11111111u, 0x123456ffu, 0x22222222u}};
    const auto* address = &pair;
    Check(&pal4::ScriptEnumObjectLifecycle::InitializeAdapterPair(pair) ==
              address &&
              pair.words == std::array<std::uint32_t, 4>{
                  0xabcdef00u, 0u, 0x12345600u, 0u},
          "16-byte adapter constructor clears only flag bytes and pointers");
}

void TestBindingDestructors() {
    for (auto destroy : {
             &pal4::ScriptEnumObjectLifecycle::DestroyValueBinding,
             &pal4::ScriptEnumObjectLifecycle::DestroyReferenceBinding}) {
        pal4::ScriptEnumObjectState state;
        state.words = {0x99999999u, 0xabcdef01u, 0x2000u,
                       0x12345601u, 0x3000u, 0x77u,
                       0x4000u, 5u, 8u};
        Runtime runtime;
        runtime.bytes[0x3fffu] = 2u;
        Check(destroy(state, runtime) == 0u &&
                  state.words == std::array<std::uint32_t, 9>{
                      0x842bd4u, 0xabcdef00u, 0u,
                      0x12345600u, 0u, 0x77u,
                      0u, 0u, 0u} &&
                  runtime.calls == std::vector<std::string>{
                      "adapter:8192:1", "adapter:12288:1",
                      "read8:16383", "write8:16383:1"},
              "derived destructors release adapters before the name buffer");

        state.words = {0u, 0xabcdef01u, 0u,
                       0x12345600u, 0x3000u, 0u,
                       0x4000u, 1u, 1u};
        runtime.calls.clear();
        runtime.bytes[0x3fffu] = 0xffu;
        Check(destroy(state, runtime) == 1u &&
                  state.words[1] == 0xabcdef01u &&
                  runtime.calls == std::vector<std::string>{
                      "read8:16383", "free:16383"},
              "null first adapter preserves its flag and saturated text is freed");
    }

    for (auto destroy : {
             &pal4::ScriptEnumObjectLifecycle::DestroyAdapterPairPrimary,
             &pal4::ScriptEnumObjectLifecycle::DestroyAdapterPairSecondary}) {
        pal4::ScriptEnumAdapterPairState pair{
            {0xabcdef01u, 0x2000u, 0x12345601u, 0x3000u}};
        Runtime runtime;
        Check(destroy(pair, runtime) == 0u &&
                  pair.words == std::array<std::uint32_t, 4>{
                      0xabcdef00u, 0u, 0x12345600u, 0u} &&
                  runtime.calls == std::vector<std::string>{
                      "adapter:8192:1", "adapter:12288:1"},
              "both adapter-pair destructor copies release in slot order");
    }
    Check(pal4::ScriptEnumObjectLifecycle::BindingNameAddress(
              0x5000u) == 0x5014u,
          "binding name getter returns the embedded string at +20");
}

void TestAdapterPairCopies() {
    constexpr std::array<std::uint32_t, 11> entries{{
        0x586b20u, 0x586bf0u, 0x586cc0u, 0x586d90u,
        0x586e60u, 0x586f30u, 0x587000u, 0x5870d0u,
        0x5871a0u, 0x587270u, 0x587340u}};
    for (const auto address : entries) {
        pal4::ScriptEnumAdapterPairState pair{
            {0xabcdef01u, 0x2000u, 0x12345601u, 0x3000u}};
        Runtime runtime;
        Check(pal4::ScriptEnumObjectLifecycle::DestroyAdapterPairAt(
                  address, pair, runtime) == 0u &&
                  pair.words == std::array<std::uint32_t, 4>{
                      0xabcdef00u, 0u, 0x12345600u, 0u} &&
                  runtime.calls == std::vector<std::string>{
                      "adapter:8192:1", "adapter:12288:1"},
              "each compiled adapter-pair entry has the same cleanup behavior");
    }
    pal4::ScriptEnumAdapterPairState pair{};
    Runtime runtime;
    bool rejected = false;
    try {
        static_cast<void>(
            pal4::ScriptEnumObjectLifecycle::DestroyAdapterPairAt(
                0x586b21u, pair, runtime));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    Check(rejected && runtime.calls.empty(),
          "address dispatcher rejects non-entry addresses");
}

void TestNullAndUnownedMembers() {
    pal4::ScriptEnumObjectState state;
    state.words = {0x842ba4u, 0xabcdef01u, 0u,
                   0x12345600u, 0x3000u, 0u,
                   0x4000u, 5u, 8u};
    Runtime runtime;
    runtime.bytes[0x3fffu] = 0xffu;
    pal4::ScriptEnumObjectLifecycle::Destroy(state, runtime);
    Check(state.words[1] == 0xabcdef01u &&
              state.words[3] == 0x12345600u && state.words[4] == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read8:16383", "free:16383", "temporary"},
          "null owned member keeps its flag; unowned member clears its pointer");
}

void TestDeletingFlags() {
    pal4::ScriptEnumObjectState state;
    state.words[0] = 0x842ba4u;
    Runtime runtime;
    Check(pal4::ScriptEnumObjectLifecycle::DeletingDestroy(
              state, 0x5000u, 1u, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  "temporary", "free:20480"} &&
              state.words[0] == 0x842bd4u,
          "single-object deleting destructor runs cleanup before free");

    runtime.calls.clear();
    state.words[0] = 0x842ba4u;
    runtime.words[0x4ffcu] = 3u;
    Check(pal4::ScriptEnumObjectLifecycle::DeletingDestroy(
              state, 0x5000u, 2u, runtime) == 0x4ffcu &&
              runtime.calls == std::vector<std::string>{
                  "read32:20476", "array:20480:3:36"} &&
              state.words[0] == 0x842ba4u,
          "array flag uses the count word and leaves element cleanup to iterator");

    runtime.calls.clear();
    Check(pal4::ScriptEnumObjectLifecycle::DeletingDestroy(
              state, 0x5000u, 3u, runtime) == 0x4ffcu &&
              runtime.calls == std::vector<std::string>{
                  "read32:20476", "array:20480:3:36", "free:20476"},
          "combined array and free flags release the count header");
}

void TestAdapterPairCleanup() {
    pal4::ScriptEnumAdapterPairState pair{
        {0xabcdef01u, 0x2000u, 0x12345601u, 0x3000u}};
    Runtime runtime;
    Check(pal4::ScriptEnumObjectLifecycle::CleanupAdapterPair(
              pair, runtime) == 0u &&
              pair.words == std::array<std::uint32_t, 4>{
                  0xabcdef00u, 0u, 0x12345600u, 0u} &&
              runtime.calls == std::vector<std::string>{
                  "adapter:8192:1", "adapter:12288:1"},
          "adapter-pair cleanup destroys both owned pointers in order");

    pair.words = {0xabcdef01u, 0u, 0x12345600u, 0x3000u};
    runtime.calls.clear();
    Check(pal4::ScriptEnumObjectLifecycle::CleanupAdapterPair(
              pair, runtime) == 1u &&
              pair.words == std::array<std::uint32_t, 4>{
                  0xabcdef01u, 0u, 0x12345600u, 0u} &&
              runtime.calls.empty(),
          "null first pointer retains its flag and determines return byte");
}

void TestBaseDeleteAndVirtualForwarding() {
    pal4::ScriptEnumObjectState state;
    state.words[0] = 0x842ba4u;
    Runtime runtime;
    Check(pal4::ScriptEnumObjectLifecycle::DeleteBase(
              state, 0x5000u, 2u, runtime) == 0x5000u &&
              state.words[0] == 0x842bd4u && runtime.calls.empty(),
          "base deleting destructor always installs base vtable");
    Check(pal4::ScriptEnumObjectLifecycle::DeleteBase(
              state, 0x5000u, 1u, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{"free:20480"},
          "base deleting destructor frees only when bit zero is set");

    ForwardRuntime forwarding;
    Check(pal4::ScriptEnumObjectLifecycle::ForwardSlot12(
              0x3000u, 0x4000u, forwarding) == 268u &&
              pal4::ScriptEnumObjectLifecycle::ForwardSlot36(
                  0x3000u, 0x4000u, forwarding) == 292u &&
              forwarding.calls == std::vector<std::string>{
                  "invoke:16384:12:12288", "invoke:16384:36:12288"},
          "forwarders pass this as argument to target virtual slots");
}

}  // namespace

int main() {
    TestEntryConstruction();
    TestBindingDestructors();
    TestAdapterPairCopies();
    TestObjectDestruction();
    TestNullAndUnownedMembers();
    TestDeletingFlags();
    TestAdapterPairCleanup();
    TestBaseDeleteAndVirtualForwarding();
    pal4::ScriptEnumObjectState state;
    pal4::ScriptEnumObjectLifecycleRuntimeStub stub;
    Check(pal4::ScriptEnumObjectLifecycle::DeletingDestroy(
              state, 0x1000u, 0u, stub) == 0x1000u,
          "default lifecycle runtime links");
    pal4::ScriptEnumForwardRuntimeStub forward_stub;
    Check(pal4::ScriptEnumObjectLifecycle::ForwardSlot12(
              0u, 0u, forward_stub) == 0u,
          "default forwarding runtime links");
    std::cout << "9 script enum object lifecycle cases passed\n";
}
