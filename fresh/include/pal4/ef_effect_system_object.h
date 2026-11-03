#pragma once

#include <cstdint>

namespace pal4 {

struct EfEffectSystemConstructionFlags {
    std::uint8_t first{};
    std::uint8_t second{};
    std::uint8_t third{};
    std::uint8_t fourth{};
    std::uint8_t fifth{};
    std::uint8_t sixth{};
};

enum class EfEffectSystemConstructCall : std::uint8_t {
    InitializeTree12,
    InitializeTree10,
    ConstructTextureDictionary40,
    ConstructNamedDictionaryTree52,
    InitializeTree5,
    InitializeTree6,
    InitializeTree2,
    InitializeTree3,
    InitializeTree11,
    InitializeTime,
    InitializeTree4,
    InitializeTree,
};

class IEfEffectSystemConstructRuntime {
public:
    virtual ~IEfEffectSystemConstructRuntime() = default;
    [[nodiscard]] virtual EfEffectSystemConstructionFlags ScratchFlags() = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        EfEffectSystemConstructCall operation, std::uint32_t target,
        EfEffectSystemConstructionFlags& flags) = 0;
    [[nodiscard]] virtual std::uint32_t AuxiliarySingleton() = 0;
    virtual void SetAuxiliarySingleton(std::uint32_t object) = 0;
    virtual void ThrowDuplicateAuxiliary() = 0;
};

class EfEffectSystemConstructRuntimeStub
    : public IEfEffectSystemConstructRuntime {
public:
    [[nodiscard]] EfEffectSystemConstructionFlags ScratchFlags() override {
        return {};
    }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t Call(
        EfEffectSystemConstructCall, std::uint32_t,
        EfEffectSystemConstructionFlags&) override { return 0u; }
    [[nodiscard]] std::uint32_t AuxiliarySingleton() override { return 0u; }
    void SetAuxiliarySingleton(std::uint32_t) override {}
    void ThrowDuplicateAuxiliary() override {}
};

