#pragma once
#include <array>
#include <cstdint>
#include <span>
namespace pal4 {
class NativeBaseObjectMemory;class NativeInventorySession;class NativePlayerSkillRuntime;
//43F5A0: isolated processed script snapshot; borrowed equipment records/skills
// and their memory owners must remain valid for the synchronous operation.
class NativePlayerSnapshot final {
public:
    using Words=std::array<std::uint32_t,0x150u/4u>;
    NativePlayerSnapshot(NativeBaseObjectMemory&,NativeInventorySession&,NativePlayerSkillRuntime&);
    Words Build(std::uint32_t player,std::span<const std::uint32_t> equipment);
private:
    NativeBaseObjectMemory& memory_;NativeInventorySession& inventory_;NativePlayerSkillRuntime& skills_;bool busy_{};
};
}
