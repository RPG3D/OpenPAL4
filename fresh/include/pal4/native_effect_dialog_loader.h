#pragma once
#include "pal4/native_effect_object_pool.h"
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeScriptObjectRuntime;
class NativePkFileManagerRuntime;
class NativeDialogParticlePlayback;
class NativeDialogObject;

// Owns the singleton+68 document cache and actual Dialog instances. Borrowed
// services/storage outlive this loader; destroy referencing pools/handles first.
// Payload creation is deferred to the existing Dialog playback composition.
class NativeEffectDialogLoader final : public INativeEffectPoolObjects {
public:
    NativeEffectDialogLoader(NativeBaseObjectMemory&,NativeEventDocumentRuntime&,
        NativeScriptObjectRuntime&,NativePkFileManagerRuntime&,std::uint32_t singleton,
        NativeDialogParticlePlayback&);
    ~NativeEffectDialogLoader();
    NativeEffectDialogLoader(const NativeEffectDialogLoader&)=delete;
    NativeEffectDialogLoader& operator=(const NativeEffectDialogLoader&)=delete;
    [[nodiscard]] std::uint32_t Load(std::string_view name);
    // Embedded effects use the same Dialog owner without a file/cache entry.
    [[nodiscard]] std::uint32_t CreateDialog();
    [[nodiscard]] bool LoadDialog(std::uint32_t address,std::uint32_t xml);
    void PrepareForDelete(std::uint32_t address);
    [[nodiscard]] NativeDialogObject& Dialog(std::uint32_t address);
    [[nodiscard]] std::size_t LiveDialogs() const;
    [[nodiscard]] std::uint32_t CachedDocuments() const;
    void ClearCache();
    std::string ReadEffectName(std::uint32_t document) override;
    std::uint32_t LoadEffect(std::uint32_t manager,std::string_view name) override;
    void ClearDialogFacets(std::uint32_t effect) override;
    void DestroyEffect(std::uint32_t effect) override;
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