class IEfEffectSystemPrimaryTreeRuntime {
public:
    virtual ~IEfEffectSystemPrimaryTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseNilReference() = 0;
    virtual void ClearSharedNil() = 0;
    virtual void DeleteSubtree(std::uint32_t root) = 0;
    virtual void ReadFirstElement(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t Next(std::uint32_t node) = 0;
    virtual void EraseNode(std::uint32_t tree,
                           std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

enum class EfEffectSystemOwnedTree : std::uint8_t {
    At148,
    At92,
    At24,
    At8,
};

enum class EfEffectSystemDestructCall : std::uint8_t {
    CleanupState,
    DeinitializePool,
    CleanupPoolObjects,
    CleanupTree124,
    CleanupTree108,
    CleanupInstances68,
    CleanupTree68,
    DestroyNamedDictionaryTree52,
    DestroyDictionary40,
    CleanupTree24,
    CleanupTree8,
};

class IEfEffectSystemDestructRuntime {
public:
    virtual ~IEfEffectSystemDestructRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void DestroyOwned(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t FirstElement(
        std::uint32_t tree) = 0;
    virtual void RemoveRange(EfEffectSystemOwnedTree kind,
                             std::uint32_t tree, std::uint32_t first,
                             std::uint32_t head,
                             std::uint32_t context) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseNilReference(
        EfEffectSystemOwnedTree kind) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil(
        EfEffectSystemOwnedTree kind) = 0;
    virtual void ClearSharedNil(EfEffectSystemOwnedTree kind) = 0;
    [[nodiscard]] virtual std::uint32_t Call(
        EfEffectSystemDestructCall operation,
        std::uint32_t target) = 0;
};

class IEfEffectSystemCleanupRuntime {
public:
    virtual ~IEfEffectSystemCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    virtual void CleanupEffect(std::uint32_t effect) = 0;
    virtual void CleanupInstances(std::uint32_t collection) = 0;
    virtual void SetPoolFlags(std::uint32_t pool,
                              std::uint32_t flags) = 0;
};

class IEfEffectSystemEnableRuntime {
public:
    virtual ~IEfEffectSystemEnableRuntime() = default;
    virtual void LogStart() = 0;
    virtual void CleanupState(std::uint32_t object) = 0;
    virtual void InitializeEvents(std::uint32_t object) = 0;
    virtual void RegisterEvents(std::uint32_t object) = 0;
    virtual void InitializeTextureDictionary(
        std::uint32_t dictionary) = 0;
    virtual void SetPoolFlags(std::uint32_t pool,
                              std::uint32_t flags) = 0;
    virtual void CleanupPoolObjects(std::uint32_t pool) = 0;
    virtual void SetTimeAndFlag(double seconds) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void LogComplete() = 0;
};

enum class EfEffectSystemStateTree : std::uint8_t { At8, At24 };

class IEfEffectSystemStateRuntime {
public:
    virtual ~IEfEffectSystemStateRuntime() = default;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void DeinitializePool(std::uint32_t pool) = 0;
    virtual void CleanupDictionary40(std::uint32_t dictionary) = 0;
    virtual void CleanupNamedDictionaryTree52(std::uint32_t dictionary) = 0;
    virtual void CleanupInstances(std::uint32_t collection) = 0;
    virtual void DeleteTreeSubtree(EfEffectSystemStateTree kind,
                                   std::uint32_t root) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil(
        EfEffectSystemStateTree kind) = 0;
    virtual void CleanupAuxiliary(std::uint32_t auxiliary) = 0;
};

class IEfEffectSystemAuxiliaryRuntime {
public:
    virtual ~IEfEffectSystemAuxiliaryRuntime() = default;
    virtual void WriteVtable(std::uint32_t object,
                             std::uint32_t vtable) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentSingleton() = 0;
    virtual void ThrowMissingSingleton() = 0;
    virtual void ClearSingleton() = 0;
    virtual void Free(std::uint32_t object) = 0;
};

enum class EfEffectSystemAuxiliaryTree : std::uint8_t {
    At28,
    At12,
};

class IEfEffectSystemAuxiliaryDestructRuntime
    : public IEfEffectSystemAuxiliaryRuntime {
public:
    ~IEfEffectSystemAuxiliaryDestructRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void CleanupEffects(std::uint32_t object) = 0;
    virtual void RemoveRange(EfEffectSystemAuxiliaryTree kind,
                             std::uint32_t tree, std::uint32_t first,
                             std::uint32_t head) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseNilReference(
        EfEffectSystemAuxiliaryTree kind) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil(
        EfEffectSystemAuxiliaryTree kind) = 0;
    virtual void ClearSharedNil(EfEffectSystemAuxiliaryTree kind) = 0;
};

class EfEffectSystemAuxiliary final {
public:
    // Shared auxiliary portion of IDA 0x635df0, operating on allocated storage.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, EfEffectSystemConstructionFlags& flags,
        IEfEffectSystemConstructRuntime& runtime);
    // IDA 0x636790: destructor/unwind path removes singleton registration.
    [[nodiscard]] static std::uint32_t FinalizeSingleton(
        std::uint32_t object, IEfEffectSystemAuxiliaryRuntime& runtime);
    // IDA 0x636eb0: deleting destructor for the auxiliary singleton.
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t object, std::uint8_t flags,
        IEfEffectSystemAuxiliaryRuntime& runtime);
    // IDA 0x638bb0: full auxiliary object destruction.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object,
        IEfEffectSystemAuxiliaryDestructRuntime& runtime);
};

class EfEffectSystemObject final {
public:
    static constexpr std::uint32_t kObjectSize = 0xa8u;
    static constexpr std::uint32_t kAuxiliaryVtableDuringInit = 0x84b1c0u;
    static constexpr std::uint32_t kAuxiliaryVtableReady = 0x84b1bcu;

    // IDA 0x635df0: initialize owned trees, dictionaries and subobjects.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, IEfEffectSystemConstructRuntime& runtime);
    // IDA 0x635fd0: destructor-path cleanup of the primary tree.
    [[nodiscard]] static std::uint32_t CleanupPrimaryTree(
        std::uint32_t tree, IEfEffectSystemPrimaryTreeRuntime& runtime);
    // IDA 0x636090: reverse-order teardown of the 168-byte singleton.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, IEfEffectSystemDestructRuntime& runtime);
    // IDA 0x636540: clean registered effects, instances and pool flags.
    static void CleanupRegisteredEffects(
        std::uint32_t object, IEfEffectSystemCleanupRuntime& runtime);
    // IDA 0x6365c0: initialize active effect-system state and events.
    [[nodiscard]] static std::uint8_t Enable(
        std::uint32_t object, bool set_time,
        IEfEffectSystemEnableRuntime& runtime);
    // IDA 0x636670: release active state while retaining object storage.
    static void CleanupState(std::uint32_t object,
                             IEfEffectSystemStateRuntime& runtime);
};

}  // namespace pal4
