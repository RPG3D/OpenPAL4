#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IRwLightRuntime {
public:
    virtual ~IRwLightRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CreateLight(std::uint32_t type) = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    virtual void RotateFrame(std::uint32_t frame,
                             const std::array<float, 3>& axis,
                             float angle, std::uint32_t mode) = 0;
    virtual void TranslateFrame(std::uint32_t frame,
                                const std::array<float, 3>& position,
                                std::uint32_t mode) = 0;
    virtual void SetFrame(std::uint32_t light, std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t SetColor(
        std::uint32_t light, const std::array<float, 4>& color) = 0;
    [[nodiscard]] virtual std::uint32_t SetRadius(
        std::uint32_t light, std::uint32_t radius_bits) = 0;
    [[nodiscard]] virtual std::uint32_t SetConeAngle(
        std::uint32_t light, float angle) = 0;
    [[nodiscard]] virtual std::uint32_t WorldOfLight(
        std::uint32_t light) = 0;
    virtual void RemoveLight(std::uint32_t world,
                             std::uint32_t light) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void DestroyFrame(std::uint32_t frame) = 0;
    virtual void DestroyLight(std::uint32_t light) = 0;
};

class RwLightRuntimeStub final : public IRwLightRuntime {
public:
    std::uint32_t CreateLight(std::uint32_t) override { return 0; }
    std::uint32_t CreateFrame() override { return 0; }
    void RotateFrame(std::uint32_t, const std::array<float, 3>&,
                     float, std::uint32_t) override {}
    void TranslateFrame(std::uint32_t, const std::array<float, 3>&,
                        std::uint32_t) override {}
    void SetFrame(std::uint32_t, std::uint32_t) override {}
    std::uint32_t SetColor(
        std::uint32_t, const std::array<float, 4>&) override { return 0; }
    std::uint32_t SetRadius(
        std::uint32_t, std::uint32_t) override { return 0; }
    std::uint32_t SetConeAngle(
        std::uint32_t, float) override { return 0; }
    std::uint32_t WorldOfLight(std::uint32_t) override { return 0; }
    void RemoveLight(std::uint32_t, std::uint32_t) override {}
    std::uint32_t ReadWord(std::uint32_t) override { return 0; }
    void DestroyFrame(std::uint32_t) override {}
    void DestroyLight(std::uint32_t) override {}
};

class RwLightOperations final {
public:
    [[nodiscard]] static std::uint32_t CreateTypeTwo(IRwLightRuntime& runtime);
    [[nodiscard]] static std::uint32_t CreateWithFrame(
        const std::array<float, 3>& position,
        const std::array<float, 3>& angles, IRwLightRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetColor(
        std::uint32_t light, const std::array<float, 4>& color,
        IRwLightRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetRadius(
        std::uint32_t light, std::uint32_t radius_bits,
        IRwLightRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetConeAngle(
        std::uint32_t light, float angle, IRwLightRuntime& runtime);
    static void Destroy(std::uint32_t light, IRwLightRuntime& runtime);
};

}  // namespace pal4
