#include "pal4/base_object_scene_parameter.h"

#include <bit>

namespace pal4 {

std::array<float, 3> BaseObjectSceneParameter::Refresh(
    BaseObjectState& actor, IBaseObjectSceneParameterRuntime& runtime) {
    auto position = runtime.ClumpPosition(actor.Read(0x14u));
    const auto height = std::bit_cast<float>(actor.Read(0xecu));
    // IDA443FB1/443FBD/443FCE run with the game's x87 PC24 control word.
    // Keep each arithmetic result in float; a double expression rounds only
    // at the final store and shifts the M05 camera target by one ULP.
    const float doubled_height = height + height;
    const float offset = doubled_height * 0.9f;
    position[1] = position[1] + offset;
    for (std::size_t axis = 0; axis < position.size(); ++axis)
        actor.Write(0x1f4u + static_cast<std::uint32_t>(axis * 4u),
                    std::bit_cast<std::uint32_t>(position[axis]));
    return position;
}

}  // namespace pal4
