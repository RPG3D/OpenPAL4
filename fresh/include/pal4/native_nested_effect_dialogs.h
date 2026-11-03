#pragma once
#include <cstdint>

namespace pal4 {
class NativeEffectDialogLoader;
class NativeEffectActiveFrame;
class NativeDialogStopRuntime;

// Borrows the same owners used by the outer Dialog. Embedded Dialogs enter
// neither the named pool nor the active tree: their EffectObject drives them.
class NativeNestedEffectDialogs final {
public:
    NativeNestedEffectDialogs(NativeEffectDialogLoader&,NativeEffectActiveFrame&,NativeDialogStopRuntime&);
    std::uint32_t Create();
    bool Load(std::uint32_t dialog,std::uint32_t xml);
    bool Start(std::uint32_t dialog);
    bool Activate(std::uint32_t dialog);
    void Update(std::uint32_t dialog,float seconds);
    void Reset(std::uint32_t dialog);
    double Measure(std::uint32_t dialog);
    void PrepareForDelete(std::uint32_t dialog);
    void Destroy(std::uint32_t dialog);
private:
    NativeEffectDialogLoader& loader_;
    NativeEffectActiveFrame& active_;
    NativeDialogStopRuntime& stop_;
};
} // namespace pal4
