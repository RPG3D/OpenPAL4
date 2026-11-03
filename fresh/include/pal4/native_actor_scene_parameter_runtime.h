#pragma once

#include "pal4/base_object_scene_parameter.h"

namespace pal4 {

class NativeActorSceneParameterRuntime final
    : public IBaseObjectSceneParameterRuntime {
public:
    [[nodiscard]] std::array<float, 3> ClumpPosition(
        std::uint32_t clump) override;
};

}  // namespace pal4
