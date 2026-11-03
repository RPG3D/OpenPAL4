#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace pal4::portable {
// PAL4/AngelScript 2.2 wire types. Bytes and slot offsets retain their original
// widths regardless of the host pointer size. Text remains original GBK bytes.
struct CsbType {
    std::uint32_t token{};
    std::string object;
    bool handle{},read_only{},handle_to_const{},reference{};
    std::vector<CsbType> element;
};
struct CsbProperty {std::string name;CsbType type;std::int32_t index{};};
struct CsbStruct {std::string name;std::uint32_t bytes{};std::vector<CsbProperty> properties;};
struct CsbInstruction {
    std::uint32_t offset{},file_offset{};
    std::uint8_t opcode{},bytes{};
    std::uint64_t operand{};
};
struct CsbObjectVariable {std::string type;std::int32_t position{};};
struct CsbFunction {
    std::string name;CsbType result;std::vector<CsbType> parameters;
    std::int32_t id{};
    std::uint32_t bytecode_bytes{},stack_slots{};
    std::vector<CsbInstruction> instructions;
    std::vector<CsbObjectVariable> object_variables;
    std::string object_type;
    std::vector<std::uint32_t> line_numbers;
};
struct CsbImport {CsbFunction function;std::int32_t from{};};
struct CsbModule {
    std::vector<CsbStruct> structures;
    std::vector<std::string> used_types;
    std::vector<CsbProperty> globals;
    std::uint32_t global_slots{};
    CsbFunction initialize,finalize;
    std::vector<CsbFunction> functions;
    std::vector<std::string> strings;
    std::vector<CsbImport> imports;
    const CsbFunction* Find(std::string_view name) const;
};
CsbModule ReadCsb(std::span<const std::uint8_t> bytes);
unsigned CsbInstructionBytes(std::uint8_t opcode);
// Empty target range for ordinary opcodes. All offsets use expanded VM bytes,
// never compact file positions. Includes direct conditional/unconditional jumps.
bool CsbDirectJump(std::uint8_t opcode);
} // namespace pal4::portable
