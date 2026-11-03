#pragma once

#include "pal4/npc_manager_access.h"

#include <cstdint>
#include <span>

namespace pal4 {

class INpcManagerClosestRuntime {
public:
    virtual ~INpcManagerClosestRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual bool GetPositionAndData(
        std::uint32_t npc, std::uint32_t reference,
        std::uint32_t& data_bits) = 0;
    [[nodiscard]] virtual std::uint32_t CountClumpMaterials(
        std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint32_t AdvanceMissingMaterials(
        std::uint32_t node, std::uint32_t header) = 0;
    [[nodiscard]] virtual double ApproximateMagnitude(
        float x, float y, float z) = 0;
};

class NpcManagerClosestRuntimeStub : public INpcManagerClosestRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    bool GetPositionAndData(std::uint32_t, std::uint32_t,
                            std::uint32_t&) override;
    std::uint32_t CountClumpMaterials(std::uint32_t) override;
    std::uint32_t AdvanceMissingMaterials(
        std::uint32_t, std::uint32_t) override;
    double ApproximateMagnitude(float, float, float) override;
};

class NpcManagerClosestFinder final {
public:
    // IDA 0x43cdf0: select by strict floating comparison and optionally mark it.
    [[nodiscard]] static std::uint32_t Find(
        std::uint32_t manager, std::uint32_t reference,
        std::uint32_t data_bits, bool mark_selected,
        const NpcManagerTreeGlobals& globals,
        INpcManagerClosestRuntime& runtime);

    // 便携owner提供原名称顺序，复用同一候选筛选，不模拟私有STL树。
    static std::uint32_t FindByCoordinateList(std::span<const std::uint32_t> candidates,
        std::uint32_t coordinate,bool mark_selected,INpcManagerClosestRuntime& runtime);
    // IDA 0x43cf10: spatial distance gated by model, activity and record radius.
    [[nodiscard]] static std::uint32_t FindByCoordinate(
        std::uint32_t manager, std::uint32_t coordinate,
        bool mark_selected, const NpcManagerTreeGlobals& globals,
        INpcManagerClosestRuntime& runtime);
};

}  // namespace pal4
