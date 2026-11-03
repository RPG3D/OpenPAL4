#pragma once

#include <string>

namespace pal4 {
struct BinaryXmlAttribute;

// Host-owned text using the existing XML attribute conversion rules.
class NativeXmlAttributeText final {
public:
    [[nodiscard]] static std::string Convert(const BinaryXmlAttribute& attribute);
};
}  // namespace pal4
