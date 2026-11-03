#pragma once

#include <cstdint>

namespace pal4 {

struct PetVector3 {
    float x{};
    float y{};
    float z{};
};

struct PetVectorView {
    std::uint32_t address{};
    PetVector3 value{};
};

class IPetManagerUpdateRuntime {
public:
    virtual ~IPetManagerUpdateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void UpdateAndRenderClump(float elapsed) = 0;
    virtual void UpdateInterpolator(std::uint32_t animation, float elapsed) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateMovement(
        std::uint32_t pet, float elapsed) = 0;
    virtual void UpdateEmbeddedMotion(std::uint32_t embedded,
                                      std::uint32_t argument,
                                      float elapsed, float weight) = 0;
    [[nodiscard]] virtual float ReadD3DVectorComponent(
        std::uint32_t embedded, std::uint32_t index) = 0;
    [[nodiscard]] virtual PetVectorView ReadCsbVector(
        std::uint32_t embedded) = 0;
    virtual void ClampHorizontalDirection(PetVector3& direction) = 0;
    [[nodiscard]] virtual double NormalizeFloatRange(float dot) = 0;
    virtual void SetFacingTarget(std::uint32_t pet,
                                 const PetVector3& target,
                                 std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t SetPosition(
        std::uint32_t pet, const PetVector3& position) = 0;
    [[nodiscard]] virtual std::uint32_t ZeroDirectionReturn(
        std::uint32_t last_vector_address) = 0;
};

class PetManagerUpdateRuntimeStub : public IPetManagerUpdateRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    void UpdateAndRenderClump(float) override;
    void UpdateInterpolator(std::uint32_t, float) override;
    std::uint32_t UpdateMovement(std::uint32_t, float) override;
    void UpdateEmbeddedMotion(std::uint32_t, std::uint32_t,
                              float, float) override;
    float ReadD3DVectorComponent(std::uint32_t,
                                  std::uint32_t) override;
    PetVectorView ReadCsbVector(std::uint32_t) override;
    void ClampHorizontalDirection(PetVector3&) override;
    double NormalizeFloatRange(float) override;
    void SetFacingTarget(std::uint32_t, const PetVector3&,
                         std::uint32_t) override;
    std::uint32_t SetPosition(std::uint32_t,
                              const PetVector3&) override;
    std::uint32_t ZeroDirectionReturn(std::uint32_t) override;
};

class PetManagerStateUpdater final {
public:
    static constexpr std::uint32_t kEmbeddedOffset = 0x564u;
    static constexpr float kRadiansToDegrees = 57.295780181884766f;
    static constexpr double kFullTurn = 6.283185307179586;

    // IDA 0x43e030: time update, animation, motion and facing/position dispatch.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t pet, float elapsed, std::uint32_t argument,
        IPetManagerUpdateRuntime& runtime);
};

}  // namespace pal4
