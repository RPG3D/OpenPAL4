#include "pal4/portable/csb.h"
#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace pal4::portable {
namespace {
// Same compact->VM lengths as the verified csb_format.cpp and AS2.2 bcSize.
constexpr std::array<std::uint8_t,123> sizes={
    6,6,8,4,6,4,4,6,6,4,4,4,8,6,8,8,
    8,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
    6,8,12,4,4,4,4,4,4,4,4,4,4,4,4,4,
    4,4,4,4,4,4,4,4,4,4,6,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,4,12,8,6,
    6,6,8,4,4,4,6,6,4,4,8};
class Reader {
public:
    std::span<const std::uint8_t> bytes;std::size_t offset{};
    explicit Reader(std::span<const std::uint8_t> source):bytes(source) {}
    [[noreturn]] void Fail(const std::string& reason) const {
        throw std::runtime_error("CSB at byte "+std::to_string(offset)+": "+reason);
    }
    std::uint64_t Number(unsigned count) {
        if(count>bytes.size()-offset)Fail("truncated field");
        std::uint64_t value=0;for(unsigned i=0;i<count;++i)value|=std::uint64_t(bytes[offset++])<<(i*8);
        return value;
    }
    std::uint32_t Word(){return static_cast<std::uint32_t>(Number(4));}
    std::int32_t Signed(){return std::bit_cast<std::int32_t>(Word());}
    bool Flag(){const auto value=Number(1);if(value>1)Fail("invalid boolean");return value!=0;}
    std::uint32_t Count() {
        const auto value=Word();if(value>65536 || value>bytes.size()-offset)Fail("invalid collection size");return value;
    }
    std::string Text() {
        const auto length=Word();
        if(length>bytes.size()-offset)Fail("truncated string");
        const auto begin=bytes.begin()+static_cast<std::ptrdiff_t>(offset);offset+=length;
        return {begin,begin+length};
    }
    CsbType Type(unsigned depth=0) {
        if(depth>64)Fail("type nesting limit exceeded");
        CsbType result;const bool array=Flag();
        if(!array){result.token=Word();result.object=Text();}
        result.handle=Flag();result.read_only=Flag();result.handle_to_const=Flag();result.reference=Flag();
        if(array)result.element.push_back(Type(depth+1));
        return result;
    }
    CsbProperty Property(){CsbProperty p;p.name=Text();p.type=Type();p.index=Signed();return p;}
    CsbFunction Function() {
        CsbFunction f;f.name=Text();f.result=Type();
        for(auto n=Count();n;--n)f.parameters.push_back(Type());
        f.id=Signed();f.bytecode_bytes=Word();
        if(f.bytecode_bytes>16u*1024u*1024u)Fail("bytecode size limit exceeded");
        std::uint32_t position=0;
        while(position<f.bytecode_bytes) {
            CsbInstruction instruction;instruction.offset=position;instruction.file_offset=static_cast<std::uint32_t>(offset);
            instruction.opcode=static_cast<std::uint8_t>(Number(1));
            instruction.bytes=static_cast<std::uint8_t>(CsbInstructionBytes(instruction.opcode));
            if(instruction.bytes>f.bytecode_bytes-position)Fail("instruction exceeds expanded bytecode size");
            instruction.operand=Number(instruction.bytes-4);position+=instruction.bytes;f.instructions.push_back(instruction);
        }
        for(auto n=Count();n;--n) {CsbObjectVariable variable;variable.type=Text();variable.position=Signed();f.object_variables.push_back(std::move(variable));}
        f.stack_slots=Word();if(f.stack_slots>65536)Fail("stack size limit exceeded");
        f.object_type=Text();
        for(auto n=Count();n;--n)f.line_numbers.push_back(Word());
        std::unordered_set<std::uint32_t> starts;
        for(const auto& instruction:f.instructions)starts.insert(instruction.offset);
        for(const auto& instruction:f.instructions)if(CsbDirectJump(instruction.opcode)) {
            const auto displacement=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(instruction.operand));
            const auto target=std::int64_t(instruction.offset)+instruction.bytes+displacement;
            if(target<0 || target>=f.bytecode_bytes || !starts.contains(static_cast<std::uint32_t>(target)))Fail("jump target is not an instruction boundary");
        }
        return f;
    }
};
}
unsigned CsbInstructionBytes(std::uint8_t opcode) {
    if(opcode>=sizes.size())throw std::runtime_error("unsupported CSB opcode "+std::to_string(opcode));
    return sizes[opcode];
}
bool CsbDirectJump(std::uint8_t opcode){return opcode==14 || opcode==15 || opcode==16 || (opcode>=91 && opcode<=94);}
const CsbFunction* CsbModule::Find(std::string_view name) const {
    const auto found=std::find_if(functions.begin(),functions.end(),[&](const auto& function){return function.name==name;});
    return found==functions.end()?nullptr:&*found;
}
CsbModule ReadCsb(std::span<const std::uint8_t> bytes) {
    if(bytes.size()>64u*1024u*1024u)throw std::length_error("CSB size limit exceeded");
    Reader r(bytes);if(r.Word()!=bytes.size()-4)r.Fail("outer length mismatch");
    CsbModule module;
    for(auto n=r.Count();n;--n) {
        CsbStruct structure;structure.name=r.Text();structure.bytes=r.Word();
        for(auto count=r.Count();count;--count)structure.properties.push_back(r.Property());
        module.structures.push_back(std::move(structure));
    }
    for(auto n=r.Count();n;--n)module.used_types.push_back(r.Text());
    for(auto n=r.Count();n;--n)module.globals.push_back(r.Property());
    module.global_slots=r.Word();if(module.global_slots>65536)r.Fail("global slot limit exceeded");
    module.initialize=r.Function();module.finalize=r.Function();
    std::unordered_set<std::int32_t> ids;
    for(auto n=r.Count();n;--n) {
        auto function=r.Function();
        // Overloads are legal: identity is the serialized function ID.
        if(!ids.insert(function.id).second)r.Fail("duplicate function ID");
        module.functions.push_back(std::move(function));
    }
    for(auto n=r.Count();n;--n)module.strings.push_back(r.Text());
    for(auto n=r.Count();n;--n) {CsbImport import;import.function=r.Function();import.from=r.Signed();module.imports.push_back(std::move(import));}
    if(r.offset!=bytes.size())r.Fail("trailing data");
    return module;
}
} // namespace pal4::portable
