#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

//575F40 game phases with typed owners. Console/effect/event incidental EAX is
//not a completion result; native callers advance through this void boundary.
class ICombatSystemUpdateServices {
public:
    virtual ~ICombatSystemUpdateServices() = default;
    virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,std::uint32_t value) = 0;
    virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual std::uint32_t RandomNumber() = 0;
    // The original stage iterator ignores bucket; it does not randomly choose
    // an actor. Virtual16 is the stage vector count, virtual12 its current actor.
    virtual std::uint32_t CreateQuickEndIterator(std::uint32_t bucket) = 0;
    virtual std::int32_t GetCandidateCount(std::uint32_t iterator) = 0;
    virtual std::uint32_t GetCandidateActor(std::uint32_t iterator) = 0;
    virtual void RunCandidateAction(std::uint32_t actor) = 0;
    virtual void ReleaseCandidate(std::uint32_t iterator,std::uint32_t flag) = 0;
    virtual void DispatchScriptEvent(std::string_view,std::uint32_t payload) = 0;
    virtual void UpdateTimedEffects(float delta) = 0;
    virtual void UpdateActorData(float delta) = 0;
    virtual void DispatchSystemUpdate(float delta) = 0;
    virtual void UpdateConsoleAnimation(float delta) = 0;
    virtual void UpdateWorldAndInput(float delta) = 0;
    virtual void UpdateGameObject(float delta) = 0;
    virtual void CloseCombatUi(std::uint32_t object) = 0;
    virtual void EndCombat(std::uint32_t object) = 0;
};

class ICombatSystemUpdateRuntime {
public:
    virtual ~ICombatSystemUpdateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RandomNumber() = 0;
    [[nodiscard]] virtual std::uint32_t GetActorManager() = 0;
    [[nodiscard]] virtual std::uint32_t SelectQuickEndCandidate(
        std::uint32_t collection, std::uint32_t bucket) = 0;
    [[nodiscard]] virtual std::int32_t GetCandidateCount(
        std::uint32_t candidate) = 0;
    [[nodiscard]] virtual std::uint32_t GetCandidateActor(
        std::uint32_t candidate) = 0;
    virtual void RunCandidateAction(std::uint32_t actor) = 0;
    virtual void ReleaseCandidate(std::uint32_t candidate,
                                  std::uint32_t deleting_flag) = 0;
    [[nodiscard]] virtual std::uint32_t DispatchScriptEvent(
        std::string_view name, std::uint32_t payload) = 0;
    virtual void UpdateTimedEffects(float delta) = 0;
    virtual void UpdateActorData(std::uint32_t manager,
                                 std::uint32_t delta_bits) = 0;
    virtual void DispatchSystemUpdate(std::uint32_t system_manager,
                                      std::uint32_t delta_bits) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateConsoleAnimation(
        float delta) = 0;
    virtual void UpdateWorldAndInput(float delta) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateGameObject(
        float delta) = 0;
    virtual void CloseCombatUi(std::uint32_t object) = 0;
    virtual void EndCombat(std::uint32_t object) = 0;
};

class CombatSystemUpdate final {
public:
    static bool RequiresQuickResolution(std::uint8_t inactive,float seconds) noexcept;
    static void Advance(std::uint32_t object,float delta,
        std::uint32_t quick_end_payload_seed,ICombatSystemUpdateServices&);
    // IDA 0x575f40.
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t object, float delta,
        std::uint32_t quick_end_payload_seed,
        ICombatSystemUpdateRuntime& runtime);
};

}  // namespace pal4
