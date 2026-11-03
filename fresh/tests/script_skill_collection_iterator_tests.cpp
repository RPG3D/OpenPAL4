#include "pal4/script_skill_collection_iterator.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptSkillCollectionIteratorRuntime {
    std::uint32_t begin = 0x8000u;
    std::uint32_t allocation = 0xa000u;
    std::uint32_t virtual_result = 0u;
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        const auto found = memory.find(address);
        return found == memory.end() ? begin : found->second;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write " + std::to_string(address) + " " +
                        std::to_string(value));
        memory[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate " + std::to_string(size));
        return allocation;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
    std::uint32_t InvokeVirtual(std::uint32_t object,
                                std::uint32_t offset) override {
        calls.push_back("virtual " + std::to_string(object) + " " +
                        std::to_string(offset));
        return virtual_result;
    }
};

struct NameRuntime final : pal4::IScriptNameNodeConstructorRuntime {
    std::vector<std::uint32_t> calls;
    void ConstructNameNode(std::uint32_t storage) override {
        calls.push_back(storage);
    }
};

void BothWrappers() {
    struct Spec {
        std::uint32_t entry;
        std::uint32_t begin_offset;
        std::uint32_t header_offset;
        std::uint32_t base_vtable;
        std::uint32_t vtable;
    };
    for (const auto spec : std::array{
             Spec{0x5894c0u, 696u, 692u, 0x848558u, 0x848500u},
             Spec{0x589520u, 680u, 676u, 0x848584u, 0x84852cu}}) {
        Runtime runtime;
        Check(pal4::ScriptSkillCollectionIterator::CreateFromCollection(
                  spec.entry, 0x7000u, 0x6000u, runtime) == 0x6000u &&
                  runtime.calls == std::vector<std::string>{
                      "read " + std::to_string(0x7000u + spec.begin_offset),
                      "allocate 12",
                      "write 40960 " + std::to_string(spec.base_vtable),
                      "write 40964 32768",
                      "write 40968 " +
                          std::to_string(0x7000u + spec.header_offset),
                      "write 40960 " + std::to_string(spec.vtable),
                      "write 24576 40960"},
              "iterator wrapper copies cursor and vector header to allocated object");
        runtime.calls.clear();
        runtime.allocation = 0u;
        Check(pal4::ScriptSkillCollectionIterator::CreateFromCollection(
                  spec.entry, 0x7000u, 0x6000u, runtime) == 0x6000u &&
                  runtime.calls == std::vector<std::string>{
                      "read " + std::to_string(0x7000u + spec.begin_offset),
                      "allocate 12", "write 24576 0"},
              "allocation failure writes a null iterator to output");
    }
}

