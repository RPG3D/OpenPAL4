#pragma once
#include <cstdint>
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEffectTreeStorage;
class NativeEffectOwnerRegistry;
class INativeEffectGroupLifecycleServices {
public:
    virtual ~INativeEffectGroupLifecycleServices()=default;
    virtual void CleanupEffects(std::uint32_t group)=0;
    virtual void InitializeGeometryData(std::uint32_t geometry)=0;
};

// Owns the five trees and twelve lists in a zero-initialized 260-byte group
// image, reusing original Initialize/Enable/Disable/Destroy. All borrowed
// storage/services must outlive this owner. Destroy before deleting Dialog
// services. Destroy invalidates borrowed queue views; stop using them first.
class NativeEffectGroupLifecycle final {
public:
    NativeEffectGroupLifecycle(NativeBaseObjectMemory&,std::uint32_t group,
        NativeEffectTreeStorage&,std::uint32_t system,NativeEffectOwnerRegistry&,
        INativeEffectGroupLifecycleServices&);
    ~NativeEffectGroupLifecycle();
    NativeEffectGroupLifecycle(const NativeEffectGroupLifecycle&)=delete;
    NativeEffectGroupLifecycle& operator=(const NativeEffectGroupLifecycle&)=delete;
    bool Enable(std::uint32_t effect);
    void Disable();
    void Destroy();
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
