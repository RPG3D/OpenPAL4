#pragma once
#include "pal4/dialog_object.h"
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeEffectTextManager;
class NativeDialogProperties;

// Owns a constructed Dialog image, template manager, embedded XML, two existing
// trees and modern named facet storage. Properties borrow vector interfaces.
// All simultaneous owners must use the same memory service (shared tree nils).
// Stop playback/release effects and frame before Clear or destruction. This
// owner does not replace the existing playback/reset/pre-delete services.
class NativeDialogObject final {
public:
    NativeDialogObject(NativeBaseObjectMemory&,NativeEventDocumentRuntime&);
    ~NativeDialogObject();
    NativeDialogObject(const NativeDialogObject&)=delete;
    NativeDialogObject& operator=(const NativeDialogObject&)=delete;
    [[nodiscard]] DialogObjectState& State();
    [[nodiscard]] std::uint32_t Address() const;
    [[nodiscard]] std::uint32_t FacetNil() const;
    [[nodiscard]] std::uint32_t TargetNil() const;
    [[nodiscard]] NativeEffectTextManager& Templates();
    [[nodiscard]] NativeDialogProperties& Properties();
    void ClearFacets();
    [[nodiscard]] bool Load(std::uint32_t root);
    void Clear();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
} // namespace pal4
