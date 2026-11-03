#pragma once

#include "pal4/system_manager_update.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace pal4 {
class NativeScriptEventRouter;

// Game object/roster operations are borrowed. Manager-local buffers, state,
// lifecycle and dispatch remain owned by NativeCombatSystemManager.
class INativeCombatSystemManagerServices : public ISystemManagerUpdateRuntime {
public:
    virtual void PrepareObject(std::uint32_t object) = 0;
    [[nodiscard]] virtual float GameDbFloat(std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::uint32_t ObjectGroup(std::uint32_t object) = 0;
    // Comparison-only read, never an RNG draw. Unknown for older fixtures.
    [[nodiscard]] virtual std::uint32_t ComparisonRandomIndex() const { return 0xffffffffu; }
};

// Owns the original128-byte SystemManager facade and modern object storage.
// Close before its borrowed router/services retire; do not destroy in callbacks.
class NativeCombatSystemManager final {
public:
    NativeCombatSystemManager(NativeScriptEventRouter&,
                              INativeCombatSystemManagerServices&);
    // Publish a successfully constructed manager through CombatSystem+396.
    // Borrowed slot must outlive this owner; Close clears publication.
    NativeCombatSystemManager(NativeScriptEventRouter&,
                              INativeCombatSystemManagerServices&,std::uint32_t& member_slot);
    ~NativeCombatSystemManager();
    void Close();
    void Subscribe();
    void Reset();
    [[nodiscard]] std::uint8_t Update(float seconds);
    [[nodiscard]] bool AllObjectsReady();
    [[nodiscard]] bool Active() const;
    [[nodiscard]] std::uint32_t Address() const;
    [[nodiscard]] std::size_t SubscriptionCount() const;
    // Valid until a roster event/reset/close; caller must not retain this view.
    [[nodiscard]] std::span<const std::uint32_t> Objects() const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
