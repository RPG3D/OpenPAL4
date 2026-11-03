#pragma once

#include "pal4/game_world_material_operations.h"

namespace pal4 {

// Native boundary for IDA 6625D0/662610. The original packed word is
// little-endian RGBA, despite the historical "flags" interface names.
class NativeModelMaterialRuntime final : public IGameWorldMaterialFlagRuntime {
public:
    std::uint32_t GeometryOfAtomic(std::uint32_t atomic) override;
    std::uint32_t GeometryFlags(std::uint32_t geometry) override;
    void SetGeometryFlags(std::uint32_t geometry, std::uint32_t flags) override;
    std::int32_t MaterialCount(std::uint32_t geometry) override;
    std::uint32_t MaterialAt(std::uint32_t geometry, std::int32_t index) override;
    void SetMaterialFlags(std::uint32_t material, std::uint32_t flags) override;
    std::uint32_t ForAllClumpAtomics(std::uint32_t clump,
                                   std::uint32_t flags) override;
};

} // namespace pal4
