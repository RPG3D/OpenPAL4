#include "pal4/native_base_object_member_runtime.h"
#include "pal4/native_base_object_memory.h"
#include "support/rw_dict_schema_fake.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Memory final : pal4::IGameConfigMemory {
    std::uint32_t Free(std::uint32_t address) override { return address; }
};

struct Schema final : pal4::test::RwDictSchemaFake {
    unsigned resets{};
    std::uint32_t SetCurrentDict(std::uint32_t dict) override {
        Check(dict == 0u, "resource cleanup resets dictionary");
        ++resets;
        return 0u;
    }
};

struct Animation final : pal4::IGameObjectEmbeddedAnimationRuntime {
    unsigned initialized{};
    unsigned cleaned{};
    void InitializeAnimationData(
        pal4::GameObjectEmbeddedAnimationState&) override { ++initialized; }
    void CleanupAnimationData(
        pal4::GameObjectEmbeddedAnimationState&) override { ++cleaned; }
    void CleanupAnimationDataRange(
        pal4::GameObjectEmbeddedAnimationState&) override {}
    std::uint32_t AnimationArrayCount(std::uint32_t) override { return 0u; }
    void DestructAnimationArray(std::uint32_t, std::uint32_t,
                                std::uint32_t) override {}
    std::uint32_t FreeMemory(std::uint32_t) override { return 0u; }
    void FreeWrappedMemory(std::uint32_t) override {}
    std::uint32_t FindHierarchy(std::uint32_t) override { return 0u; }
    void ProcessAnimationData(pal4::GameObjectEmbeddedAnimationState&,
                              std::uint32_t, std::int32_t) override {}
};

struct FailingText final : pal4::IStringBufferStorageRuntime {
    pal4::NativeBaseObjectMemory& storage;
    unsigned allocations{};
    explicit FailingText(pal4::NativeBaseObjectMemory& storage)
        : storage(storage) {}
    std::uint32_t Allocate(std::uint32_t size) override {
        if (++allocations == 2u)
            throw std::runtime_error("text allocation failure");
        return storage.Allocate(size);
    }
    std::uint32_t Free(std::uint32_t address) override {
        return storage.Free(address);
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return storage.ReadDword(address);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        storage.WriteDword(address, value);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        storage.WriteByte(address, value);
    }
};

struct FailingPath final : pal4::ID3dFacePathRuntime {
    std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    std::uint32_t Allocate(std::uint32_t) override {
        throw std::runtime_error("collision allocation failure");
    }
    void Free(std::uint32_t) override {}
};

void CheckConstructed(const pal4::BaseObjectState& state) {
    for (auto offset : {0u, 0x218u, 0x388u}) {
        const auto vtable = offset == 0u ? 0x8433d8u : 0x84b8c8u;
        Check(state.Read(offset) == vtable &&
                  state.Read(offset + 10u * 4u) == 1u &&
                  state.Read(offset + 39u * 4u) == 0x3f800000u,
              "all three resource manager images use the existing constructor");
    }
    Check(state.Read(0x170u + 12u * 4u) == 0x3f800000u,
          "embedded animation time scale initialized");
    for (auto offset : {0x520u, 0x528u, 0x534u, 0x53cu}) {
        Check(state.Read(offset) == 0xffffffffu &&
                  state.Read(offset + 4u) == 0u,
              "name node handles initialized");
    }
    for (auto offset : {0x544u, 0x554u}) {
        const auto text = state.Read(offset);
        Check(text != 0u && state.Read(offset + 4u) == 0u &&
                  state.Read(offset + 8u) == 32u &&
                  *reinterpret_cast<const char*>(
                      static_cast<std::uintptr_t>(text)) == '\0',
              "both embedded strings own empty 32-byte buffers");
    }
    const auto sentinel = state.Read(0x564u + 21u * 4u);
    Check(state.Read(0x564u) == pal4::CollisionActor::kVtableAddress &&
              sentinel != 0u &&
              *reinterpret_cast<const std::uint32_t*>(
                  static_cast<std::uintptr_t>(sentinel)) == sentinel,
          "collision actor owns a self-linked path sentinel");
}

}  // namespace

int main() {
    Memory memory;
    Schema schema;
    Animation animation;
    pal4::NativeBaseObjectMemory storage;
    pal4::NativeBaseObjectMemberRuntime members(memory, schema, animation,
                                                  storage, storage.Path());
    pal4::BaseObjectState state{};
    state.words.fill(0xdeadbeefu);
    const auto address = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&state));
    Check(pal4::BaseObjectLifecycle::Construct(state, address, members) ==
              address,
          "base constructor returns its object address");
    CheckConstructed(state);
    Check(animation.initialized == 1u &&
              storage.OutstandingAllocations() == 3u,
          "animation, two strings and collision sentinel are constructed");
    pal4::BaseObjectLifecycle::Destroy(state, members);
    Check(schema.resets == 3u && animation.cleaned == 1u &&
              state.Read(0x544u) == 0u && state.Read(0x554u) == 0u &&
              state.Read(0x564u + 21u * 4u) == 0u &&
              storage.OutstandingAllocations() == 0u,
          "base destructor releases members in reverse order");

    pal4::BaseObjectState failed{};
    pal4::NativeBaseObjectMemory failed_storage;
    FailingText failing_text(failed_storage);
    Schema failed_schema;
    Animation failed_animation;
    pal4::NativeBaseObjectMemberRuntime failing_members(
        memory, failed_schema, failed_animation, failing_text,
        failed_storage.Path());
    bool threw = false;
    try {
        (void)pal4::BaseObjectLifecycle::Construct(
            failed, address, failing_members);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && failed_schema.resets == 3u &&
              failed_animation.cleaned == 1u &&
              failed.Read(0x544u) == 0u &&
              failed_storage.OutstandingAllocations() == 0u,
          "failed text construction unwinds completed members");

    pal4::BaseObjectState failed_character{};
    pal4::NativeBaseObjectMemory character_storage;
    FailingPath failing_path;
    Schema character_schema;
    Animation character_animation;
    pal4::NativeBaseObjectMemberRuntime character_members(
        memory, character_schema, character_animation, character_storage,
        failing_path);
    threw = false;
    try {
        (void)pal4::BaseObjectLifecycle::Construct(
            failed_character, address, character_members);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && character_schema.resets == 3u &&
              character_animation.cleaned == 1u &&
              failed_character.Read(0x544u) == 0u &&
              failed_character.Read(0x554u) == 0u &&
              character_storage.OutstandingAllocations() == 0u,
          "failed collision construction releases both text buffers");
    std::cout << "PASS: native base object member lifecycle\n";
}
