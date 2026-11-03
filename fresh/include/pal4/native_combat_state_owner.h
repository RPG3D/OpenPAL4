#pragma once
#include "pal4/pal_game_iv_components.h"
#include <array>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;

// PALIV_COMBAT's72-byte registered state and embedded native XML. Runtime
// entry/render/update services have their own binding; this is not a battle engine.
// The memory/document services must outlive this owner, including its destructor.
class NativeCombatStateOwner final : private IPalGameIVComponentsRuntime {
public:
    NativeCombatStateOwner(NativeBaseObjectMemory& memory, NativeEventDocumentRuntime& documents)
        : memory_(memory), documents_(documents) {}
    ~NativeCombatStateOwner() override { Destroy(); }
    NativeCombatStateOwner(const NativeCombatStateOwner&) = delete;
    NativeCombatStateOwner& operator=(const NativeCombatStateOwner&) = delete;
    [[nodiscard]] std::uint32_t Construct();
    void Destroy();
    [[nodiscard]] std::uint32_t Address() const noexcept;
    [[nodiscard]] std::uint32_t Document() const;
    // Stages an independent request snapshot; the temporary SDK root can die.
    void AssignRequest(std::uint32_t source);
    std::uint32_t SavedFlag(std::uint32_t index)const;
    void SetSavedFlag(std::uint32_t index,std::uint32_t value);
private:
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t Free(std::uint32_t address) override;
    std::uint32_t StateId(std::uint32_t state) override;
    void DeleteState(std::uint32_t state, std::uint32_t flags) override;
    std::uint32_t EmptyCleanupResult() override { return 0u; }
    std::uint32_t ConstructComponent(PalGameIVComponentKind kind, std::uint32_t address) override;
    NativeBaseObjectMemory& memory_;
    NativeEventDocumentRuntime& documents_;
    std::array<std::uint32_t, 18> image_{};
    bool constructed_{};
};
}
