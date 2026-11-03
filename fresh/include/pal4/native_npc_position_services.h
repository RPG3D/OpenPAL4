#pragma once
#include "pal4/script_player_position.h"
namespace pal4 {
// Scene owner boundary keeps the package position runtime independent of the
// RenderWare scene loader. Borrower must retire before the scene owner.
class INativeNpcPositionServices {
public:
    virtual ~INativeNpcPositionServices() = default;
    virtual std::uint32_t NpcManager() = 0;
    virtual std::uint32_t NpcByName(std::uint32_t,std::string_view) = 0;
    virtual bool OwnsObject(std::uint32_t) const noexcept = 0;
    virtual void SetNpcPosition(std::uint32_t,const PlayerPosition&,std::uint32_t mode) = 0;
    virtual std::int32_t SetNpcAction(std::uint32_t,std::string_view) = 0;
};
}
