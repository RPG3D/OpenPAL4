#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IPlayerActorAssetLoader {
public:
    virtual ~IPlayerActorAssetLoader() = default;
    // A successful load publishes its initial resource bounds to the slot,
    // measured before animation changes the loaded DFF's hook frames.
    [[nodiscard]] virtual bool Load(std::uint32_t slot,
                                    std::string_view name,
                                    std::string_view model) = 0;
    [[nodiscard]] virtual bool UnloadSlot(std::uint32_t slot) noexcept = 0;
};

}  // namespace pal4
