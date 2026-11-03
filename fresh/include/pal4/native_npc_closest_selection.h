#pragma once

#include "pal4/npc_manager_closest_finder.h"

#include <array>
#include <cstdint>

namespace pal4 {

class NativeNpcManagerOwner;

// Binds the reconstructed NPC target search to live scene Clumps.
class NativeNpcClosestSelection final : private INpcManagerClosestRuntime {
public:
    explicit NativeNpcClosestSelection(NativeNpcManagerOwner& manager)
        : manager_(manager) {}

    [[nodiscard]] std::uint32_t FindRay(
        const std::array<float, 6>& ray, bool mark_selected);
    [[nodiscard]] std::uint32_t FindByCoordinate(
        const std::array<float, 3>& position, bool mark_selected);

private:
    std::uint32_t ReadDword(std::uint32_t address) override;
    void WriteDword(std::uint32_t address, std::uint32_t value) override;
    bool GetPositionAndData(std::uint32_t npc, std::uint32_t reference,
                            std::uint32_t& data_bits) override;
    std::uint32_t CountClumpMaterials(std::uint32_t clump) override;
    std::uint32_t AdvanceMissingMaterials(
        std::uint32_t node, std::uint32_t header) override;
    double ApproximateMagnitude(float x, float y, float z) override;

    NativeNpcManagerOwner& manager_;
};

}  // namespace pal4
