#pragma once
#include <cstdint>
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEffectFactoryRegistry;
class NativeEffectTextureDictionary;
class NativeEffectNamedDictionaryTree;
class NativeEffectObjectPool;
class NativeEffectDialogLoader;
class NativeEffectSupportObjects;

// Borrows owners constructed on the same 168-byte singleton image and allocator.
// Drain/destroy effect groups and external raw Dialogs before Cleanup or Enable.
// All borrowed services must outlive this controller; it owns no payloads.
// Call Cleanup explicitly before service teardown; destruction only drops references.
class NativeEffectSystemLifecycle final {
public:
    NativeEffectSystemLifecycle(NativeBaseObjectMemory&, std::uint32_t singleton,
        NativeEffectFactoryRegistry&, NativeEffectTextureDictionary&,
        NativeEffectNamedDictionaryTree&, NativeEffectObjectPool&,
        NativeEffectDialogLoader&, NativeEffectSupportObjects&);
    ~NativeEffectSystemLifecycle();
    NativeEffectSystemLifecycle(const NativeEffectSystemLifecycle&)=delete;
    NativeEffectSystemLifecycle& operator=(const NativeEffectSystemLifecycle&)=delete;
    bool Enable(bool set_radius);
    void Cleanup();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
