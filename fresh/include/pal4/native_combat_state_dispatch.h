#pragma once
#include <cstdint>

namespace pal4 {
// One registered PALIV_COMBAT state. Entry must copy its temporary native XML
// argument before returning. The dispatcher borrows this owner until unbound.
class INativeCombatStateDispatch {
public:
    virtual ~INativeCombatStateDispatch() = default;
    [[nodiscard]] virtual std::uint32_t Address() const = 0;
    [[nodiscard]] virtual bool Enter(std::uint32_t previous_id, std::uint32_t document) = 0;
    virtual void Leave(std::uint32_t next_id) = 0;
    virtual void Render(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t Update(float elapsed) = 0;
};
}
