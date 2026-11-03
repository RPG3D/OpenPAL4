#pragma once

#include "pal4/native_actor_animation_catalog.h"
#include "pal4/native_gob_object_image.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

struct RpHAnimHierarchy;

namespace pal4 {

class NativeActorAnimationClipAsset;
class NativeBaseObjectMemory;
class NativeGobClumpAsset;

// Owns the GOB action records and clips while borrowing its scene Clump.
class NativeGobAnimationBinding final : public INativeGobAnimationService {
public:
    NativeGobAnimationBinding(NativeGobObjectImage& object,
                              NativeGobClumpAsset& model,
                              NativeBaseObjectMemory& memory);
    ~NativeGobAnimationBinding() override;
    NativeGobAnimationBinding(const NativeGobAnimationBinding&) = delete;
    NativeGobAnimationBinding& operator=(
        const NativeGobAnimationBinding&) = delete;

    [[nodiscard]] bool Load(std::string_view model_name);
    void Release();
    [[nodiscard]] std::uint32_t ObjectAddress() const noexcept { return object_.Address(); }
    // Retained records can outlive an interrupted load without a playable clip.
    [[nodiscard]] bool Bound() const noexcept { return allocation_ != 0u; }
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }
    [[nodiscard]] const std::string& AnimationLoadError() const noexcept {
        return animation_load_error_;
    }
    [[nodiscard]] bool Select(std::string_view action) override;
    [[nodiscard]] float Length() const override;
    [[nodiscard]] float Time() const override;
    [[nodiscard]] bool SetTime(float time) override;
    [[nodiscard]] bool AdvanceFrame(float elapsed);
    [[nodiscard]] bool ProcessRender(std::uint32_t camera) override;

private:
    class Runtime;
    void PrepareRecords();
    void FinishModelLoad();
    [[nodiscard]] bool PlaybackReady() const noexcept;
    NativeGobObjectImage& object_;
    NativeGobClumpAsset& model_;
    NativeBaseObjectMemory& memory_;
    NativeActorAnimationCatalog catalog_;
    std::vector<std::unique_ptr<NativeActorAnimationClipAsset>> clips_;
    RpHAnimHierarchy* hierarchy_ = nullptr;
    std::uint32_t allocation_ = 0u;
    std::uint32_t count_ = 0u;
    std::string last_error_;
    std::string animation_load_error_;
};

}  // namespace pal4
