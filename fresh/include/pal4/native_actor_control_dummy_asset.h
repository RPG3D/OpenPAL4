#pragma once

#include "pal4/actor_control_dummy_clump.h"

#include <cstdint>
#include <filesystem>
#include <string>

struct RpClump;

namespace pal4 {

// Owns ActorControl's JDumy clump from the original palobject.cpk.
class NativeActorControlDummyAsset final : public IActorControlDummyClump {
public:
    NativeActorControlDummyAsset() = default;
    NativeActorControlDummyAsset(const NativeActorControlDummyAsset&) = delete;
    NativeActorControlDummyAsset& operator=(
        const NativeActorControlDummyAsset&) = delete;
    ~NativeActorControlDummyAsset();

    [[nodiscard]] bool Load(const std::filesystem::path& game_root) override;
    void Unload() noexcept override;
    [[nodiscard]] std::uint32_t Address() const noexcept override {
        return static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(clump_));
    }
    [[nodiscard]] RpClump* Main() const noexcept { return clump_; }
    [[nodiscard]] std::uint32_t AtomicCount() const noexcept {
        return atomics_;
    }
    [[nodiscard]] std::uint32_t MissingTextures() const noexcept {
        return missing_textures_;
    }
    [[nodiscard]] const std::string& LastError() const noexcept {
        return last_error_;
    }

private:
    RpClump* clump_{};
    std::uint32_t atomics_{};
    std::uint32_t textures_{};
    std::uint32_t missing_textures_{};
    std::string last_error_;
};

}  // namespace pal4
