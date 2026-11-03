#pragma once
#include "pal4/collection_lifecycle.h"
#include <filesystem>
#include <memory>
#include <string_view>
struct RpWorld;
struct RpClump;
namespace pal4 {
class NativePkFileManagerRuntime;
class NativePalGameIVTextureStorage;
class NativeUiComponentResource;
// Borrows an otherwise empty semantic collection and world. Retire this owner
// before the world, package manager, game texture storage or SDK session.
class NativeUiCollectionObjects final {
public:
    NativeUiCollectionObjects(CollectionState&, RpWorld*, const std::filesystem::path&,
        NativePkFileManagerRuntime&, NativePalGameIVTextureStorage&, RpClump* background=nullptr);
    ~NativeUiCollectionObjects();
    NativeUiCollectionObjects(const NativeUiCollectionObjects&)=delete;
    NativeUiCollectionObjects& operator=(const NativeUiCollectionObjects&)=delete;
    NativeUiComponentResource& Create(std::string_view prefix, std::string_view model,
                                      std::uint8_t mode=1u);
    NativeUiComponentResource* Find(std::uint32_t id);
    void Remove(std::uint32_t id);
    void Advance(std::uint32_t object, double elapsed);
    // Isolated component previews only. Full UI scenes use RenderWorld/FlushQueued.
    void Render(std::uint32_t camera);
    // Original UI pre-render: world traversal queues transparent background and
    // components; effects draw next; FlushQueued completes that shared queue.
    void RenderWorld(std::uint32_t camera);
    void FlushQueued(std::uint32_t camera);
    void Clear();
    [[nodiscard]] std::size_t Size() const noexcept;
    [[nodiscard]] std::size_t OutstandingQueueAllocations() const noexcept;
    [[nodiscard]] std::size_t PendingAtomics() const noexcept;
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
