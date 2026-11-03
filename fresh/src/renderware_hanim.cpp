#include "pal4/renderware_hanim.h"

namespace pal4 {

std::uint32_t RenderWareHAnimStub::GetHierarchy(
    std::uint32_t /*frame*/) {
    return 0u;
}

void RenderWareHAnimStub::ForAllChildren(
    std::uint32_t /*frame*/, const FrameVisitor& /*visitor*/) {}

std::uint32_t RenderWareHAnimStub::ReadDword(
    std::uint32_t /*address*/) {
    return 0u;
}

void RenderWareHAnimStub::SetSkinHierarchy(
    std::uint32_t /*atomic*/, std::uint32_t /*hierarchy*/) {}

std::uint32_t RenderWareHAnimStub::GetFrameId(
    std::uint32_t /*frame*/) {
    return 0u;
}

std::uint32_t HAnimFrameWalker::Visit(
    const std::uint32_t frame, std::uint32_t& found,
    IRenderWareHAnimRuntime& runtime) {
    const auto hierarchy = runtime.GetHierarchy(frame);
    if (hierarchy != 0u) {
        found = hierarchy;
        return 0u;
    }
    runtime.ForAllChildren(frame, [&](const std::uint32_t child) {
        return Visit(child, found, runtime);
    });
    return frame;
}

std::uint32_t HAnimFrameWalker::FindInAtomicChildren(
    const std::uint32_t atomic, IRenderWareHAnimRuntime& runtime) {
    if (atomic == 0u) return 0u;
    const auto frame = runtime.ReadDword(atomic + 4u);
    std::uint32_t found = 0u;
    runtime.ForAllChildren(frame, [&](const std::uint32_t child) {
        return Visit(child, found, runtime);
    });
    return found;
}

std::uint32_t HAnimFrameWalker::AssignSkinHierarchy(
    const std::uint32_t atomic, const std::uint32_t hierarchy,
    IRenderWareHAnimRuntime& runtime) {
    runtime.SetSkinHierarchy(atomic, hierarchy);
    return atomic;
}

std::uint32_t HAnimFrameWalker::CaptureFrameId(
    const std::uint32_t frame, std::uint32_t& id,
    IRenderWareHAnimRuntime& runtime) {
    id = runtime.GetFrameId(frame);
    return 0u;
}

}  // namespace pal4
