#pragma once

#include "pal4/native_base_object_memory.h"
#include "pal4/pal_game_iv_state_registry.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>

namespace pal4 {

// Owns PALGameIV's +0x188 state nodes; state objects outlive this list.
class NativePalGameIVStateList final
    : private IPalGameIVStateRegistryRuntime {
public:
    explicit NativePalGameIVStateList(std::uint32_t game);
    NativePalGameIVStateList(const NativePalGameIVStateList&) = delete;
    NativePalGameIVStateList& operator=(
        const NativePalGameIVStateList&) = delete;
    ~NativePalGameIVStateList() override { Clear(); }

    [[nodiscard]] bool Register(std::uint32_t state, std::uint32_t id,
                                std::function<void()> destroy);
    [[nodiscard]] std::uint32_t Find(std::uint32_t id);
    // Resolve an owned state's virtual GetStateId without invoking an EXE vtable
    // on a reconstructed host object. Foreign/stale states are rejected.
    [[nodiscard]] std::uint32_t StateId(std::uint32_t state) override;
    void Clear();
    [[nodiscard]] std::size_t Count() const noexcept { return entries_.size(); }
    [[nodiscard]] std::size_t OutstandingAllocations() const noexcept {
        return memory_.OutstandingAllocations();
    }

private:
    struct Entry {
        std::uint32_t id;
        std::function<void()> destroy;
    };

    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    [[nodiscard]] std::uint32_t Allocate(std::uint32_t bytes) override;
    [[nodiscard]] std::uint32_t Free(std::uint32_t address) override;
    void DeleteState(std::uint32_t state, std::uint32_t flags) override;
    [[nodiscard]] std::uint32_t EmptyCleanupResult() override { return 0u; }

    std::uint32_t game_;
    NativeBaseObjectMemory memory_;
    std::unordered_map<std::uint32_t, Entry> entries_;
};

}  // namespace pal4
