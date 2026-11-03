#pragma once

#include "pal4/effect_system_render_state.h"

#include <cstdint>

namespace pal4 {

class IEffectSystemGroupDrawRuntime
    : public IEffectSystemRenderStateRuntime {
public:
    ~IEffectSystemGroupDrawRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t SharedNil(
        std::uint32_t tree) = 0;
    virtual void NoOpDraw(std::uint32_t context) = 0;
    virtual void DrawObject(std::uint32_t object,
                            std::uint32_t context) = 0;
    virtual void DrawXmlObject(std::uint32_t object,
                               std::uint32_t context) = 0;
};

}  // namespace pal4
