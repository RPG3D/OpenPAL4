#pragma once

#include "pal4/dialog_object.h"

namespace pal4 {
// Stateless SDK adapter for the shared Dialog setters and refresh algorithm.
// Borrows frame pointers stored in the Dialog; owns no frames or Dialogs.
class NativeDialogTransformRuntime final : public IDialogObjectTransformRuntime,
    public IDialogObjectFrameRuntime {
public:
    int RefreshTransform(DialogObjectState&) override;
    std::uint32_t FrameParent(std::uint32_t frame) override;
    int RemoveFrameChild(std::uint32_t frame) override;
    int AddFrameChild(std::uint32_t parent, std::uint32_t frame) override;
    void SetIdentity(std::uint32_t frame) override;
    void Scale(std::uint32_t frame, const std::array<float,3>&, std::uint32_t combine) override;
    void Rotate(std::uint32_t frame, const std::array<float,3>& axis,
        float angle, std::uint32_t combine) override;
    void Translate(std::uint32_t frame, const std::array<float,3>&, std::uint32_t combine) override;
    int UpdateObjects(std::uint32_t frame) override;
};
}  // namespace pal4
