#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class GlobalRenderState;

class IGlobalRenderStateRuntime {
public:
    virtual ~IGlobalRenderStateRuntime() = default;
    [[nodiscard]] virtual std::array<std::uint32_t, 3> TransformDirection(
        const GlobalRenderState& state,
        const std::array<std::uint32_t, 3>& source) = 0;
    [[nodiscard]] virtual std::uint32_t CommitDirection(
        GlobalRenderState& state,
        const std::array<float, 3>& direction) = 0;
};

class GlobalRenderState final {
public:
    static constexpr std::uint32_t kAddress = 0x8e3ef8u;
    static constexpr std::uint32_t kVtableAddress = 0x841dc8u;
    static constexpr std::uint32_t kGlobalAngleAddress = 0x8e3f2cu;

    [[nodiscard]] std::uint32_t Initialize(
        IGlobalRenderStateRuntime& runtime);
    [[nodiscard]] std::uint32_t InitializeDefaultBasis(
        IGlobalRenderStateRuntime& runtime);
    [[nodiscard]] std::uint32_t InitializeThunk(
        IGlobalRenderStateRuntime& runtime) {
        return Initialize(runtime);
    }
    [[nodiscard]] std::uint32_t ReadDword(std::uint32_t offset) const;
    void WriteDword(std::uint32_t offset, std::uint32_t value);
    [[nodiscard]] std::array<std::uint32_t, 3>& SetBasisY(
        std::array<std::uint32_t, 3>& output,
        const std::array<std::uint32_t, 3>& values);
    [[nodiscard]] std::array<std::uint32_t, 3>& SetBasisZ(
        std::array<std::uint32_t, 3>& output,
        const std::array<std::uint32_t, 3>& values);
    [[nodiscard]] std::array<float, 3>& TransformVector(
        std::array<float, 3>& output,
        const std::array<float, 3>& input) const;
    [[nodiscard]] std::uint32_t SetNormalizedDirection(
        const std::array<float, 3>& direction,
        const std::array<std::uint32_t, 3>& basis_y,
        IGlobalRenderStateRuntime& runtime);
    [[nodiscard]] static constexpr std::uint32_t VectorByteSize() {
        return 12u;
    }
    static void InitializeGlobalAngle(float& storage);
    static void InitializeGlobalAngleThunk(float& storage) {
        InitializeGlobalAngle(storage);
    }

private:
    std::array<std::uint32_t, 13> words_{};
};

class Vector3Math final {
public:
    [[nodiscard]] static double Dot(const std::array<float, 3>& left,
                                    const std::array<float, 3>& right);
};

}  // namespace pal4
