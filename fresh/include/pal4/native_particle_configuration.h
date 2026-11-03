#pragma once
#include "pal4/rtk_prt_std_atomic_configuration.h"
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeEffectTextureDictionary;
class INativeDialogParticleQueues;
// Owns the resource at State()+92. Services outlive this owner; destroy all
// particle owners before the shared effect dictionary and SDK shutdown.
class NativeParticleConfiguration final {
public:
    NativeParticleConfiguration(NativeBaseObjectMemory&,NativeEventDocumentRuntime&,
                                NativeEffectTextureDictionary&);
    ~NativeParticleConfiguration();
    NativeParticleConfiguration(const NativeParticleConfiguration&)=delete;
    NativeParticleConfiguration& operator=(const NativeParticleConfiguration&)=delete;
    [[nodiscard]] bool Load(std::uint32_t document);
    void Clear();
    // Caller schedules update and drawing separately, as in the original.
    int Update(double delta,std::uint32_t argument=0u,bool skip_emission=false);
    int Draw();
    // Original owner-level gates and 500-step pre-render. A running owner
    // requires a valid caller context at State()+16 (its argument is +48).
    std::uint8_t Advance(double delta);
    std::uint8_t PreRender();
    std::uint8_t ResetPlayback();
    // Original Stop resource/event phase; factory deletes the owner next.
    void ReleasePlayback(INativeDialogParticleQueues&);
    [[nodiscard]] RtkPrtStdAtomicObjectState& State() noexcept;
private:
    class Runtime;
    std::unique_ptr<Runtime> runtime_;
};
}
