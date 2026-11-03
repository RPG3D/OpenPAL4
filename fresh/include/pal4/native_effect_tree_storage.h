#pragma once

#include "pal4/effect_system.h"
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;

// One service per effect system. Borrowed tree images must outlive their explicit
// Cleanup calls; the shared nils remain alive until the last tree releases them.
// Cleanup releases tree nodes, not the Dialog/effect payloads they reference.
class NativeEffectTreeStorage final {
public:
    explicit NativeEffectTreeStorage(NativeBaseObjectMemory& memory);
    ~NativeEffectTreeStorage();
    NativeEffectTreeStorage(const NativeEffectTreeStorage&) = delete;
    NativeEffectTreeStorage& operator=(const NativeEffectTreeStorage&) = delete;
    void InitializeActive(std::uint32_t tree);
    void InitializePending(std::uint32_t tree);
    void InitializeInline(std::uint32_t tree);
    void InitializeContext(std::uint32_t tree);
    void InitializeTimed(std::uint32_t tree);
    void CleanupActive(std::uint32_t tree);
    void CleanupPending(std::uint32_t tree);
    void CleanupInline(std::uint32_t tree);
    void CleanupContext(std::uint32_t tree);
    void CleanupTimed(std::uint32_t tree);
    [[nodiscard]] MonsterManagerTreeInsertResult InsertActive(std::uint32_t tree,
        std::uint32_t id,std::uint32_t dialog,std::int32_t repeats);
    [[nodiscard]] MonsterManagerTreeInsertResult InsertInline(std::uint32_t tree,
        std::uint32_t object,std::uint8_t enabled);
    [[nodiscard]] std::uint32_t FindInline(std::uint32_t tree,std::uint32_t object) const;
    [[nodiscard]] std::uint32_t EraseActive(std::uint32_t tree,std::uint32_t node);
    [[nodiscard]] std::uint32_t EraseInline(std::uint32_t tree,std::uint32_t node);
    [[nodiscard]] std::uint32_t ErasePending(std::uint32_t tree,std::uint32_t node);
    [[nodiscard]] MonsterManagerTreeInsertResult InsertContext(std::uint32_t tree,
        std::uint32_t id, std::uint32_t object);
    [[nodiscard]] std::uint32_t LowerBoundContext(std::uint32_t tree,
        std::uint32_t id) const;
    [[nodiscard]] std::uint32_t EraseContext(std::uint32_t tree, std::uint32_t node);
    void InsertTimedAt(std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t object, float seconds);
    [[nodiscard]] std::uint32_t EraseTimed(std::uint32_t tree, std::uint32_t node);
    // Numeric-node storage only; payloads must already be destroyed.
    void DeleteTimedBranch(std::uint32_t root);
    [[nodiscard]] std::uint32_t ContextNil() const;
    [[nodiscard]] std::uint32_t TimedNil() const;
    [[nodiscard]] std::uint32_t ActiveNil() const;
    [[nodiscard]] std::uint32_t PendingNil() const;
    [[nodiscard]] std::uint32_t InlineNil() const;
private:
    class Memory;
    std::unique_ptr<Memory> memory_;
};
}  // namespace pal4
