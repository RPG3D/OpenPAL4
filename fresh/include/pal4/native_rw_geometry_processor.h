#pragma once

#include <cstdint>
#include <exception>

struct RpClump;

namespace pal4 {

class IRwGeometryQueueAllocator {
public:
    virtual ~IRwGeometryQueueAllocator() = default;
    [[nodiscard]] virtual std::uint32_t AllocateQueueBytes(
        std::uint32_t bytes) = 0;
    virtual void FreeQueueBytes(std::uint32_t address) = 0;
};

// Native traversal for the game-owned RenderWare plugin 8449.
class NativeRwGeometryProcessor final {
public:
    static void BindQueueAllocator(IRwGeometryQueueAllocator* allocator);
    static void UnbindQueueAllocator(IRwGeometryQueueAllocator* allocator);
    // Explicit queues (e.g. persistent effect groups) use their own lifetime.
    static void BindQueueAllocator(std::uint32_t queue, IRwGeometryQueueAllocator* allocator);
    static void UnbindQueueAllocator(std::uint32_t queue, IRwGeometryQueueAllocator* allocator);
    [[nodiscard]] static std::uint32_t ProcessClump(RpClump* clump,
                                                    std::uint32_t queue);
    [[nodiscard]] static std::int32_t RenderQueued(std::uint32_t queue);
    // IDA660270: retain allocated capacity and reset the queue's end to begin.
    [[nodiscard]] static std::int32_t ClearQueued(std::uint32_t queue);
    static void RestoreClump(RpClump* clump);
    [[nodiscard]] static std::uint32_t RenderCallbackAddress();
    [[nodiscard]] static std::exception_ptr TakeRenderError();
};

}  // namespace pal4
