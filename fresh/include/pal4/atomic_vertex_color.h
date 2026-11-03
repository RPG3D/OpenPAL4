#pragma once

#include <cstdint>

namespace pal4 {

enum class VertexColorPass { set, clear };

class IAtomicVertexColorRuntime {
public:
    virtual ~IAtomicVertexColorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t LockGeometry(
        std::uint32_t geometry, std::uint32_t flags) = 0;
    virtual void UnlockGeometry(std::uint32_t geometry) = 0;
    [[nodiscard]] virtual std::uint32_t ForEachAtomic(
        std::uint32_t clump, VertexColorPass pass,
        const std::uint32_t* argument) = 0;
};

class AtomicVertexColorRuntimeStub final : public IAtomicVertexColorRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t LockGeometry(
        std::uint32_t, std::uint32_t) override { return 0u; }
    void UnlockGeometry(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t ForEachAtomic(
        std::uint32_t, VertexColorPass,
        const std::uint32_t*) override { return 0u; }
};

class AtomicVertexColor final {
public:
    // IDA 0x5fa120.
    [[nodiscard]] static std::uint32_t SetClumpColor(
        std::uint32_t clump, std::uint32_t color,
        IAtomicVertexColorRuntime& runtime);
    // IDA 0x5fa140.
    [[nodiscard]] static std::uint32_t SetAtomicColor(
        std::uint32_t atomic, const std::uint32_t& color,
        IAtomicVertexColorRuntime& runtime);
    // IDA 0x5fa1a0.
    [[nodiscard]] static std::uint32_t ClearClumpColor(
        std::uint32_t clump, std::uint32_t mask,
        IAtomicVertexColorRuntime& runtime);
    // IDA 0x5fa1c0.
    [[nodiscard]] static std::uint32_t ClearAtomicColor(
        std::uint32_t atomic, const std::uint32_t& mask,
        IAtomicVertexColorRuntime& runtime);
};

}  // namespace pal4
