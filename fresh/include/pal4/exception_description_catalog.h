#pragma once

#include <cstdint>

namespace pal4 {

class ExceptionDescriptionCatalog final {
public:
    [[nodiscard]] static const char* Lookup(std::uint32_t code);
};

}  // namespace pal4
