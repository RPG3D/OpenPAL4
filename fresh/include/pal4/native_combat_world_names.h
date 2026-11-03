#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <string_view>
namespace pal4 {
// Original WorldManager348/364 name records, owned by host std::string.
// Native pointer/length views are read-only and must not enter VC6 cleanup.
// Borrowed Combat storage outlives the owner; Close retains the binding lease
// until destruction, matching the other WorldManager owners.
class NativeCombatWorldNames final {
public:
    NativeCombatWorldNames(std::uint32_t combat,std::array<std::uint32_t,8>& storage);
    ~NativeCombatWorldNames();
    NativeCombatWorldNames(const NativeCombatWorldNames&)=delete;
    NativeCombatWorldNames& operator=(const NativeCombatWorldNames&)=delete;
    void Assign(std::string_view region,std::string_view scene);
    bool Matches(std::string_view region,std::string_view scene) const;
    std::string_view Region() const;
    std::string_view Scene() const;
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
