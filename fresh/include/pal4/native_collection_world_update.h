#pragma once

#include "pal4/collection_object_operations.h"

#include <functional>

namespace pal4 {

class NativeWorldAsset;

// Borrows the collection and original world asset. Component owners supply
// their own frame adapter; an unexpected component is an explicit failure.
class NativeCollectionWorldUpdate final : public ICollectionUpdateRuntime {
public:
    using ComponentUpdate = std::function<void(std::uint32_t, double)>;
    NativeCollectionWorldUpdate(CollectionState& collection, NativeWorldAsset& world,
                                ComponentUpdate components = {});
    int UpdateRenderParameters(CollectionState& collection, float elapsed) override;
    void RotateComponent(std::uint32_t object, double elapsed) override;

private:
    CollectionState& collection_;
    NativeWorldAsset& world_;
    ComponentUpdate components_;
};

}  // namespace pal4
