#pragma once
#include "pal4/dialog_object.h"
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeEffectTextManager;

// Borrows an initialized Dialog target tree, its nil and the XML template owner.
// Native list buffers/clones belong to the Dialog and its normal cleanup path.
class NativeDialogConfigRuntime final {
public:
    NativeDialogConfigRuntime(NativeBaseObjectMemory&, NativeEventDocumentRuntime&,
        NativeEffectTextManager&, std::uint32_t target_nil);
    ~NativeDialogConfigRuntime();
    NativeDialogConfigRuntime(const NativeDialogConfigRuntime&)=delete;
    NativeDialogConfigRuntime& operator=(const NativeDialogConfigRuntime&)=delete;
    [[nodiscard]] bool Load(DialogObjectState&, std::uint32_t root);
    // Only for an idle Dialog with no task container or acquired effects.
    // Retains vector capacities and target head/nil for reuse.
    void Clear(DialogObjectState&);
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}  // namespace pal4
