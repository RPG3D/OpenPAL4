#include "pal4/d3dx_shader_dispatch_simple.h"

namespace pal4 {

bool D3DXShaderDispatchSimple::Execute(
    const std::uint32_t opcode,
    const std::span<const std::uint32_t> arguments,
    ID3DXShaderDispatchSimpleRuntime& runtime,
    std::uint32_t& result) {
    switch (D3DXShaderDispatchTable::HandlerAddress(opcode)) {
    case 0x6906b9:
        if (arguments.empty()) return false;
        result = arguments[0];
        return true;
    case 0x6906c1:
        if (arguments.size() < 2) return false;
        result = runtime.AppendList(arguments[0], arguments[1]);
        return true;
    case 0x6906d3:
        if (arguments.size() < 2) return false;
        result = arguments[0];
        runtime.MergeUsage(arguments[0], arguments[1]);
        return true;
    case 0x69083d:
        if (arguments.size() < 2) return false;
        result = arguments[1];
        return true;
    case 0x690c45:
        result = 0;
        runtime.PopScope();
        return true;
    case 0x690f1f:
        result = 0;
        return true;
    default:
        return false;
    }
}

}  // namespace pal4
