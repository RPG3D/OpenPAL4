#include "pal4/xml_node_array.h"

namespace pal4 {

std::uint32_t XmlNodeArrayRuntimeStub::Allocate(std::uint32_t) { return 0u; }
std::uint32_t XmlNodeArrayRuntimeStub::CopyConstruct(
    std::uint32_t destination, std::uint32_t) { return destination; }
std::uint32_t XmlNodeArrayRuntimeStub::CopyAssign(
    std::uint32_t destination, std::uint32_t) { return destination; }
void XmlNodeArrayRuntimeStub::DestroyNode(std::uint32_t) {}
void XmlNodeArrayRuntimeStub::Free(std::uint32_t) {}

std::uint32_t XmlNodeArray::CopyConstructNode(
    std::uint32_t destination, std::uint32_t source,
    IXmlNodeArrayRuntime& runtime, std::uint32_t undefinedEax) {
    return destination ? runtime.CopyConstruct(destination, source) : undefinedEax;
}

std::uint32_t XmlNodeArray::InsertCopies(
    XmlNodeArrayState& state, std::uint32_t position,
    std::uint32_t count, std::uint32_t source,
    IXmlNodeArrayRuntime& runtime) {
    constexpr std::uint32_t stride = 60u;
    const auto oldBegin = state.words[1];
    const auto oldEnd = state.words[2];
    const auto tailCount = (oldEnd - position) / stride;

    if ((state.words[3] - oldEnd) / stride < count) {
        const auto oldCount = oldBegin ? (oldEnd - oldBegin) / stride : 0u;
        const auto newCapacity = oldCount + (oldCount > count ? oldCount : count);
        const auto allocatedCapacity =
            static_cast<std::int32_t>(newCapacity) < 0 ? 0u : newCapacity;
        const auto newBegin = runtime.Allocate(stride * allocatedCapacity);
        auto destination = newBegin;
        try {
            for (auto node = oldBegin; node != position; node += stride, destination += stride)
                (void)CopyConstructNode(destination, node, runtime);
            for (auto remaining = count; remaining != 0u; --remaining, destination += stride)
                (void)CopyConstructNode(destination, source, runtime);
            for (auto node = position; node != oldEnd; node += stride, destination += stride)
                (void)CopyConstructNode(destination, node, runtime);
        } catch (...) {
            for (auto node = newBegin; node != destination; node += stride)
                runtime.DestroyNode(node);
            runtime.Free(newBegin);
            throw;
        }
        for (auto node = oldBegin; node != oldEnd; node += stride)
            runtime.DestroyNode(node);
        runtime.Free(oldBegin);
        state.words[3] = newBegin + stride * newCapacity;
        state.words[1] = newBegin;
        state.words[2] = newBegin + stride * (oldCount + count);
        return 3u * (oldCount + count);
    }

    if (tailCount < count) {
        const auto insertedBytes = stride * count;
        for (auto node = position; node != oldEnd; node += stride)
            (void)CopyConstructNode(node + insertedBytes, node, runtime);
        for (auto node = oldEnd; node != oldEnd + stride * (count - tailCount); node += stride)
            (void)CopyConstructNode(node, source, runtime);
        for (auto node = position; node != oldEnd; node += stride)
            (void)runtime.CopyAssign(node, source);
        state.words[2] = oldEnd + insertedBytes;
        return state.words[2];
    }

    if (count == 0u)
        return 0u;
    const auto insertedBytes = stride * count;
    for (auto node = oldEnd - insertedBytes; node != oldEnd; node += stride)
        (void)CopyConstructNode(node + insertedBytes, node, runtime);
    for (auto node = oldEnd - insertedBytes; node != position; ) {
        node -= stride;
        (void)runtime.CopyAssign(node + insertedBytes, node);
    }
    std::uint32_t result = 0u;
    for (auto node = position; node != position + insertedBytes; node += stride)
        result = runtime.CopyAssign(node, source);
    state.words[2] = oldEnd + insertedBytes;
    return result;
}

std::uint32_t XmlNodeArray::Cleanup(
    XmlNodeArrayState& state, IXmlNodeArrayRuntime& runtime) {
    const auto begin = state.words[1];
    const auto end = state.words[2];
    for (auto node = begin; node != end; node += 60u)
        runtime.DestroyNode(node);
    runtime.Free(begin);
    state.words[1] = 0u;
    state.words[2] = 0u;
    state.words[3] = 0u;
    return 0u;
}

}  // namespace pal4
