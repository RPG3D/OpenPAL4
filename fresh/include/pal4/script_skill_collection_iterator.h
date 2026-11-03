#pragma once

#include <cstdint>

namespace pal4 {

class IScriptSkillCollectionIteratorRuntime {
public:
    virtual ~IScriptSkillCollectionIteratorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeVirtual(
        std::uint32_t object, std::uint32_t vtable_offset) = 0;
};

class ScriptSkillCollectionIterator final {
public:
    // IDA 0x5894c0 / 0x589520: construct a 12-byte iterator from a list.
    [[nodiscard]] static std::uint32_t CreateFromCollection(
        std::uint32_t native_entry, std::uint32_t collection,
        std::uint32_t output, IScriptSkillCollectionIteratorRuntime& runtime);

    // IDA 0x589bc0 / 0x589c60: allocating copy called by the two wrappers.
    [[nodiscard]] static std::uint32_t CreateObject(
        std::uint32_t native_entry, std::uint32_t cursor,
        std::uint32_t vector_header,
        IScriptSkillCollectionIteratorRuntime& runtime);

    // IDA 0x589bb0 / 0x589c50.
    static void InitializeBase(std::uint32_t native_entry,
                               std::uint32_t object,
                               IScriptSkillCollectionIteratorRuntime& runtime);
    // IDA 0x589bf0 / 0x589c90.
    [[nodiscard]] static std::int32_t Count(
        std::uint32_t native_entry, std::uint32_t object,
        IScriptSkillCollectionIteratorRuntime& runtime);
    // IDA 0x589c10/0x589c30/0x589cc0/0x589ce0.
    [[nodiscard]] static std::uint32_t Move(
        std::uint32_t native_entry, std::uint32_t object,
        IScriptSkillCollectionIteratorRuntime& runtime);
    // IDA 0x589d00/0x589d20/0x589d40/0x589d60.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t native_entry, std::uint32_t object,
        std::uint32_t flags,
        IScriptSkillCollectionIteratorRuntime& runtime);
    // IDA 0x589d80.
    [[nodiscard]] static std::int32_t CalculateOffset(
        std::uint32_t vector_header,
        IScriptSkillCollectionIteratorRuntime& runtime);
};

class IScriptNameNodeConstructorRuntime {
public:
    virtual ~IScriptNameNodeConstructorRuntime() = default;
    virtual void ConstructNameNode(std::uint32_t storage) = 0;
};

class ScriptNameNodeConstructor final {
public:
    // IDA 0x589b90. The two extra stdcall arguments are unused.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t storage, std::uint32_t unused_one,
        std::uint32_t unused_two,
        IScriptNameNodeConstructorRuntime& runtime);
};

}  // namespace pal4
