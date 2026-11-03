#include "pal4/effect_system_auxiliary_tree_insert.h"

namespace pal4 {

std::uint32_t IEffectSystemAuxiliaryInsertRuntime::SharedNil() const {
    return Kind() == EffectRegistrationTree::Clumps
               ? 0x93ce10u : 0x93cdfcu;
}

std::uint32_t IEffectSystemAuxiliaryInsertRuntime::NodeSize() const {
    return Kind() == EffectRegistrationTree::Clumps ? 20u : 24u;
}

std::uint32_t IEffectSystemAuxiliaryInsertRuntime::ColorOffset() const {
    return Kind() == EffectRegistrationTree::Clumps ? 16u : 20u;
}

std::int32_t IEffectSystemAuxiliaryInsertRuntime::CompareRange(
    std::uint32_t input, std::uint32_t node) {
    return CompareText(input, node);
}

std::int32_t IEffectSystemAuxiliaryInsertRuntime::CompareText(
    std::uint32_t first, std::uint32_t second) {
    const auto a = ReadDword(first);
    const auto b = ReadDword(second);
    return a < b ? -1 : (a > b ? 1 : 0);
}

void IEffectSystemAuxiliaryInsertRuntime::AssignText(
    std::uint32_t destination, std::uint32_t source) {
    WriteDword(destination, ReadDword(source));
    if (Kind() == EffectRegistrationTree::Atomics)
        WriteDword(destination + 4u, ReadDword(source + 4u));
}

void IEffectSystemAuxiliaryInsertRuntime::SetNodeData(
    MonsterManagerTreeInsertResult& result,
    std::uint32_t candidate) {
    result = {candidate, false};
}

std::uint32_t EffectSystemAuxiliaryTreeInsert::InsertAt(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t child, std::uint32_t parent,
    std::uint32_t key_address,
    IEffectSystemAuxiliaryInsertRuntime& runtime) {
    const auto node = MonsterManagerTreeInsert::InsertHelper(
        tree, child, parent, key_address, runtime);
    runtime.WriteDword(result, node);
    return result;
}

}  // namespace pal4
