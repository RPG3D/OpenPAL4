#pragma once

#include <cstdint>

namespace pal4 {

class ICombatTransitionRuntime {
public:
    virtual ~ICombatTransitionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ScreenEffect() = 0;
    virtual void InitializeEffectParameters(std::uint32_t effect) = 0;
    // Original wrapper670060: UI package index5; suffix6 was an ordinal.
    virtual void ProcessScript6() = 0;
    [[nodiscard]] virtual std::uint32_t TextureDictionary() = 0;
    [[nodiscard]] virtual std::uint32_t LoadTexture(
        std::uint32_t dictionary, std::uint32_t name) = 0;
    virtual void CleanupScripts() = 0;
    [[nodiscard]] virtual std::uint32_t GameConfig() = 0;
    [[nodiscard]] virtual std::int32_t Ftol(double value) = 0;
    [[nodiscard]] virtual std::uint32_t DrawRectangle(
        std::uint32_t frame, std::uint32_t x, std::uint32_t y,
        std::uint32_t width, std::uint32_t height,
        std::uint32_t texture, std::uint32_t red,
        std::uint32_t green, std::uint32_t blue,
        std::int32_t alpha) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    virtual void InitializeStateAndUI(std::uint32_t game,
                                      std::uint32_t state,
                                      std::uint32_t document) = 0;
};

// Original PALIV_GAME_OVER/state7, registered as20 bytes by5B6CB0.
// Historical class name retained for existing shared callers.
class CombatTransitionOverlay final {
public:
    static constexpr std::uint32_t kVtable = 0x8485ecu;
    static constexpr std::uint32_t kBaseVtable = 0x8485c8u;
    static constexpr std::uint32_t kTextureName = 0x8b93c0u;

    // IDA 0x58b380 / 0x58b3a0 / 0x58b3c0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t object, ICombatTransitionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, std::uint32_t flags,
        ICombatTransitionRuntime& runtime);
    static void ResetState(std::uint32_t object,
                           ICombatTransitionRuntime& runtime);

    // IDA 0x58b3d0 / 0x58b420 / 0x58b4a0 / 0x58b4e0.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, ICombatTransitionRuntime& runtime);
    [[nodiscard]] static std::uint32_t Draw(
        std::uint32_t object, std::uint32_t frame,
        ICombatTransitionRuntime& runtime);
    static void AdvanceTimer(std::uint32_t object, float delta,
                             ICombatTransitionRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetProperties(
        std::uint32_t object, ICombatTransitionRuntime& runtime);
};

}  // namespace pal4
