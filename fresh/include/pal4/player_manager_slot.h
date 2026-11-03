#pragma once

#include <cstdint>

namespace pal4 {

class IPlayerManagerSlotRuntime {
public:
    virtual ~IPlayerManagerSlotRuntime() = default;
    virtual void BaseConstruct(std::uint32_t object) = 0;
    virtual void BaseReset(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupWeaponEffects(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t BaseDestroy(
        std::uint32_t object) = 0;
    virtual void InitializeSixWords(std::uint32_t address) = 0;
    virtual void ConstructBlockArray(std::uint32_t address,
                                     std::uint32_t count,
                                     std::uint32_t element_bytes) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyBlockArray(
        std::uint32_t address, std::uint32_t count,
        std::uint32_t element_bytes) = 0;
    virtual void InitializeScript(std::uint32_t address) = 0;
    virtual void CleanupScriptOnUnwind(std::uint32_t address) = 0;
    virtual void ResetScriptVtable(std::uint32_t address) = 0;
    virtual void InitializeResourceArrays(std::uint32_t address) = 0;
    virtual void CleanupResourceArraysOnUnwind(std::uint32_t address) = 0;
    virtual void DestroyResourceArray(std::uint32_t address,
                                      std::uint32_t count,
                                      std::uint32_t element_bytes) = 0;
    virtual void InitializeTail(std::uint32_t address) = 0;
    virtual void CleanupTail(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
};

class PlayerManagerSlotRuntimeStub : public IPlayerManagerSlotRuntime {
public:
    void BaseConstruct(std::uint32_t) override;
    void BaseReset(std::uint32_t) override;
    std::uint32_t CleanupWeaponEffects(std::uint32_t) override;
    std::uint32_t BaseDestroy(std::uint32_t) override;
    void InitializeSixWords(std::uint32_t) override;
    void ConstructBlockArray(std::uint32_t, std::uint32_t,
                             std::uint32_t) override;
    std::uint32_t DestroyBlockArray(std::uint32_t,
                                    std::uint32_t,
                                    std::uint32_t) override;
    void InitializeScript(std::uint32_t) override;
    void CleanupScriptOnUnwind(std::uint32_t) override;
    void ResetScriptVtable(std::uint32_t) override;
    void InitializeResourceArrays(std::uint32_t) override;
    void CleanupResourceArraysOnUnwind(std::uint32_t) override;
    void DestroyResourceArray(std::uint32_t, std::uint32_t,
                              std::uint32_t) override;
    void InitializeTail(std::uint32_t) override;
    void CleanupTail(std::uint32_t) override;
    std::uint32_t ReadDword(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    void WriteByte(std::uint32_t, std::uint8_t) override;
    void Free(std::uint32_t) override;
};

class PlayerManagerSlot final {
public:
    static constexpr std::uint32_t kObjectBytes = 0xb14u;
    static constexpr std::uint32_t kFirstBlockArrayOffset = 0x938u;
    static constexpr std::uint32_t kSecondBlockArrayOffset = 0x988u;
    static constexpr std::uint32_t kScriptOffset = 0x654u;
    static constexpr std::uint32_t kResourceArrayOffset = 0x9f8u;
    static constexpr std::uint32_t kTailOffset = 0xaa8u;
    static constexpr std::uint32_t kMainVtable = 0x842f10u;
    static constexpr std::uint32_t kAnimationVtable = 0x842f00u;
    static constexpr std::uint32_t kSecondaryVtable = 0x842ef4u;
    static constexpr std::uint32_t kScriptVtable = 0x842f60u;
    static constexpr std::uint32_t kScriptCleanupVtable = 0x8430e8u;

    // IDA 0x43e6a0: constructor callback for the four-element manager array.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint8_t incoming_stack_byte,
        IPlayerManagerSlotRuntime& runtime);
    // IDA 0x43e7d0: shared deleting destructor body reached from vtables.
    [[nodiscard]] static std::uint32_t DestroyDeleting(
        std::uint32_t object, std::uint32_t flags,
        IPlayerManagerSlotRuntime& runtime);
    // IDA 0x440e70: adjusted-this thunk from the script subobject.
    [[nodiscard]] static std::uint32_t DestroyDeletingFromScript(
        std::uint32_t script_subobject, std::uint32_t flags,
        IPlayerManagerSlotRuntime& runtime);
    // IDA 0x43e7f0: destructor for the +0x938 array subobject.
    [[nodiscard]] static std::uint32_t DestroyCollections(
        std::uint32_t collections, IPlayerManagerSlotRuntime& runtime);
    // IDA 0x43e890: destructor callback for one manager array element.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IPlayerManagerSlotRuntime& runtime);
    // IDA 0x43eda0: reset the inherited object, then clear the tail state.
    [[nodiscard]] static std::uint32_t Reset(
        std::uint32_t object, IPlayerManagerSlotRuntime& runtime);
    // IDA 0x43f400: thunk to the inherited weapon-effect cleanup.
    [[nodiscard]] static std::uint32_t ClearWeaponEffects(
        std::uint32_t object, IPlayerManagerSlotRuntime& runtime);
};

}  // namespace pal4
