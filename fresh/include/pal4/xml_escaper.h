#pragma once

#include <string>
#include <string_view>

namespace pal4 {

class XmlEscaper final {
public:
    // IDA 0x5f03e0.
    static std::string& Append(std::string& output, std::string_view input);
};

}  // namespace pal4
