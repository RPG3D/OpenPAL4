#pragma once

#include "pal4/strip_visual_block_view.h"
#include <cstdint>
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;

// Borrows block storage, allocator and emitter clump. Construct once, then
// Destroy before freeing storage or stopping the SDK. Hook frames are borrowed.
class NativeStripVisualRuntime final {
public:
    explicit NativeStripVisualRuntime(NativeBaseObjectMemory& memory);
    ~NativeStripVisualRuntime();
    NativeStripVisualRuntime(const NativeStripVisualRuntime&) = delete;
    NativeStripVisualRuntime& operator=(const NativeStripVisualRuntime&) = delete;
    void Construct(StripVisualBlockView block, std::uint8_t stack_byte = 0u);
    void Destroy(StripVisualBlockView block);
    bool Setup(StripVisualBlockView block, std::uint32_t texture,
               std::uint32_t clump, float lifetime, bool alternate_hooks);
    bool SetupWidth(StripVisualBlockView, std::uint32_t texture, float width, float lifetime);
    void UpdateFrame(StripVisualBlockView);
    void ClearSegments(StripVisualBlockView);
    std::uint32_t Cleanup(StripVisualBlockView block);
    std::uint8_t Update(StripVisualBlockView block, double seconds, bool drain);
    void Draw(StripVisualBlockConstView block);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}  // namespace pal4
