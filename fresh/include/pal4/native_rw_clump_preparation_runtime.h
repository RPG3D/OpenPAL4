#pragma once
#include "pal4/rw_clump_optimizer.h"
#include "pal4/rw_geometry_flags.h"

namespace pal4 {
class NativeRwClumpPreparationRuntime final : public IRwClumpOptimizerRuntime,
                                               public IRwGeometryFlagsRuntime {
public:
    std::uint32_t ReadOptimizerWord(std::uint32_t address) override;
    void WriteOptimizerWord(std::uint32_t address,std::uint32_t value) override;
    std::uint32_t ReadGeometryWord(std::uint32_t address) override;
    void WriteGeometryWord(std::uint32_t address,std::uint32_t value) override;
    void SetTriStripMethod(std::uint32_t context) override;
    std::uint8_t TriStripMeshFlag() override;
    std::uint32_t ForAllOptimizerMeshes(std::uint32_t geometry,
        std::uint32_t (*callback)(std::uint32_t,std::uint32_t,std::uint32_t,IRwClumpOptimizerRuntime&),
        std::uint32_t context) override;
    void OptimizeAtomicRaw(std::uint32_t atomic) override;
    void OptimizeClumpRaw(std::uint32_t clump,std::uint32_t mode) override;
    std::uint32_t ForAllOptimizerAtomics(std::uint32_t clump,
        std::uint32_t (*callback)(std::uint32_t,std::uint32_t,IRwClumpOptimizerRuntime&),
        std::uint32_t context) override;
    std::uint32_t ForAllGeometryFlagAtomics(std::uint32_t clump,
        std::uint32_t (*callback)(std::uint32_t,std::uint32_t,IRwGeometryFlagsRuntime&),
        std::uint32_t context) override;
};
}  // namespace pal4
