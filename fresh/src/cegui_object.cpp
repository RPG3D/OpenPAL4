#include "pal4/cegui_object.h"

namespace pal4 {

std::uint32_t CeguiObject::Construct(
    CeguiObjectState& state, const std::uint32_t address,
    const std::uint8_t first_source_byte,
    const std::uint8_t second_source_byte,
    const std::uint8_t flag,
    ICeguiObjectRuntime& runtime) {
    state.Bytes()[0] = second_source_byte;
    state.Bytes()[8] = flag;
    state.Bytes()[1] = first_source_byte;
    runtime.InitializeList(address);
    return address;
}

std::uint32_t CeguiObject::InitializeList(
    CeguiObjectState& state, CeguiTreeGlobals& globals,
    ICeguiObjectListRuntime& runtime) {
    const auto candidate_nil = runtime.Allocate(0x18u);
    runtime.WriteDword(candidate_nil + 0x14u, 1u);
    runtime.WriteDword(candidate_nil + 4u, 0u);
    if (globals.nil == 0u) {
        globals.nil = candidate_nil;
        runtime.WriteDword(candidate_nil, 0u);
        runtime.WriteDword(candidate_nil + 8u, 0u);
    }
    ++globals.reference_count;
    if (candidate_nil != globals.nil) runtime.Free(candidate_nil);

    const auto header = runtime.Allocate(0x18u);
    runtime.WriteDword(header + 4u, globals.nil);
    runtime.WriteDword(header + 0x14u, 0u);
    state.WriteDword(4u, header);
    state.WriteDword(0x0cu, 0u);
    runtime.WriteDword(header, header);
    runtime.WriteDword(header + 8u, header);
    return header;
}

}  // namespace pal4
