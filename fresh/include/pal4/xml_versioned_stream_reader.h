#pragma once

#include <cstdint>

#include "pal4/xml_node_data_reader.h"

namespace pal4 {

class XmlVersionedStreamReader final {
public:
    // IDA 0x5f7dc0. The nonzero stream address identifies the original
    // virtual stream; input and node runtime bind to that same stream.
    [[nodiscard]] static std::uint8_t Read(
        std::uint32_t stream, std::uint32_t destination,
        IXmlNodeDataInput& input, IXmlNodeDataRuntime& nodes);
};

}  // namespace pal4
