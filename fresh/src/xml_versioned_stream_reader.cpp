#include "pal4/xml_versioned_stream_reader.h"

namespace pal4 {
namespace {

std::int32_t ReadSignedWord(IXmlNodeDataInput& input) {
    std::uint8_t bytes[4]{};
    input.Read(bytes, sizeof(bytes));
    const auto bits = static_cast<std::uint32_t>(bytes[0]) |
                      (static_cast<std::uint32_t>(bytes[1]) << 8u) |
                      (static_cast<std::uint32_t>(bytes[2]) << 16u) |
                      (static_cast<std::uint32_t>(bytes[3]) << 24u);
    return static_cast<std::int32_t>(bits);
}

}  // namespace

std::uint8_t XmlVersionedStreamReader::Read(
    const std::uint32_t stream, const std::uint32_t destination,
    IXmlNodeDataInput& input, IXmlNodeDataRuntime& nodes) {
    if (stream == 0u || destination == 0u) return 0u;
    const auto major = ReadSignedWord(input);
    const auto minor = ReadSignedWord(input);
    if (major > 1 || (major == 1 && minor > 1)) return 0u;
    return XmlNodeDataReader::ReadVirtualStream(0u, destination, nodes);
}

}  // namespace pal4
