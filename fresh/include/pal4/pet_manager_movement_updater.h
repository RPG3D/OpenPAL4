#pragma once

#include <cstdint>
#include <string_view>

#include "pal4/pet_manager_state_updater.h"

namespace pal4 {

struct PetMovementScratch {
    PetVector3 direction{};  // IDA 0x8e4334
    PetVector3 next_position{};  // IDA 0x8e4328
};

class IPetManagerMovementRuntime {
public:
    virtual ~IPetManagerMovementRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ActorControl() = 0;
    [[nodiscard]] virtual std::uint32_t ActorMode(
        std::uint32_t control) = 0;
    [[nodiscard]] virtual double Magnitude(
        const PetVector3& vector) = 0;
    virtual void ClampDirection(PetVector3& direction) = 0;
    [[nodiscard]] virtual double NormalizeFloatRange(float dot) = 0;
    virtual void SetFacingTarget(std::uint32_t pet,
                                 const PetVector3& target,
                                 std::uint32_t mode) = 0;
    virtual void SetPosition(std::uint32_t pet,
                             const PetVector3& position) = 0;
    [[nodiscard]] virtual std::string_view AnimationName(
        std::uint32_t animation) = 0;
    [[nodiscard]] virtual int CompareNames(std::string_view current,
                                            std::string_view expected) = 0;
    [[nodiscard]] virtual std::uint32_t SelectAnimation(
        std::uint32_t animation, std::string_view name) = 0;
};

class PetManagerMovementRuntimeStub : public IPetManagerMovementRuntime {
public:
    std::uint32_t ReadDword(std::uint32_t) override;
    void WriteDword(std::uint32_t, std::uint32_t) override;
    std::uint32_t ActorControl() override;
    std::uint32_t ActorMode(std::uint32_t) override;
    double Magnitude(const PetVector3&) override;
    void ClampDirection(PetVector3&) override;
    double NormalizeFloatRange(float) override;
    void SetFacingTarget(std::uint32_t, const PetVector3&,
                         std::uint32_t) override;
    void SetPosition(std::uint32_t, const PetVector3&) override;
    std::string_view AnimationName(std::uint32_t) override;
    int CompareNames(std::string_view, std::string_view) override;
    std::uint32_t SelectAnimation(std::uint32_t,
                                  std::string_view) override;
};

class PetManagerMovementUpdater final {
public:
    static constexpr std::uint32_t kDirectionAddress = 0x8e4334u;
    static constexpr std::uint32_t kNextPositionAddress = 0x8e4328u;
    static constexpr float kRadiansToDegrees = 57.295780181884766f;
    static constexpr double kFullTurn = 6.283185307179586;

    // IDA 0x43e390: follow the stored source and select C01/C02 motion.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t pet, float elapsed, PetMovementScratch& scratch,
        IPetManagerMovementRuntime& runtime);

private:
    [[nodiscard]] static std::uint32_t EnsureAnimation(
        std::uint32_t pet, std::string_view name,
        IPetManagerMovementRuntime& runtime);
};

}  // namespace pal4
