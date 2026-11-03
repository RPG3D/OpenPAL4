#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;

// Payload operations belong to the Dialog/effect owner. These are required even
// when a caller currently expects every acquisition to come from the cache.
class INativeEffectPoolObjects {
public:
    virtual ~INativeEffectPoolObjects() = default;
    virtual std::string ReadEffectName(std::uint32_t document) = 0;
    virtual std::uint32_t LoadEffect(std::uint32_t manager, std::string_view name) = 0;
    virtual void ClearDialogFacets(std::uint32_t effect) = 0;
    virtual void DestroyEffect(std::uint32_t effect) = 0;
};

// Owns the three pool trees at singleton +84 and their name records. Borrows the
// singleton image, allocator and payload services; all must outlive this object.
// Construct once on uninitialized pool storage. Payload destruction is delegated
// to the required owner service; frame detachment uses the public SDK.
class NativeEffectObjectPool final {
public:
    NativeEffectObjectPool(NativeBaseObjectMemory&, std::uint32_t singleton,
        INativeEffectPoolObjects&, NativeEventDocumentRuntime& documents);
    ~NativeEffectObjectPool();
    NativeEffectObjectPool(const NativeEffectObjectPool&) = delete;
    NativeEffectObjectPool& operator=(const NativeEffectObjectPool&) = delete;
    void Enable(std::uint32_t capacity);
    void Deinitialize();
    void CleanupAllObjects();
    [[nodiscard]] std::uint32_t Acquire(std::string_view name);
    void Return(std::uint32_t effect, std::uint8_t mode);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
    std::uint32_t pool_;
};
}  // namespace pal4
