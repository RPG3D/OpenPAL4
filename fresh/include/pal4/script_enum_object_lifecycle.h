#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct ScriptEnumObjectState {
    std::array<std::uint32_t, 9> words{};
};
static_assert(sizeof(ScriptEnumObjectState) == 36u);

struct ScriptEnumAdapterPairState {
    std::array<std::uint32_t, 4> words{};
};
static_assert(sizeof(ScriptEnumAdapterPairState) == 16u);

class IScriptEnumObjectLifecycleRuntime {
public:
    virtual ~IScriptEnumObjectLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void DestroyAdapter(std::uint32_t address,
                                std::uint32_t flags) = 0;
    virtual void CleanupEmptyTemporary() = 0;
    virtual void DestroyArray(std::uint32_t address,
                              std::uint32_t count,
                              std::uint32_t stride) = 0;
};

class ScriptEnumObjectLifecycleRuntimeStub final
    : public IScriptEnumObjectLifecycleRuntime {
public:
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void Free(std::uint32_t address) override;
    void DestroyAdapter(std::uint32_t address,
                        std::uint32_t flags) override;
    void CleanupEmptyTemporary() override;
    void DestroyArray(std::uint32_t address, std::uint32_t count,
                      std::uint32_t stride) override;
};

class IScriptEnumForwardRuntime {
public:
    virtual ~IScriptEnumForwardRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Invoke(
        std::uint32_t target, std::uint32_t vtable_slot,
        std::uint32_t source_object) = 0;
};

class ScriptEnumForwardRuntimeStub final : public IScriptEnumForwardRuntime {
public:
    std::uint32_t Invoke(std::uint32_t target,
                         std::uint32_t vtable_slot,
                         std::uint32_t source_object) override;
};

class ScriptEnumObjectLifecycle final {
public:
    static constexpr std::uint32_t kBaseVtable = 0x842bd4u;

    // IDA 0x5852c0 / 0x585330.
    [[nodiscard]] static std::uint32_t ConstructEntry(
        ScriptEnumObjectState& state, std::uint32_t object_address);
    [[nodiscard]] static ScriptEnumAdapterPairState& InitializeAdapterPair(
        ScriptEnumAdapterPairState& state);
    // IDA 0x5854b0 / 0x5856f0.
    [[nodiscard]] static std::uint8_t DestroyValueBinding(
        ScriptEnumObjectState& state,
        IScriptEnumObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t DestroyReferenceBinding(
        ScriptEnumObjectState& state,
        IScriptEnumObjectLifecycleRuntime& runtime);
    // IDA 0x585620 / 0x585890.
    [[nodiscard]] static std::uint8_t DestroyAdapterPairPrimary(
        ScriptEnumAdapterPairState& state,
        IScriptEnumObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t DestroyAdapterPairSecondary(
        ScriptEnumAdapterPairState& state,
        IScriptEnumObjectLifecycleRuntime& runtime);
    // IDA 0x586b20..0x587340: eleven identical compiled copies.
    [[nodiscard]] static std::uint8_t DestroyAdapterPairAt(
        std::uint32_t native_entry, ScriptEnumAdapterPairState& state,
        IScriptEnumObjectLifecycleRuntime& runtime);
    // IDA 0x585880.
    [[nodiscard]] static std::uint32_t BindingNameAddress(
        std::uint32_t object_address);

    [[nodiscard]] static std::uint32_t StringSubobjectAddress(
        std::uint32_t object_address);
    static void Destroy(ScriptEnumObjectState& state,
                        IScriptEnumObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeletingDestroy(
        ScriptEnumObjectState& state, std::uint32_t object_address,
        std::uint8_t flags, IScriptEnumObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint8_t CleanupAdapterPair(
        ScriptEnumAdapterPairState& state,
        IScriptEnumObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeleteBase(
        ScriptEnumObjectState& state, std::uint32_t object_address,
        std::uint8_t flags, IScriptEnumObjectLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot12(
        std::uint32_t source_object, std::uint32_t target,
        IScriptEnumForwardRuntime& runtime);
    [[nodiscard]] static std::uint32_t ForwardSlot36(
        std::uint32_t source_object, std::uint32_t target,
        IScriptEnumForwardRuntime& runtime);
};

}  // namespace pal4
