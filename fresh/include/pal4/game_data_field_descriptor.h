#pragma once

#include <cstdint>
#include <string>

namespace pal4 {

enum class GameDataFieldKind {
    numeric,
    text,
    action_type,
    hit_extra,
};

struct GameDataFieldDescriptor {
    std::uint32_t offset = 0;
    std::string name;
    GameDataFieldKind kind = GameDataFieldKind::numeric;
    std::uint32_t vtable = 0;
    std::uint32_t object_size = 0;
    std::uint32_t source_address = 0;
    std::uint32_t source_count = 0;

    bool operator==(const GameDataFieldDescriptor&) const = default;
};

}  // namespace pal4
