#pragma once
#include <cstdint>

namespace pal4 {
// Cleanup facets keep original algorithms usable without binding loaders/renderers.
class IGobBaseCleanupRuntime {
public:
    virtual ~IGobBaseCleanupRuntime() = default;
    virtual std::uint32_t ReadWord(std::uint32_t) = 0;
    virtual void WriteWord(std::uint32_t,std::uint32_t) = 0;
    virtual bool EffectNeedsUpdate(std::uint32_t) = 0;
    virtual void HandleEffectEvent(std::uint32_t,std::uint32_t,std::uint32_t) = 0;
    virtual bool EffectReadyToShow(std::uint32_t) = 0;
    virtual std::uint32_t RemoveEffect(std::uint32_t) = 0;
    virtual std::uint32_t CleanupClump(std::uint32_t) = 0;
    virtual void DestructXmlDocument(std::uint32_t) = 0;
    virtual std::uint32_t FreeMemory(std::uint32_t) = 0;
};
class IGobInheritedCleanupRuntime {
public:
    virtual ~IGobInheritedCleanupRuntime() = default;
    virtual std::uint32_t CleanupBase(std::uint32_t,std::uint32_t) = 0;
};
class IGobActionCleanupRuntime : public IGobInheritedCleanupRuntime {
public:
    virtual void CleanupAnimation(std::uint32_t) = 0;
};
class IGobEffectCleanupRuntime : public IGobInheritedCleanupRuntime {
public:
    virtual bool EffectNeedsUpdate(std::uint32_t) = 0;
    virtual void HandleEffectEvent(std::uint32_t,std::uint32_t,std::uint32_t) = 0;
};
class IGobMachineCleanupRuntime {
public:
    virtual ~IGobMachineCleanupRuntime() = default;
    virtual std::uint32_t ReadWord(std::uint32_t) = 0;
    virtual void WriteWord(std::uint32_t,std::uint32_t) = 0;
    virtual void CleanupAnimationData(std::uint32_t) = 0;
    virtual void CleanupBaseResources(std::uint32_t) = 0;
    virtual void ClearTaskContainer(std::uint32_t) = 0;
};
class IGobSoundCleanupRuntime {
public:
    virtual ~IGobSoundCleanupRuntime() = default;
    virtual void WriteWord(std::uint32_t,std::uint32_t) = 0;
    virtual void CleanupBaseResources(std::uint32_t) = 0;
    virtual std::uint32_t StopAndRemoveAudioHandle(std::uint32_t) = 0;
};
} // namespace pal4
