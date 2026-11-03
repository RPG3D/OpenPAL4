#pragma once

#include <cstdint>

namespace pal4 {

class ISkillSystemDispatchRuntime {
public:
    virtual ~ISkillSystemDispatchRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t LookupSkill(std::uint32_t id) = 0;
    virtual void ActorEvent(std::uint32_t slot, std::uint32_t actor,
                            std::uint32_t skill, std::uint32_t skill_id,
                            std::uint32_t target) = 0;
    virtual void TargetEvent(std::uint32_t slot, std::uint32_t target,
                             std::uint32_t actor, std::uint32_t skill,
                             std::uint32_t skill_id) = 0;
    virtual void InvokeCallback(std::uint32_t callback, std::uint32_t skill,
                                std::uint32_t context, std::uint32_t actor,
                                std::uint32_t target) = 0;
};

class SkillSystemDispatch final {
public:
    static constexpr std::uint32_t kVectorBeginOffset = 2348u;
    static constexpr std::uint32_t kVectorEndOffset = 2352u;
    static constexpr std::uint32_t kSkillIdOffset = 16u;
    static constexpr std::uint32_t kNextSkillIdOffset = 56u;

    // IDA 0x58a870. The vector must already be sorted by signed skill ID.
    [[nodiscard]] static std::uint32_t FindCallback(
        std::uint32_t system, std::int32_t skill_id,
        ISkillSystemDispatchRuntime& runtime);

    // IDA 0x58a7c0. A cyclic linked-skill chain repeats as in the native code.
    [[nodiscard]] static std::uint32_t ProcessChain(
        std::uint32_t system, std::uint32_t skill, std::uint32_t context,
        std::uint32_t actor, std::uint32_t target,
        ISkillSystemDispatchRuntime& runtime);
};

}  // namespace pal4
