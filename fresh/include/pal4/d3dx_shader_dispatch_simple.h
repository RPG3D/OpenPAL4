#pragma once

#include "pal4/d3dx_shader_dispatch_table.h"

#include <cstdint>
#include <span>

namespace pal4 {

class ID3DXShaderDispatchSimpleRuntime {
public:
    virtual ~ID3DXShaderDispatchSimpleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t AppendList(
        std::uint32_t first, std::uint32_t second) = 0;
    virtual void MergeUsage(std::uint32_t destination,
                            std::uint32_t incoming) = 0;
    virtual void PopScope() = 0;
};

class D3DXShaderDispatchSimple final {
public:
    // IDA 0x6906b9/0x6906c1/0x6906d3/0x69083d/0x690c45/0x690f1f.
    [[nodiscard]] static bool Execute(
        std::uint32_t opcode, std::span<const std::uint32_t> arguments,
        ID3DXShaderDispatchSimpleRuntime& runtime,
        std::uint32_t& result);
};

}  // namespace pal4
