#pragma once
#include <cstdint>

namespace pal4 {
class NativeBaseObjectMemory;
class BinaryXmlConfig;

// Applies a soMonster snapshot through shared IDA432110 storage gating.
// The config is the current monster info record's +4C buffer-cache document.
class NativeMonsterStorageRestore final {
public:
    [[nodiscard]] static std::uint32_t Apply(NativeBaseObjectMemory& memory,
        std::uint32_t object, const BinaryXmlConfig& config,
        const BinaryXmlConfig& snapshot);
};
}
