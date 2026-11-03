#pragma once
#include "pal4/effect_system_cleanup.h"
#include <array>
#include <memory>
#include <string_view>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativePkFileManagerRuntime;
class NativeEffectDialogLoader;
class NativeEffectObjectPool;
class NativeEffectTreeStorage;
class NativeEffectActiveFrame;
class INativeEffectCreationEvents {
public:
    virtual ~INativeEffectCreationEvents()=default;
    virtual void Created(std::uint32_t group,const EffectSystemHandle&,
        const std::array<float,3>& position,std::uint32_t repeat_count)=0;
};

// Original630990/630E10 over actual native packages, pool, XML, Dialog playback
// and active tree. Borrows all services and initialized group/singleton images.
class NativeEffectCreation final {
public:
    NativeEffectCreation(NativeBaseObjectMemory&,NativeEventDocumentRuntime&,
        NativePkFileManagerRuntime&,std::uint32_t singleton,std::uint32_t group,
        NativeEffectDialogLoader&,NativeEffectObjectPool&,NativeEffectTreeStorage&,
        NativeEffectActiveFrame&,INativeEffectCreationEvents&);
    ~NativeEffectCreation();
    [[nodiscard]] std::uint32_t Group() const noexcept;
    [[nodiscard]] std::string_view LastFailure() const noexcept;
    [[nodiscard]] EffectSystemHandle Create(std::string_view name,
        const std::array<float,3>& position,std::int32_t repeats=1,std::uint8_t flag=0u);
    // Original second entry prepares playback but leaves task enabling to caller.
    [[nodiscard]] EffectSystemHandle CreateWithoutPause(std::string_view name,
        const std::array<float,3>& position,std::int32_t repeats=1,std::uint8_t flag=0u);
private:
    class Services;
    std::unique_ptr<Services> services_;
};
}
