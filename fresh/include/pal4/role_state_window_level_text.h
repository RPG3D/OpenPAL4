#pragma once

#include "pal4/string_buffer.h"

#include <cstdint>
#include <string>

namespace pal4 {

class IRoleStateWindowLevelRuntime : public IStringBufferRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ZeroLiteralAddress() = 0;
};

class RoleStateWindowLevelText final {
public:
    [[nodiscard]] static std::string FormatText(std::int32_t value);
    [[nodiscard]] static StringBuffer& FormatInto(
        StringBuffer& output, std::int32_t value,
        IRoleStateWindowLevelRuntime& runtime);
};

}  // namespace pal4
