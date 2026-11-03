#include "pal4/frame_rotation.h"

namespace pal4 {

int FrameRotation::Update(
    FrameRotationState& frame, const std::array<float, 3>& angles,
    std::array<float, 3>& output, IFrameRotationRuntime& runtime) {
    const std::array<float, 3> position{
        frame.words[16], frame.words[17], frame.words[18]};

    runtime.ClampAxis(frame.Axis(1));
    runtime.ClampAxis(frame.Axis(0));
    runtime.ClampAxis(frame.Axis(2));

    runtime.Translate(frame, {-position[0], -position[1], -position[2]}, 2);
    for (std::size_t i = 0; i < 3; ++i)
        runtime.Rotate(frame, frame.Axis(i), angles[i], 2);
    runtime.Translate(frame, position, 2);
    runtime.UpdateObjects(frame);
    return runtime.CalculateAngles(frame.Axis(0), output);
}

}  // namespace pal4
