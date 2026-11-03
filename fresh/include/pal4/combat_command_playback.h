#pragma once

#include <cstdint>

namespace pal4 {

class ICombatCommandPlaybackRuntime {
public:
    virtual ~ICombatCommandPlaybackRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptEventRouter() = 0;
    virtual void DispatchEvent(std::uint32_t router, std::uint32_t descriptor,
                               std::uint32_t event, std::uint32_t mode,
                               bool pointer_payload) = 0;
    virtual void DispatchLocalByte(std::uint32_t router,
                                   std::uint32_t descriptor,
                                   std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t GameDatabase() = 0;
    [[nodiscard]] virtual std::uint32_t FindMagic(
        std::uint32_t table, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t FindStunt(
        std::uint32_t table, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t FindProperty(
        std::uint32_t database, std::uint32_t id) = 0;
    [[nodiscard]] virtual std::uint32_t MagicTargetMode(std::uint32_t magic) = 0;
    [[nodiscard]] virtual std::uint32_t StuntTargetMode(std::uint32_t stunt) = 0;
    [[nodiscard]] virtual std::uint32_t PropertyTargetMode(
        std::uint32_t property) = 0;
    [[nodiscard]] virtual std::uint32_t ActorFromFacet(std::uint32_t event) = 0;
    [[nodiscard]] virtual std::uint32_t ActorAttackMode(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t SelectCandidates(
        std::uint32_t actor, std::uint32_t event) = 0;
    [[nodiscard]] virtual std::int32_t CandidateCount(
        std::uint32_t candidates) = 0;
    [[nodiscard]] virtual std::uint32_t CurrentCandidate(
        std::uint32_t candidates) = 0;
    virtual void DestroyCandidates(std::uint32_t candidates) = 0;
    virtual void ReportInvalidTargetMode(std::uint32_t mode) = 0;
};

class CombatCommandPlayback final {
public:
    // IDA 0x57c940.
    static void PlayMagic(std::uint32_t event,
                          ICombatCommandPlaybackRuntime& runtime);
    // IDA 0x57c9a0.
    static void PlayStunt(std::uint32_t event,
                          ICombatCommandPlaybackRuntime& runtime);
    // IDA 0x57ca00.
    static void UseProperty(std::uint32_t event,
                            ICombatCommandPlaybackRuntime& runtime);
    // IDA 0x57ca60.
    static void PostProcess(std::uint32_t event, std::uint32_t target_mode,
                            ICombatCommandPlaybackRuntime& runtime);
    // IDA 0x57cbd0.
    static void SelectAttack(std::uint32_t event,
                           ICombatCommandPlaybackRuntime& runtime);
};

}  // namespace pal4
