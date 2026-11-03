#pragma once

#include "pal4/native_actor_animation_catalog.h"
#include "pal4/native_actor_animation_clip_asset.h"
#include "pal4/native_actor_clump_asset.h"

#include <filesystem>
#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

struct RpHAnimHierarchy;
struct RpAtomic;
struct RpMaterial;

namespace pal4 {

// Owns one actor's clumps and every animation referenced by its .tab file.
class NativeAnimatedActorAsset final {
public:
    NativeAnimatedActorAsset() = default;
    ~NativeAnimatedActorAsset();
    NativeAnimatedActorAsset(const NativeAnimatedActorAsset&) = delete;
    NativeAnimatedActorAsset& operator=(const NativeAnimatedActorAsset&) = delete;

    [[nodiscard]] bool Load(const std::filesystem::path& game_root,
                            std::string_view model);
    void Unload() noexcept;
    [[nodiscard]] bool SelectAction(std::string_view action);
    [[nodiscard]] bool Advance(float elapsed);

    [[nodiscard]] RpClump* Main() const noexcept { return clumps_.Main(); }
    [[nodiscard]] RpClump* Shadow() const noexcept { return clumps_.Shadow(); }
    // GameResourceManager words52..71, calculated from the loaded DFF pose
    // before selecting an animation. Animation updates must not replace this
    // initial resource cache with the first displayed pose.
    [[nodiscard]] const std::array<std::uint32_t,20>& InitialBounds() const noexcept {
        return initial_bounds_;
    }
    [[nodiscard]] RpLight* ShadowLight() const noexcept { return clumps_.ShadowLight(); }
    [[nodiscard]] RpHAnimHierarchy* Hierarchy() const noexcept {
        return hierarchy_;
    }
    [[nodiscard]] RpAtomic* BottomEffectHook() const noexcept {
        return bottom_effect_hook_;
    }
    [[nodiscard]] RpMaterial* FaceMaterial() const noexcept {
        return face_material_;
    }
    [[nodiscard]] RpMaterial* LegMaterial() const noexcept {
        return leg_material_;
    }
    [[nodiscard]] std::uint32_t HiddenHookCount() const noexcept {
        return hidden_hooks_;
    }
    [[nodiscard]] std::size_t ActionCount() const noexcept {
        return clips_.size();
    }
    [[nodiscard]] std::string_view ActionAt(std::size_t index) const {
        return catalog_.Entries().at(index).first;
    }
    [[nodiscard]] std::string_view ActionResourceAt(
        std::size_t index) const {
        return catalog_.Entries().at(index).second;
    }
    [[nodiscard]] RtAnimAnimation* AnimationAt(std::size_t index) const {
        return clips_.at(index)->Animation();
    }
    [[nodiscard]] const std::string& CurrentAction() const noexcept;
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }

private:
    NativeActorClumpAsset clumps_;
    std::array<std::uint32_t,20> initial_bounds_{};
    NativeActorAnimationCatalog catalog_;
    std::vector<std::unique_ptr<NativeActorAnimationClipAsset>> clips_;
    RpHAnimHierarchy* hierarchy_ = nullptr;
    RpAtomic* bottom_effect_hook_ = nullptr;
    RpMaterial* face_material_ = nullptr;
    RpMaterial* leg_material_ = nullptr;
    std::uint32_t hidden_hooks_ = 0u;
    std::string current_action_;
    std::string last_error_;
};

}  // namespace pal4
