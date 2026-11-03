#pragma once

#include "pal4/xml_element_parser.h"

#include <array>
#include <cstdint>

namespace pal4 {

class XmlContentParser final {
public:
    // IDA 0x5f2cd0. Text and non-element parse callbacks are later entries.
    [[nodiscard]] static std::uint32_t Parse(
        std::uint32_t element, std::uint32_t source,
        std::array<std::uint32_t, 4>* position, int mode,
        IXmlElementRuntime& runtime);
};

}  // namespace pal4
