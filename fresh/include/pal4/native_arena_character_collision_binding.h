#pragma once

#include <cstdint>

namespace pal4 {

struct BaseObjectState;
class NativeArenaCollisionTree;

class NativeArenaCharacterCollisionBinding final {
public:
    [[nodiscard]] static std::uint32_t Initialize(
        BaseObjectState& object, NativeArenaCollisionTree& collision);
    static void Clear(BaseObjectState& object);
};

}  // namespace pal4
