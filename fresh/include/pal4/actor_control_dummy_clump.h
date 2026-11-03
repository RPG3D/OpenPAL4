#pragma once

#include <cstdint>
#include <filesystem>

namespace pal4 {

class IActorControlDummyClump {
public:
    virtual ~IActorControlDummyClump() = default;
    [[nodiscard]] virtual bool Load(
        const std::filesystem::path& game_root) = 0;
    [[nodiscard]] virtual std::uint32_t Address() const noexcept = 0;
    virtual void Unload() noexcept = 0;
};

}  // namespace pal4
