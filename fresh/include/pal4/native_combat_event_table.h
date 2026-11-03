#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>

namespace pal4 {
// Owns CombatSystem+400's564-byte object (IDA57BF00), not the global script
// event bus.67 binding pairs precede a host-owned text backend at536. The
// backend handle is native, never a VC6 string/container image.
// Borrowed member slot must outlive this owner; Close clears its publication.
class NativeCombatEventTable final {
public:
    explicit NativeCombatEventTable(std::uint32_t& member_slot);
    ~NativeCombatEventTable();
    NativeCombatEventTable(const NativeCombatEventTable&)=delete;
    NativeCombatEventTable& operator=(const NativeCombatEventTable&)=delete;
    std::uint32_t Address() const;
    std::size_t BindingCount() const;
    std::string_view Text() const;
    void SetText(std::string_view);
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
