#pragma once

#include <cstdint>

namespace pal4 {

class ICombatEventCallbackRuntime {
public:
    virtual ~ICombatEventCallbackRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t BeginActors(
        std::uint32_t collection) = 0;
    [[nodiscard]] virtual bool AtEnd(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t Actor(
        std::uint32_t iterator) = 0;
    virtual void ProcessActor(std::uint32_t actor) = 0;
    virtual void Advance(std::uint32_t iterator) = 0;
    virtual void DestroyIterator(std::uint32_t iterator) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystemInstance() = 0;
    [[nodiscard]] virtual std::uint32_t GameString(
        std::uint32_t identifier) = 0;
    [[nodiscard]] virtual std::uint32_t FormatHint(
        std::uint32_t format, std::uint32_t value) = 0;
    virtual void CreateHintWindow() = 0;
    virtual void ShowHint(std::uint32_t text, float x, float y,
                          float duration, std::uint32_t first,
                          std::uint32_t second) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseHint(
        std::uint32_t text) = 0;
};

class CombatEventCallbacks final {
public:
    // IDA 0x57a770.
    [[nodiscard]] static std::uint32_t HandleEvent(
        std::uint32_t owner, std::uint32_t event,
        ICombatEventCallbackRuntime& runtime);
    // IDA 0x57a910.
    static void ResetState(std::uint32_t owner,
                           ICombatEventCallbackRuntime& runtime);
};

}  // namespace pal4
