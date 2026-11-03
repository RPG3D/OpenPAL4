#pragma once

#include "pal4/arena_skill_manager.h"
#include "pal4/monster_distance_trigger.h"

namespace pal4 {

class GameMathTables;
class NativeBaseObjectMemory;
class NativeMonsterManagerOwner;

// Borrows a live arena skill manager image and owned monster services.
// Does not create skills, own their arrays, or simulate missing effects.
class NativeMonsterArenaSkillFrame final : public IArenaSkillObjectReader,
                                          public IMonsterDistanceTriggerRuntime {
public:
    NativeMonsterArenaSkillFrame(NativeBaseObjectMemory& memory,
                                 NativeMonsterManagerOwner& monsters,
                                 const GameMathTables& math);
    [[nodiscard]] std::uint32_t Visit(std::uint32_t arena_skills,
                                       std::uint32_t monster);

    std::uint32_t ReadWord(std::uint32_t address) override;
    double Distance(const std::array<float, 3>& delta) override;
    void NotifyAt564(std::uint32_t address) override;
    std::string AnimationName(std::uint32_t address) override;
    bool NameEqualsIgnoreCase(std::string_view left,
                              std::string_view right) override;
    bool SelectAnimation(std::uint32_t address, std::string_view name) override;

private:
    NativeBaseObjectMemory& memory_;
    NativeMonsterManagerOwner& monsters_;
    const GameMathTables& math_;
};

}  // namespace pal4
