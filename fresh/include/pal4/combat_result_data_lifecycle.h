#pragma once

#include <cstdint>

namespace pal4 {

class ICombatResultFieldRuntime {
public:
    virtual ~ICombatResultFieldRuntime()=default;
    virtual void WriteWord(std::uint32_t,std::uint32_t)=0;
    virtual void WriteByte(std::uint32_t,std::uint8_t)=0;
};
class ICombatResultDataLifecycleRuntime {
public:
    virtual ~ICombatResultDataLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void CleanupRecord(std::uint32_t address) = 0;
    virtual void FreeNodes(std::uint32_t tree,
                           std::uint32_t root) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    virtual void FirstElement(std::uint32_t tree) = 0;
    [[nodiscard]] virtual std::uint32_t Successor(std::uint32_t node) = 0;
    virtual void RemoveNode(std::uint32_t tree,
                            std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DecrementSharedRef() = 0;
    [[nodiscard]] virtual std::uint32_t TakeSharedNil() = 0;
};

class ICombatResultDataConstructionRuntime : public ICombatResultFieldRuntime {
public:
    virtual ~ICombatResultDataConstructionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void CleanupRecord(std::uint32_t address) = 0;
};

class CombatResultDataLifecycle final {
public:
    static constexpr std::uint32_t kSharedRefCountAddress = 0x8f9594u;
    static constexpr std::uint32_t kSharedNilAddress = 0x8f9598u;

    // IDA 0x5710a0/0x571210.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, std::uint8_t object_seed,
        std::uint8_t record_seed,
        ICombatResultDataConstructionRuntime& runtime);
    static void InitializeRecord(
        std::uint32_t record, std::uint8_t seed,
        ICombatResultFieldRuntime& runtime);
    static void InitializeScalars(std::uint32_t object,ICombatResultFieldRuntime&);

    // IDA 0x543a60.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t object, ICombatResultDataLifecycleRuntime& runtime);
    // IDA 0x543c50.
    [[nodiscard]] static std::uint32_t DestructTree(
        std::uint32_t tree, ICombatResultDataLifecycleRuntime& runtime);
    // IDA 0x5736b0; omits the old destructor's FirstElement refresh.
    [[nodiscard]] static std::uint32_t CleanupTree(
        std::uint32_t tree, ICombatResultDataLifecycleRuntime& runtime);
};

}  // namespace pal4