void InvalidEntries() {
    Runtime runtime;
    for (auto entry : {0x5894bfu, 0x5894c1u, 0x589520u}) {
        bool rejected = false;
        try {
            static_cast<void>(pal4::ScriptSkillCollectionIterator::CreateObject(
                entry, 0x8000u, 0x9000u, runtime));
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        Check(rejected && runtime.calls.empty(),
              "non-constructor address cannot allocate an iterator");
    }
    for (auto entry : {0x5894c1u, 0x589521u, 0x589bc0u}) {
        bool rejected = false;
        try {
            static_cast<void>(
                pal4::ScriptSkillCollectionIterator::CreateFromCollection(
                    entry, 0x7000u, 0x6000u, runtime));
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        Check(rejected && runtime.calls.empty(),
              "non-wrapper address has no iterator side effects");
    }
}

void IteratorOperations() {
    Runtime runtime;
    constexpr auto iterator = 0x7000u;
    constexpr auto header = 0x9000u;
    runtime.memory[iterator + 8u] = header;
    runtime.memory[header + 4u] = 0xa000u;
    runtime.memory[header + 8u] = 0xa030u;
    Check(pal4::ScriptSkillCollectionIterator::Count(
              0x589bf0u, iterator, runtime) == 6 &&
              pal4::ScriptSkillCollectionIterator::Count(
                  0x589c90u, iterator, runtime) == 4,
          "iterator counts use the 8-byte and 12-byte vector strides");
    runtime.memory[header + 4u] = 0u;
    Check(pal4::ScriptSkillCollectionIterator::Count(
              0x589bf0u, iterator, runtime) == 0 &&
              pal4::ScriptSkillCollectionIterator::Count(
                  0x589c90u, iterator, runtime) == 0,
          "null vector begin has zero elements");
    runtime.memory[header + 4u] = 0xa000u;
    runtime.memory[header + 8u] = 0x9fffu;
    Check(pal4::ScriptSkillCollectionIterator::Count(
              0x589bf0u, iterator, runtime) == -1,
          "8-byte count preserves arithmetic shift of negative byte span");

    for (const auto [entry, offset, delta] : std::array{
             std::array<std::uint32_t, 3>{0x589c10u, 24u, 8u},
             std::array<std::uint32_t, 3>{0x589c30u, 20u, 8u},
             std::array<std::uint32_t, 3>{0x589cc0u, 24u, 12u},
             std::array<std::uint32_t, 3>{0x589ce0u, 20u, 12u}}) {
        runtime.calls.clear();
        runtime.memory[iterator + 4u] = 0x8000u;
        runtime.virtual_result = 0x100u;
        const auto expected = offset == 24u ? 0x8000u + delta :
                                              0x8000u - delta;
        Check(pal4::ScriptSkillCollectionIterator::Move(
                  entry, iterator, runtime) == 0x100u &&
                  runtime.memory[iterator + 4u] == expected &&
                  runtime.calls.front() ==
                      "virtual 28672 " + std::to_string(offset),
              "zero low result byte advances or reverses cursor");
        runtime.virtual_result = 1u;
        Check(pal4::ScriptSkillCollectionIterator::Move(
                  entry, iterator, runtime) == 1u &&
                  runtime.memory[iterator + 4u] == expected,
              "nonzero low result byte leaves cursor unchanged");
    }

    runtime.memory[header + 4u] = 0xa000u;
    runtime.memory[header + 8u] = 0xa024u;
    Check(pal4::ScriptSkillCollectionIterator::CalculateOffset(
              header, runtime) == 3,
          "12-byte vector helper returns element count");
    runtime.memory[header + 4u] = 0u;
    Check(pal4::ScriptSkillCollectionIterator::CalculateOffset(
              header, runtime) == 0,
          "12-byte vector helper handles null begin");
}

void InitializersAndDestructors() {
    for (const auto [initializer, vtable] : std::array{
             std::array<std::uint32_t, 2>{0x589bb0u, 0x848558u},
             std::array<std::uint32_t, 2>{0x589c50u, 0x848584u}}) {
        Runtime runtime;
        pal4::ScriptSkillCollectionIterator::InitializeBase(
            initializer, 0x7000u, runtime);
        Check(runtime.calls == std::vector<std::string>{
                  "write 28672 " + std::to_string(vtable)},
              "base iterator initializer writes exact vtable");
    }
    for (const auto [entry, vtable] : std::array{
             std::array<std::uint32_t, 2>{0x589d00u, 0x848558u},
             std::array<std::uint32_t, 2>{0x589d20u, 0x848558u},
             std::array<std::uint32_t, 2>{0x589d40u, 0x848584u},
             std::array<std::uint32_t, 2>{0x589d60u, 0x848584u}}) {
        for (std::uint32_t flags = 0u; flags < 4u; ++flags) {
            Runtime runtime;
            Check(pal4::ScriptSkillCollectionIterator::Destroy(
                      entry, 0x7000u, flags, runtime) == 0x7000u,
                  "iterator destructor returns object address");
            const auto write = "write 28672 " + std::to_string(vtable);
            const auto expected = (flags & 1u) != 0u
                ? std::vector<std::string>{write, "free 28672"}
                : std::vector<std::string>{write};
            Check(runtime.calls == expected,
                  "iterator destructor writes vtable before optional free");
        }
    }
}

void NameNodeWrapper() {
    NameRuntime runtime;
    Check(pal4::ScriptNameNodeConstructor::Construct(
              0x7000u, 13u, 42u, runtime) == 0x7000u &&
              runtime.calls == std::vector<std::uint32_t>{0x7000u},
          "name-node wrapper delegates construction and returns storage");
}

}  // namespace

int main() {
    try {
        BothWrappers();
        InvalidEntries();
        IteratorOperations();
        InitializersAndDestructors();
        NameNodeWrapper();
        std::cout << "script skill collection iterator tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
