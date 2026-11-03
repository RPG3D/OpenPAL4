#pragma once

#include "pal4/combat_scene_update.h"
#include <functional>

namespace pal4 {
struct NativeCombatSystemImage;
class NativeEventDocumentRuntime;

struct NativeCombatSceneUpdateBindings {
    std::function<void(float)> combat;
    std::function<void(double)> ui;
    std::function<void()> cleanup_script_cache;
    // Synchronous PALGameIV transition. The XML is borrowed only for this call;
    // a retaining destination must assign/clone before returning.
    std::function<void(std::uint32_t, std::uint32_t)> transition;
};

// Native58AFD0: owns its temporary real XML, borrows the stable520-byte combat
// image and frame/state services. Unbind before retiring any borrowed owner.
class NativeCombatSceneUpdate final : private ICombatSceneUpdateServices {
public:
    NativeCombatSceneUpdate(NativeCombatSystemImage&, NativeEventDocumentRuntime&,
                            NativeCombatSceneUpdateBindings);
    NativeCombatSceneUpdate(const NativeCombatSceneUpdate&) = delete;
    NativeCombatSceneUpdate& operator=(const NativeCombatSceneUpdate&) = delete;
    [[nodiscard]] std::uint32_t Update(float delta);
private:
    void AdvanceCombat(float) override;
    void UpdateUI(double) override;
    std::uint32_t State() override;
    bool Alternate() override;
    std::uint32_t Result() override;
    void CleanupScriptCache() override;
    std::uint32_t ConstructXmlDocument() override;
    std::uint32_t DestroyXmlDocument(std::uint32_t) override;
    void WriteResult(std::uint32_t, std::uint32_t, bool) override;
    void Transition(std::uint32_t, std::uint32_t) override;
    void RequireDocument(std::uint32_t) const;
    NativeCombatSystemImage& image_;
    NativeEventDocumentRuntime& documents_;
    NativeCombatSceneUpdateBindings bindings_;
    std::uint32_t document_{};
    bool updating_{};
};
}  // namespace pal4
