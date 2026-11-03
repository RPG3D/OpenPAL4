#include "pal4/dynamic_strip_object_render.h"

namespace pal4 {
namespace {

void Cleanup(DynamicStripBufferRange& range,
             IDynamicStripBufferRuntime& runtime) {
    runtime.Free(range.begin);
    range = {};
}

}  // namespace

void DynamicStripVisualBuffers::CleanupIndices(
    DynamicStripBufferRange& range, IDynamicStripBufferRuntime& runtime) {
    Cleanup(range, runtime);
}

void DynamicStripVisualBuffers::CleanupVertices(
    DynamicStripBufferRange& range, IDynamicStripBufferRuntime& runtime) {
    Cleanup(range, runtime);
}

}  // namespace pal4
