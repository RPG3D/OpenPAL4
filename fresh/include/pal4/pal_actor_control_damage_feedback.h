#pragma once

#include "pal4/pal_actor_control.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

using DamageWorldPoint = std::array<float, 3>;
using DamageScreenPoint = std::array<float, 2>;

struct DamageFeedbackRecord {
    std::array<std::uint32_t, 23> words{};
};
static_assert(sizeof(DamageFeedbackRecord) == 92u);

class IActorControlDamageFeedbackAnimationRuntime {
public:
    virtual ~IActorControlDamageFeedbackAnimationRuntime() = default;
    virtual void UpdateAnimationProgress(float delta);
    [[nodiscard]] virtual std::uint16_t FpuCompareStatus(
        long double left, long double right);
};

class IActorControlDamageFeedbackDeleteRuntime {
public:
    virtual ~IActorControlDamageFeedbackDeleteRuntime() = default;
    virtual void Cleanup(std::uint32_t object);
    virtual void Free(std::uint32_t object);
};

class IActorControlDamageFeedbackRuntime {
public:
    virtual ~IActorControlDamageFeedbackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager();
    [[nodiscard]] virtual std::uint32_t FindPlayer(
        std::uint32_t manager, std::uint32_t index);
    [[nodiscard]] virtual std::int32_t ReadAttribute(
        std::uint32_t attributes, std::uint32_t slot,
        std::uint32_t property);
    virtual void ChangeAttribute(std::uint32_t attributes,
                                 std::uint32_t property, std::int32_t amount);
    [[nodiscard]] virtual std::int32_t FloatToLong(long double value);
    [[nodiscard]] virtual DamageWorldPoint PlayerPosition(std::uint32_t address);
    [[nodiscard]] virtual float PlayerHeight(std::uint32_t address);
    [[nodiscard]] virtual std::uint32_t CameraSubsystem();
    [[nodiscard]] virtual std::uint32_t ActiveCamera(
        std::uint32_t subsystem);
    [[nodiscard]] virtual DamageScreenPoint Project(
        std::uint32_t camera, const DamageWorldPoint& position);
    [[nodiscard]] virtual std::uint32_t UiFrameManager();
    [[nodiscard]] virtual DamageScreenPoint ConvertCoordinates(
        std::uint32_t manager, const DamageScreenPoint& point);
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes);
    virtual void InitializeFeedback(std::uint32_t object);
    virtual void WriteWord(std::uint32_t address, std::uint32_t value);
    [[nodiscard]] virtual std::string FormatDamage(std::int32_t amount);
    virtual void InitializeFont(std::uint32_t object, std::string_view text,
                                std::string_view font, float scale);
    virtual void SetPosition(std::uint32_t object,
                             const DamageScreenPoint& position);
    virtual void SetFontProperties(std::uint32_t object, float x, float y);
    virtual void AddFeedback(std::uint32_t list, std::uint8_t kind,
                             std::uint32_t object);
    [[nodiscard]] virtual std::uint32_t CleanupText(std::string& text);
};

class PalActorControlDamageFeedback final {
public:
    // IDA 0x426910. The full EAX value comes from the final string cleanup.
    [[nodiscard]] static std::uint32_t Apply(
        const PalActorControlState& state, float percentage,
        IActorControlDamageFeedbackRuntime& runtime);
    // IDA 0x4274b0, vtable slot at 0x84269c.
    [[nodiscard]] static std::uint32_t TickAnimation(
        DamageFeedbackRecord& record, float delta,
        IActorControlDamageFeedbackAnimationRuntime& runtime);
    // IDA 0x427540, shared by several feedback-object vtables.
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t object, std::uint32_t flags,
        IActorControlDamageFeedbackDeleteRuntime& runtime);
};

}  // namespace pal4
