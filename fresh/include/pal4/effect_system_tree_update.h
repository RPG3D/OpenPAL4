#pragma once

#include "pal4/effect_system_cleanup.h"

#include <cstdint>

namespace pal4 {

class IEffectSystemTreeUpdateRuntime : public IEffectSystemPruneRuntime {
public:
    ~IEffectSystemTreeUpdateRuntime() override = default;
    virtual void UpdateXmlObject(std::uint32_t object,
                                 std::uint32_t first,
                                 std::uint32_t second) = 0;
    virtual void UpdateInlineObject(std::uint32_t object,
                                    std::uint32_t first,
                                    std::uint32_t second) = 0;
};

}  // namespace pal4
