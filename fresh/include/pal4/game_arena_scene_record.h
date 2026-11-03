#pragma once

#include <cstdint>
#include <string>

namespace pal4 {

struct GameArenaSceneRecord {
    std::uint32_t address = 0u;
    std::string world;
    std::string scene;
    std::uint32_t actor_type = 0u;
};

}  // namespace pal4
