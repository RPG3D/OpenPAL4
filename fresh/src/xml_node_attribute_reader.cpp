#include "pal4/xml_node_attribute_reader.h"

namespace pal4 {

ConfigStringState& XmlNodeAttributeReader::ReadString(
    const std::uint32_t document_address, ConfigStringState& output,
    const std::uint32_t key_address,
    const std::uint32_t fallback_address,
    IXmlNodeAttributeReaderRuntime& runtime) {
    const auto query = runtime.CreateQuery(
        key_address, static_cast<std::uint8_t>(fallback_address));
    std::uint32_t found = 0u;
    try {
        (void)ScriptAttributeTreeLookup::Find(
            document_address + 0x14u, found, query,
            runtime.TreeRuntime());
    } catch (...) {
        runtime.DestroyQuery(query);
        throw;
    }
    runtime.DestroyQuery(query);

    const auto header = runtime.TreeRuntime().ReadDword(
        document_address + 0x18u);
    if (found != header && found + 0x1cu != 0u) {
        runtime.AttributeToString(found + 0x1cu, output,
                                  fallback_address);
        return output;
    }

    output = {};
    output.flag = static_cast<std::uint8_t>(fallback_address);
    const auto length = runtime.StringLength(fallback_address);
    StringBuffer buffer;
    if (buffer.AllocateText(length, 1u, runtime.Strings())) {
        output.data = buffer.State().data;
        output.capacity = buffer.State().capacity;
        try {
            runtime.Strings().Copy(output.data, fallback_address,
                                   length);
            output.size = length;
            runtime.Strings().WriteByte(output.data + length, 0u);
        } catch (...) {
            const auto data = output.data;
            if (data != 0u) {
                const auto reference = runtime.Strings().ReadByte(data - 1u);
                if (reference == 0u || reference == 0xffu)
                    runtime.Strings().Free(data - 1u);
                else
                    runtime.Strings().WriteByte(data - 1u,
                        static_cast<std::uint8_t>(reference - 1u));
            }
            output = {};
            throw;
        }
    }
    return output;
}

}  // namespace pal4
