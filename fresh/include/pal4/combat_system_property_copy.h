#pragma once

#include "pal4/combat_system_text_properties.h"

#include <cstdint>

namespace pal4 {

class ICombatSystemPropertyCopyRuntime
    : public ICombatSystemTextPropertyRuntime {
public:
    virtual ~ICombatSystemPropertyCopyRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CombatSystemInstance() = 0;
    virtual void AssignFirstText(std::uint32_t destination,
                                 std::uint32_t source) = 0;
};

struct CombatConfigSnapshot;
class ICombatConfigurationCopyServices {
public:
    virtual ~ICombatConfigurationCopyServices()=default;
    virtual void CopyWord(std::uint32_t index,std::uint32_t value)=0;
    virtual void CopyText(std::uint32_t index)=0;
    virtual void CopyFlag(std::uint32_t index,std::uint8_t value)=0;
};
class CombatSystemPropertyCopy final {
public:
    static std::uint8_t CopyConfiguration(const CombatConfigSnapshot&,ICombatConfigurationCopyServices&);
    // IDA 0x579460.
    [[nodiscard]] static std::uint8_t CopyConfiguration(
        std::uint32_t source, ICombatSystemPropertyCopyRuntime& runtime);

    // IDA 0x5796b0 and 0x5796d0.
    [[nodiscard]] static std::uint8_t AssignFirstText(
        std::uint32_t source, ICombatSystemPropertyCopyRuntime& runtime);
    [[nodiscard]] static std::uint8_t AssignSecondText(
        std::uint32_t source, ICombatSystemPropertyCopyRuntime& runtime);
};

}  // namespace pal4
