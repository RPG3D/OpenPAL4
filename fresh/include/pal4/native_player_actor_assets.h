#pragma once

#include "pal4/native_animated_actor_asset.h"
#include "pal4/native_actor_shadow_binding.h"
#include "pal4/player_actor_asset_loader.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

namespace pal4 {

// Keeps the four Role actor resources alive after PlayerManagerLoader returns.
// Bound player-slot storage must outlive this owner or be explicitly unloaded.
class NativePlayerActorAssets final : public IPlayerActorAssetLoader {
public:
    explicit NativePlayerActorAssets(std::filesystem::path game_root)
        : game_root_(std::move(game_root)) {}
    ~NativePlayerActorAssets() { Unload(); }

    [[nodiscard]] bool Load(std::uint32_t slot,
                            std::string_view name,
                            std::string_view model) override;
    [[nodiscard]] bool UnloadSlot(std::uint32_t slot) noexcept override;
    void Unload() noexcept;
    [[nodiscard]] const NativeAnimatedActorAsset& At(
        std::size_t index) const noexcept { return actors_[index]; }
    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] const NativeAnimatedActorAsset* Find(
        std::uint32_t slot) const noexcept;
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }

private:
    void Bind(std::size_t index);
    void Clear(std::size_t index) noexcept;

    std::filesystem::path game_root_;
    std::array<NativeAnimatedActorAsset, 4> actors_;
    std::array<std::unique_ptr<NativeActorShadowBinding>,4> shadows_;
    std::array<std::uint32_t, 4> slots_{};
    std::array<std::string, 4> names_;
    std::array<std::string, 4> models_;
    std::array<std::string, 4> directories_;
    std::string last_error_;
};

}  // namespace pal4
