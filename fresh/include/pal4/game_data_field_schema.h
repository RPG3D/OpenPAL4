#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class GameDataFieldKind : std::uint8_t {
    integer = 1,
    text = 2,
};

class IGameDataFieldSchemaRuntime {
public:
    virtual ~IGameDataFieldSchemaRuntime() = default;
    virtual void RegisterField(std::uint32_t binding,
                               std::string_view name,
                               std::uint32_t offset,
                               GameDataFieldKind kind) = 0;
};

}  // namespace pal4
