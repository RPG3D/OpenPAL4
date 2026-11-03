#pragma once

#include <cstdint>

namespace pal4 {

class IRwMaterialFlagsRuntime;
using RwAtomicFlagsCallback = std::uint32_t (*)(
    std::uint32_t atomic, std::uint32_t mode,
    IRwMaterialFlagsRuntime& runtime);
using RwMaterialFlagsCallback = std::uint32_t (*)(
    std::uint32_t material, std::uint32_t mode,
    IRwMaterialFlagsRuntime& runtime);

class IRwMaterialFlagsRuntime {
public:
    virtual ~IRwMaterialFlagsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ForAllAtomics(
        std::uint32_t clump, RwAtomicFlagsCallback callback,
        std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ForAllWorldMaterials(
        std::uint32_t world, RwMaterialFlagsCallback callback,
        std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
};

class RwMaterialFlagsRuntimeStub final : public IRwMaterialFlagsRuntime {
public:
    [[nodiscard]] std::uint32_t ForAllAtomics(
        std::uint32_t clump, RwAtomicFlagsCallback callback,
        std::uint32_t mode) override;
    [[nodiscard]] std::uint32_t ForAllWorldMaterials(
        std::uint32_t world, RwMaterialFlagsCallback callback,
        std::uint32_t mode) override;
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
};

class RwMaterialFlags final {
public:
    // IDA 0x665610.
    [[nodiscard]] static std::uint32_t SetClump(
        std::uint32_t clump, std::uint32_t mode,
        IRwMaterialFlagsRuntime& runtime);
    // IDA 0x665690.
    [[nodiscard]] static std::uint32_t SetAtomic(
        std::uint32_t atomic, std::uint32_t mode,
        IRwMaterialFlagsRuntime& runtime);
    // IDA 0x6656d0.
    [[nodiscard]] static std::uint32_t SetMaterial(
        std::uint32_t material, std::uint32_t mode,
        IRwMaterialFlagsRuntime& runtime);
    // IDA 0x6656f0.
    [[nodiscard]] static std::uint32_t SetWorld(
        std::uint32_t world, std::uint32_t mode,
        IRwMaterialFlagsRuntime& runtime);
};

}  // namespace pal4
