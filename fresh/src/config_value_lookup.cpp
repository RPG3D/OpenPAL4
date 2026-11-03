#include "pal4/config_value_lookup.h"

#include <algorithm>

namespace pal4 {

ConfigStringState& ConfigValueLookup::AssignMessage(
    ConfigStringState& text, const std::uint32_t source,
    const std::uint32_t length, IConfigValueMemory& memory) {
    if (length > 0xfffffffdu) {
        memory.LengthError();
    }
    if (text.data != 0u) {
        const std::uint8_t references = memory.ReadByte(text.data - 1u);
        if (references != 0u && references != 0xffu) {
            if (length == 0u) {
                memory.WriteByte(text.data - 1u,
                                 static_cast<std::uint8_t>(references - 1u));
                text.data = 0u;
                text.size = 0u;
                text.capacity = 0u;
                return text;
            }
            memory.Reallocate(text, length);
            memory.CopyBytes(text.data, source, length);
            text.size = length;
            memory.WriteByte(text.data + length, 0u);
            return text;
        }
    }
    if (length == 0u) {
        memory.SetErrorFlag(text, 1u);
        return text;
    }
    if (text.capacity > 0x1fu || text.capacity < length) {
        memory.SetErrorFlag(text, 1u);
        memory.Reallocate(text, length);
    }
    memory.CopyBytes(text.data, source, length);
    text.size = length;
    memory.WriteByte(text.data + length, 0u);
    return text;
}

std::uint32_t ConfigValueLookup::FindConfigItem(
    const std::uint32_t tree_address, const ConfigStringState& key,
    IConfigValueMemory& memory) {
    const std::uint32_t header = memory.ReadDword(tree_address + 4u);
    const std::uint32_t nil = memory.ReadDword(kNilNodeGlobal);
    std::uint32_t candidate = header;
    std::uint32_t node = memory.ReadDword(header + 4u);
    while (node != nil) {
        const std::uint32_t data =
            key.data == 0u ? memory.EmptyStringAddress() : key.data;
        const std::int32_t comparison = memory.CompareRange(
            node + 0x0cu, 0u, memory.ReadDword(node + 0x14u),
            data, key.size);
        if (comparison < 0) {
            node = memory.ReadDword(node + 8u);
        } else {
            candidate = node;
            node = memory.ReadDword(node);
        }
    }
    return candidate;
}

std::int32_t ConfigValueLookup::CompareText(
    const ConfigStringState& left, const std::uint32_t right_key_address,
    IConfigValueMemory& memory) {
    std::uint32_t right = memory.ReadDword(right_key_address + 4u);
    const std::uint32_t right_length =
        memory.ReadDword(right_key_address + 8u);
    if (right == 0u) {
        right = memory.EmptyStringAddress();
    }
    const std::uint32_t common = std::min(left.size, right_length);
    for (std::uint32_t index = 0u; index < common; ++index) {
        const auto lhs = memory.ReadByte(left.data + index);
        const auto rhs = memory.ReadByte(right + index);
        if (lhs < rhs) {
            return -1;
        }
        if (lhs > rhs) {
            return 1;
        }
    }
    if (left.size < right_length) {
        return -1;
    }
    if (left.size > right_length) {
        return 1;
    }
    return 0;
}

std::uint32_t ConfigValueLookup::GetConfigStringValue(
    const std::uint32_t document_address,
    const std::uint32_t item_name, IConfigValueMemory& memory) {
    ConfigStringState key{};
    key.flag = static_cast<std::uint8_t>(item_name);
    (void)AssignMessage(key, item_name, memory.StringLength(item_name),
                        memory);
    const std::uint32_t tree_address = document_address + 0x14u;
    const std::uint32_t header = memory.ReadDword(tree_address + 4u);
    std::uint32_t selected = FindConfigItem(tree_address, key, memory);
    if (selected != header &&
        CompareText(key, selected + 0x0cu, memory) < 0) {
        selected = header;
    }
    if (key.data != 0u) {
        const std::uint32_t header_address = key.data - 1u;
        const std::uint8_t references = memory.ReadByte(header_address);
        if (references == 0u || references == 0xffu) {
            memory.Free(header_address);
        } else {
            memory.WriteByte(
                header_address,
                static_cast<std::uint8_t>(references - 1u));
        }
    }
    return selected == header ? 0u : selected + 0x1cu;
}

}  // namespace pal4
