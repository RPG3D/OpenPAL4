#pragma once
#include <cstdint>
#include <memory>

namespace pal4::portable {

class ArenaSkillState {
public:
    ArenaSkillState();
    ~ArenaSkillState();
    ArenaSkillState(const ArenaSkillState&)=delete;
    ArenaSkillState& operator=(const ArenaSkillState&)=delete;

    std::uint32_t HideSkillObject();
    void Close();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace pal4::portable
