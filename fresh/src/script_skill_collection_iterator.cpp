#include "pal4/script_skill_collection_iterator.h"

#include <bit>
#include <stdexcept>

namespace pal4 {
namespace {

struct IteratorSpec {
    std::uint32_t begin_offset;
    std::uint32_t header_offset;
    std::uint32_t constructor;
    std::uint32_t base_vtable;
    std::uint32_t vtable;
};

IteratorSpec Spec(std::uint32_t native_entry) {
    switch (native_entry) {
        case 0x5894c0u:
        case 0x589bc0u:
            return {696u, 692u, 0x589bc0u, 0x848558u, 0x848500u};
        case 0x589520u:
        case 0x589c60u:
            return {680u, 676u, 0x589c60u, 0x848584u, 0x84852cu};
        default:
            throw std::invalid_argument("unknown skill collection iterator entry");
    }
}

}  // namespace

std::uint32_t ScriptSkillCollectionIterator::CreateFromCollection(
    std::uint32_t native_entry, std::uint32_t collection,
    std::uint32_t output, IScriptSkillCollectionIteratorRuntime& runtime) {
    if (native_entry != 0x5894c0u && native_entry != 0x589520u)
        throw std::invalid_argument("unknown collection iterator wrapper");
    const auto spec = Spec(native_entry);
    const auto cursor = runtime.ReadWord(collection + spec.begin_offset);
    const auto created = CreateObject(spec.constructor, cursor,
                                      collection + spec.header_offset,
                                      runtime);
    runtime.WriteWord(output, created);
    return output;
}

std::uint32_t ScriptSkillCollectionIterator::CreateObject(
    std::uint32_t native_entry, std::uint32_t cursor,
    std::uint32_t vector_header,
    IScriptSkillCollectionIteratorRuntime& runtime) {
    if (native_entry != 0x589bc0u && native_entry != 0x589c60u)
        throw std::invalid_argument("unknown collection iterator constructor");
    const auto spec = Spec(native_entry);
    const auto created = runtime.Allocate(12u);
    if (created == 0u) return 0u;
    InitializeBase(native_entry == 0x589bc0u ? 0x589bb0u : 0x589c50u,
                   created, runtime);
    runtime.WriteWord(created + 4u, cursor);
    runtime.WriteWord(created + 8u, vector_header);
    runtime.WriteWord(created, spec.vtable);
    return created;
}

void ScriptSkillCollectionIterator::InitializeBase(
    std::uint32_t native_entry, std::uint32_t object,
    IScriptSkillCollectionIteratorRuntime& runtime) {
    const auto vtable = native_entry == 0x589bb0u ? 0x848558u :
                        native_entry == 0x589c50u ? 0x848584u : 0u;
    if (vtable == 0u)
        throw std::invalid_argument("unknown collection iterator base initializer");
    runtime.WriteWord(object, vtable);
}

std::int32_t ScriptSkillCollectionIterator::Count(
    std::uint32_t native_entry, std::uint32_t object,
    IScriptSkillCollectionIteratorRuntime& runtime) {
    if (native_entry != 0x589bf0u && native_entry != 0x589c90u)
        throw std::invalid_argument("unknown collection iterator count entry");
    const auto header = runtime.ReadWord(object + 8u);
    const auto begin = runtime.ReadWord(header + 4u);
    if (begin == 0u) return 0;
    const auto end = runtime.ReadWord(header + 8u);
    const auto bytes = std::bit_cast<std::int32_t>(end - begin);
    return native_entry == 0x589bf0u ? bytes >> 3 : bytes / 12;
}

std::uint32_t ScriptSkillCollectionIterator::Move(
    std::uint32_t native_entry, std::uint32_t object,
    IScriptSkillCollectionIteratorRuntime& runtime) {
    std::uint32_t slot;
    std::uint32_t width;
    bool advance;
    switch (native_entry) {
        case 0x589c10u: slot = 24u; width = 8u; advance = true; break;
        case 0x589c30u: slot = 20u; width = 8u; advance = false; break;
        case 0x589cc0u: slot = 24u; width = 12u; advance = true; break;
        case 0x589ce0u: slot = 20u; width = 12u; advance = false; break;
        default:
            throw std::invalid_argument("unknown collection iterator move entry");
    }
    const auto result = runtime.InvokeVirtual(object, slot);
    if (static_cast<std::uint8_t>(result) == 0u) {
        const auto cursor = runtime.ReadWord(object + 4u);
        runtime.WriteWord(object + 4u,
                          advance ? cursor + width : cursor - width);
    }
    return result;
}

std::uint32_t ScriptSkillCollectionIterator::Destroy(
    std::uint32_t native_entry, std::uint32_t object,
    std::uint32_t flags,
    IScriptSkillCollectionIteratorRuntime& runtime) {
    std::uint32_t initializer;
    switch (native_entry) {
        case 0x589d00u:
        case 0x589d20u: initializer = 0x589bb0u; break;
        case 0x589d40u:
        case 0x589d60u: initializer = 0x589c50u; break;
        default:
            throw std::invalid_argument("unknown collection iterator destructor");
    }
    InitializeBase(initializer, object, runtime);
    if ((flags & 1u) != 0u) runtime.Free(object);
    return object;
}

std::int32_t ScriptSkillCollectionIterator::CalculateOffset(
    std::uint32_t vector_header,
    IScriptSkillCollectionIteratorRuntime& runtime) {
    const auto begin = runtime.ReadWord(vector_header + 4u);
    if (begin == 0u) return 0;
    const auto end = runtime.ReadWord(vector_header + 8u);
    return std::bit_cast<std::int32_t>(end - begin) / 12;
}

std::uint32_t ScriptNameNodeConstructor::Construct(
    std::uint32_t storage, std::uint32_t, std::uint32_t,
    IScriptNameNodeConstructorRuntime& runtime) {
    runtime.ConstructNameNode(storage);
    return storage;
}

}  // namespace pal4
