#include "pal4/thunder_visual_path.h"

namespace pal4 {
namespace {

void Cleanup(ThunderSplineBufferRange& range,
             IThunderSplineBufferRuntime& runtime) {
    runtime.Free(range.begin);
    range = {};
}

}  // namespace

void ThunderSplineBuffers::CleanupSecond(
    ThunderSplineBufferRange& range,
    IThunderSplineBufferRuntime& runtime) {
    Cleanup(range, runtime);
}

void ThunderSplineBuffers::CleanupFirst(
    ThunderSplineBufferRange& range,
    IThunderSplineBufferRuntime& runtime) {
    Cleanup(range, runtime);
}

}  // namespace pal4
