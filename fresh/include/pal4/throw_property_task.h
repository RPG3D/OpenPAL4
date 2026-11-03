#pragma once

#include "pal4/use_property_task.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

using ThrowPropertyPosition = std::array<float, 3>;

class IThrowPropertyConstructionRuntime {
public:
    virtual ~IThrowPropertyConstructionRuntime() = default;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ConstructBase(std::uint32_t task) = 0;
};

class IThrowPropertyTaskRuntime : public IThrowPropertyConstructionRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    virtual void DestroyBase(std::uint32_t task) = 0;
    virtual void FreeTask(std::uint32_t task) = 0;
    [[nodiscard]] virtual PropertyTaskEffectHandle CreateActorEffect(
        std::uint32_t actor, std::string_view effect_name,
        std::uint32_t hook_address) = 0;
    virtual void ResetEffect(PropertyTaskEffectHandle& effect) = 0;
    virtual void LoadScripts() = 0;
    [[nodiscard]] virtual std::uint32_t CreateComponent() = 0;
    [[nodiscard]] virtual std::string_view PropertyName(
        std::uint32_t property, std::uint32_t offset) = 0;
    virtual void LoadComponentClump(std::uint32_t component,
                                    std::string_view prefix,
                                    std::string_view name) = 0;
    virtual void SetComponentTexture(std::uint32_t component,
                                     std::string_view name) = 0;
    [[nodiscard]] virtual ThrowPropertyPosition ActorPosition(
        std::uint32_t actor) = 0;
    [[nodiscard]] virtual float ActorBoundingHeight(
        std::uint32_t actor) = 0;
    virtual void SetComponentPosition(
        std::uint32_t component,
        const ThrowPropertyPosition& position) = 0;
    [[nodiscard]] virtual float VectorLength(
        const ThrowPropertyPosition& vector) = 0;
    [[nodiscard]] virtual ThrowPropertyPosition Normalize(
        const ThrowPropertyPosition& vector) = 0;
    virtual void CleanupScripts() = 0;
    virtual void DeleteComponent(std::uint32_t component) = 0;
};

class ThrowPropertyTask final {
public:
    static constexpr std::uint32_t kVtable = 0x8457f4u;
    static constexpr std::uint32_t kEffectHook = 0x8fa274u;

    // IDA 0x55ddc0-0x55e150.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t task, IThrowPropertyConstructionRuntime& runtime);
    static void Destroy(std::uint32_t task,
                        IThrowPropertyTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t Delete(
        std::uint32_t task, std::uint8_t flags,
        IThrowPropertyTaskRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetActorsAndProperty(
        std::uint32_t task, std::uint32_t source,
        std::uint32_t target, std::uint32_t property,
        IThrowPropertyTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Process(
        std::uint32_t task, IThrowPropertyTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t UpdatePosition(
        std::uint32_t task, float delta,
        IThrowPropertyTaskRuntime& runtime);
    [[nodiscard]] static std::uint8_t Cleanup(
        std::uint32_t task, IThrowPropertyTaskRuntime& runtime);
    static void CleanupInternal(std::uint32_t task,
                                IThrowPropertyTaskRuntime& runtime);
};

}  // namespace pal4
