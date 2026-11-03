#pragma once

#include "pal4/rw_material_flags.h"

namespace pal4 {
// Native traversal/memory boundary for the shared PAL4 texture-filter policy.
class NativeRwMaterialFlagsRuntime final : public IRwMaterialFlagsRuntime {
public:
    std::uint32_t ForAllAtomics(std::uint32_t clump,
        RwAtomicFlagsCallback callback, std::uint32_t mode) override;
    std::uint32_t ForAllWorldMaterials(std::uint32_t world,
        RwMaterialFlagsCallback callback, std::uint32_t mode) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
};
}  // namespace pal4
