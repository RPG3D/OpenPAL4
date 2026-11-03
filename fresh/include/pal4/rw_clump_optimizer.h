#pragma once

#include <cstdint>

namespace pal4 {

class IRwClumpOptimizerRuntime {
public:
    virtual ~IRwClumpOptimizerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadOptimizerWord(
        std::uint32_t address) = 0;
    virtual void WriteOptimizerWord(std::uint32_t address,
                                    std::uint32_t value) = 0;
    virtual void SetTriStripMethod(std::uint32_t context) = 0;
    [[nodiscard]] virtual std::uint8_t TriStripMeshFlag() = 0;
    [[nodiscard]] virtual std::uint32_t ForAllOptimizerMeshes(
        std::uint32_t geometry,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                   std::uint32_t, IRwClumpOptimizerRuntime&),
        std::uint32_t context) = 0;
    virtual void OptimizeAtomicRaw(std::uint32_t atomic) = 0;
    virtual void OptimizeClumpRaw(std::uint32_t clump,
                                  std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ForAllOptimizerAtomics(
        std::uint32_t clump,
        std::uint32_t (*callback)(std::uint32_t, std::uint32_t,
                                   IRwClumpOptimizerRuntime&),
        std::uint32_t context) = 0;
};

class RwClumpOptimizer final {
public:
    // IDA 0x667e80, 0x667ec0 and 0x667f00.
    [[nodiscard]] static std::uint32_t OptimizeAtomic(
        std::uint32_t atomic, std::uint32_t context,
        IRwClumpOptimizerRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetMeshTriStripMethod(
        std::uint32_t mesh, std::uint32_t mesh_header,
        std::uint32_t context, IRwClumpOptimizerRuntime& runtime);
    [[nodiscard]] static std::uint32_t OptimizeClump(
        std::uint32_t clump, IRwClumpOptimizerRuntime& runtime);
};

}  // namespace pal4
