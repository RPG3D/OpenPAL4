#pragma once
#include "pal4/native_dialog_task_runtime.h"
#include <cstddef>
#include <array>
#include <memory>

namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeScriptObjectRuntime;
class INativeEffectFactoryLookup;
class NativeEffectSupportObjects;

// Dispatches to the concrete effect's facets. Missing effects/facets must fail
// explicitly; implementations may not substitute successful empty callbacks.
class INativeEffectTaskStatus {
public:
    virtual ~INativeEffectTaskStatus()=default;
    virtual bool EffectsEnabled()=0;
    virtual bool EffectIsActive(std::uint32_t effect)=0;
};

class INativeEffectTaskFacets : public INativeEffectTaskStatus {
public:
    ~INativeEffectTaskFacets() override=default;
    virtual std::uint32_t ResolveFacet(std::uint32_t effect,std::string_view task)=0;
    virtual void Show(std::uint32_t facet)=0;
    virtual void Hide(std::uint32_t facet)=0;
    virtual void StopEmission(std::uint32_t facet)=0;
    virtual void StartEmission(std::uint32_t facet,float duration,std::uint8_t increase)=0;
    virtual void ReadScale(std::uint32_t facet,std::array<float,3>& scale)=0;
    virtual void SetScale(std::uint32_t facet,const std::array<float,3>& scale,
        std::uint32_t mode)=0;
    virtual void SetColorFacetEnabled(std::uint32_t facet,std::uint8_t enabled)=0;
    virtual void ReadColor(std::uint32_t facet,std::array<std::uint8_t,4>& color)=0;
    virtual void SetColor(std::uint32_t facet,const std::array<std::uint8_t,4>& color)=0;
    virtual void PlayEffect(std::uint32_t facet,std::uint8_t loop)=0;
};

// Concrete Showobj/Hideobj/Decress/Scale/Position/Rotate/Revolute/ChangeColor/Track and
// Play/Pause/Rewind/StopEffect/RumpShahdow/Message/StartCamera/StopCamera/Wobble tasks using the shared task creator and
// scheduler. Other registered task kinds fail explicitly until bound.
// Registered lifecycles retain the implementation after this facade is released.
// Memory, documents, task registry and facets are borrowed and must outlive tasks.
// A supplied factory lookup is also borrowed and must outlive registered tasks;
// isolated probes may omit it to use the static descriptor catalog.
class NativeEffectTaskFactory final : public INativeDialogTaskPayloads {
public:
    NativeEffectTaskFactory(NativeBaseObjectMemory&,NativeEventDocumentRuntime&,
        NativeScriptObjectRuntime&,INativeEffectTaskFacets&,
        INativeEffectFactoryLookup* registry=nullptr,
        NativeEffectSupportObjects* random=nullptr);
    ~NativeEffectTaskFactory();
    NativeEffectTaskFactory(const NativeEffectTaskFactory&)=delete;
    NativeEffectTaskFactory& operator=(const NativeEffectTaskFactory&)=delete;
    bool EffectsEnabled() override;
    bool EffectIsActive(std::uint32_t) override;
    std::uint32_t CreateTask(std::uint32_t xml) override;
    void BindDialogObject(std::uint32_t task,std::uint32_t object) override;
    void PrepareTask(std::uint32_t task) override;
    void StartTask(std::uint32_t task) override;
    [[nodiscard]] std::size_t LiveTasks() const;
private:
    class Runtime;
    std::shared_ptr<Runtime> runtime_;
};
}  // namespace pal4
