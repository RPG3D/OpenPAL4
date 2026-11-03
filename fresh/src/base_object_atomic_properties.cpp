#include "pal4/base_object_atomic_properties.h"

namespace pal4 {
namespace {

std::uint32_t SetProperty(BaseObjectState& state, std::uint32_t property,
                          std::uint32_t string_offset,
                          std::uint32_t pointer_offset,
                          IBaseObjectAtomicRuntime& runtime) {
    const auto object_name = state.Read(8u);
    runtime.InitString(state, string_offset, object_name,
                       runtime.StringLength(object_name));
    const auto property_name = runtime.ReadDword(property) + 16u;
    const auto result = runtime.AppendString(
        state, string_offset, property_name,
        runtime.StringLength(property_name));
    state.Write(pointer_offset, property);
    return result;
}

std::uint32_t ProcessAtomic(BaseObjectState& state, std::uint32_t atomic,
                            std::string_view prefix, bool face,
                            IBaseObjectAtomicRuntime& runtime) {
    const auto collection = runtime.ReadDword(atomic + 24u);
    const auto count = runtime.UsedCount(collection);
    for (std::int32_t index = 0; index < count; ++index) {
        const auto record = runtime.RecordAt(collection, index);
        if (runtime.ReadDword(record + 4u) != 3u) continue;
        const auto text = runtime.RecordText(record, index);
        if (text.size() > prefix.size() &&
            text.substr(0u, prefix.size()) == prefix) {
            const auto property = runtime.ReadDword(
                runtime.ReadDword(collection + 32u));
            if (face) {
                (void)BaseObjectAtomicProperties::SetFaceProperty(
                    state, property, runtime);
            } else {
                (void)BaseObjectAtomicProperties::SetLegProperty(
                    state, property, runtime);
            }
            return 0u;
        }
    }
    return atomic;
}

std::uint32_t ProcessAtomics(BaseObjectState& state, bool face,
                             IBaseObjectAtomicRuntime& runtime) {
    const auto clump = state.Read(0x14u);
    if (clump == 0u) return 0u;
    state.Write(face ? 0x550u : 0x560u, 0u);
    return runtime.ForAllAtomics(clump, [&](std::uint32_t atomic) {
        return face ?
            BaseObjectAtomicProperties::ProcessFaceAtomic(state, atomic,
                                                            runtime) :
            BaseObjectAtomicProperties::ProcessLegAtomic(state, atomic,
                                                           runtime);
    });
}

}  // namespace

std::uint32_t BaseObjectAtomicProperties::ProcessFaceAtomics(
    BaseObjectState& state, IBaseObjectAtomicRuntime& runtime) {
    return ProcessAtomics(state, true, runtime);
}

std::uint32_t BaseObjectAtomicProperties::ProcessLegAtomics(
    BaseObjectState& state, IBaseObjectAtomicRuntime& runtime) {
    return ProcessAtomics(state, false, runtime);
}

std::uint32_t BaseObjectAtomicProperties::ProcessFaceAtomic(
    BaseObjectState& state, std::uint32_t atomic,
    IBaseObjectAtomicRuntime& runtime) {
    return ProcessAtomic(state, atomic, "FACE", true, runtime);
}

std::uint32_t BaseObjectAtomicProperties::ProcessLegAtomic(
    BaseObjectState& state, std::uint32_t atomic,
    IBaseObjectAtomicRuntime& runtime) {
    return ProcessAtomic(state, atomic, "LEG", false, runtime);
}

std::uint32_t BaseObjectAtomicProperties::SetFaceProperty(
    BaseObjectState& state, std::uint32_t property,
    IBaseObjectAtomicRuntime& runtime) {
    return SetProperty(state, property, 0x544u, 0x550u, runtime);
}

std::uint32_t BaseObjectAtomicProperties::SetLegProperty(
    BaseObjectState& state, std::uint32_t property,
    IBaseObjectAtomicRuntime& runtime) {
    return SetProperty(state, property, 0x554u, 0x560u, runtime);
}

}  // namespace pal4
