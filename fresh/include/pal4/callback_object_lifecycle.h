#pragma once

#include "pal4/callback_state_initializer.h"
#include "pal4/binary_tree_iterator.h"

#include <cstdint>

namespace pal4 {

class ICallbackObjectRuntime {
public:
    virtual ~ICallbackObjectRuntime() = default;

    [[nodiscard]] virtual BinaryTreeNode* ResolveNode(
        std::uint32_t address) = 0;
    virtual void HandleStatusCheck(BinaryTreeNode* node) = 0;
    virtual void FreeNode(BinaryTreeNode* node) = 0;
    [[nodiscard]] virtual int Free(std::uint32_t address) = 0;
};

class CallbackObjectLifecycle final {
public:
    [[nodiscard]] static int Destroy(CallbackState::Storage& object,
                                     ICallbackObjectRuntime& runtime);

    // IDA 0x401050 is a tail jump to the destructor at 0x401060.
    [[nodiscard]] static int DestroyThunk(CallbackState::Storage& object,
                                          ICallbackObjectRuntime& runtime);

    [[nodiscard]] static int CleanupCollection(
        CallbackState::Storage& object, ICallbackObjectRuntime& runtime);
};

}  // namespace pal4
