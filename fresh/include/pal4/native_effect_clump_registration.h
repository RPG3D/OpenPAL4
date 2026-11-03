#pragma once

#include <array>
#include <cstdint>
#include <exception>
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class IArenaSequenceRuntime;

// Borrows the auxiliary object, its two nil nodes and its tree allocator.
// Unregister clumps before destroying them, or before clearing auxiliary trees.
class NativeEffectClumpRegistration final {
public:
    NativeEffectClumpRegistration(NativeBaseObjectMemory&, std::uint32_t auxiliary,
        const std::array<std::uint32_t, 2>& nils, IArenaSequenceRuntime&);
    ~NativeEffectClumpRegistration();
    bool Add(std::uint32_t clump);
    void Remove(std::uint32_t clump);
    std::exception_ptr TakeRenderError();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}  // namespace pal4
