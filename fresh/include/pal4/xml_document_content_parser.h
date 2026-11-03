#pragma once

#include "pal4/xml_element_parser.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IXmlDocumentContentRuntime : public IXmlElementRuntime {
};

class XmlDocumentContentParser final {
public:
    // IDA 0x5f2490. Source and result are 32-bit addresses in the runtime.
    [[nodiscard]] static std::uint32_t Parse(
        std::uint32_t document, std::uint32_t source,
        const std::array<std::uint32_t, 2>* position,
        int mode, IXmlDocumentContentRuntime& runtime);
};

}  // namespace pal4
