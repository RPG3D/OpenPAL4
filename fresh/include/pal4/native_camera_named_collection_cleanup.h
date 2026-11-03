#pragma once

#include "pal4/callback_object_lifecycle.h"

namespace pal4 {
class NativeBaseObjectMemory;

// The creating camera owner supplies its complete destructor, including tasks,
// CameraData, name and outer object. It must use its original allocator.
class INativeNamedCameraObjects {
public:
    virtual ~INativeNamedCameraObjects() = default;
    virtual void DestroyNamedCamera(std::uint32_t object) = 0;
};

// Original callback-tree nodes and copied keys belong to memory; camera payloads
// belong to objects. Both dependencies and collection storage must outlive use.
class NativeCameraNamedCollectionCleanup final : public ICallbackObjectRuntime {
public:
    NativeCameraNamedCollectionCleanup(NativeBaseObjectMemory& memory,
        std::uint32_t collection, INativeNamedCameraObjects& objects);
    BinaryTreeNode* ResolveNode(std::uint32_t address) override;
    void HandleStatusCheck(BinaryTreeNode* node) override;
    void FreeNode(BinaryTreeNode* node) override;
    int Free(std::uint32_t address) override;
private:
    NativeBaseObjectMemory& memory_;
    std::uint32_t collection_;
    INativeNamedCameraObjects& objects_;
};
}
