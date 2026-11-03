#pragma once

#include <cstdint>

namespace pal4 {

class IScriptTaskBaseRuntime;

class ICombatEndActionRuntime {
public:
    virtual ~ICombatEndActionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void NotifySource(std::uint32_t source) = 0;
    [[nodiscard]] virtual bool SourceStateThree(std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t ProcessDead(
        std::uint32_t source, std::uint32_t target) = 0;
    [[nodiscard]] virtual std::uint8_t RegisterActorEvent(
        std::uint32_t action) = 0;
};

class CombatEndActionTask final {
public:
    static constexpr std::uint32_t kSize = 48u;
    static constexpr std::uint32_t kVtable = 0x845c9cu;

    // IDA 0x5661b0: notify actor and register a death action for state 3.
    [[nodiscard]] static std::uint8_t Start(
        std::uint32_t task, ICombatEndActionRuntime& runtime);

    // IDA 0x5687b0/0x5687d0: deleting and base destructors.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t task, std::uint8_t flags,
        IScriptTaskBaseRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyBase(
        std::uint32_t task, IScriptTaskBaseRuntime& runtime);
};

}  // namespace pal4
