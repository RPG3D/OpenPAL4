#pragma once

#include "pal4/combat_result_data_lifecycle.h"

#include <cstdint>

namespace pal4 {

class ICombatEndingWindowDestructorRuntime
    : public ICombatResultDataLifecycleRuntime {
public:
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void DeleteWindow(std::uint32_t object,
                              std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t DestructBase(
        std::uint32_t object) = 0;
};

class CombatEndingWindowDestructor final {
public:
    static constexpr std::uint32_t kVtable = 0x845370u;
    static constexpr std::uint32_t kEmbeddedVtable = 0x845358u;

    // IDA 0x543d10.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t window, ICombatEndingWindowDestructorRuntime& runtime);
    // IDA 0x543a40, vtable-referenced deleting destructor.
    [[nodiscard]] static std::uint32_t DeletingDestruct(
        std::uint32_t window, std::uint8_t flags,
        ICombatEndingWindowDestructorRuntime& runtime);
};

}  // namespace pal4
