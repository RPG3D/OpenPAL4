#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorAnimationBindingRuntime {
public:
    virtual ~IActorAnimationBindingRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual bool EqualIgnoringCase(
        std::string_view left, std::string_view right) = 0;
    [[nodiscard]] virtual std::uint32_t FindHookHand(
        std::uint32_t clump) = 0;
    [[nodiscard]] virtual std::uint32_t IdentityRenderCallback() = 0;
    [[nodiscard]] virtual std::uint32_t DefaultRenderCallback() = 0;
    virtual void InvertMatrix(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void UpdateFrame(std::uint32_t frame) = 0;
    virtual void RotateMatrix(std::uint32_t matrix,
                              std::uint32_t axis,
                              float degrees, std::uint32_t mode) = 0;
    virtual void ProcessUserData(std::uint32_t context,
                                 std::string_view name,
                                 std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t HideHookAtomics(
        std::uint32_t clump) = 0;
};

class ActorAnimationBinding final {
public:
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t actor, std::uint32_t context,
        std::string_view name, std::uint32_t mode,
        IActorAnimationBindingRuntime& runtime);
    [[nodiscard]] static std::uint32_t BindCallback(
        std::uint32_t actor, bool left_weapon,
        std::uint32_t mode, IActorAnimationBindingRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateOrdinary(
        std::uint32_t hook, bool left_weapon,
        IActorAnimationBindingRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateAlternate(
        std::uint32_t hook, bool left_weapon,
        IActorAnimationBindingRuntime& runtime);
};

}  // namespace pal4
