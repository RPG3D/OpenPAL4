#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IActorInstanceMemoryRuntime {
public:
    virtual ~IActorInstanceMemoryRuntime()=default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};
class IActorInstanceFrameRuntime : public virtual IActorInstanceMemoryRuntime {
public:
    virtual void SetAnimationFlag(std::uint32_t state,
                                  std::uint32_t value) = 0;
    [[nodiscard]] virtual std::string_view GlobalAnimationName() = 0;
    virtual void SetAnimationName(std::uint32_t state,
                                  std::string_view name) = 0;
    virtual void InvokeActorSlot24(std::uint32_t actor,
                                   std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeActorSlot20(
        std::uint32_t actor) = 0;
};

class IActorInstanceRenderRuntime {
public:
    virtual ~IActorInstanceRenderRuntime()=default;
    virtual void RenderClump(std::uint32_t actor,std::uint32_t argument)=0;
    virtual void ProcessAnimation(std::uint32_t state,std::uint32_t argument)=0;
    virtual void RenderChild(std::uint32_t child,std::uint32_t argument)=0;
};
class IActorInstanceUpdateRuntime : public virtual IActorInstanceMemoryRuntime {
public:
    virtual void UpdateAndRenderClump(std::uint32_t actor,std::uint32_t elapsed_bits)=0;
    virtual void UpdateInterpolator(std::uint32_t state,std::uint32_t elapsed_bits)=0;
    virtual std::uint32_t UpdateBoundingBox(std::uint32_t box,std::uint32_t clump)=0;
};
// Frame reset, render and elapsed update expose only their required services.
class IActorInstanceAnimationRuntime : public IActorInstanceFrameRuntime,
                                      public IActorInstanceRenderRuntime,
                                      public IActorInstanceUpdateRuntime {
public:
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void BeginResourceContext() = 0;
    virtual void SetResourceName(std::uint32_t field,
                                 std::string_view name) = 0;
    [[nodiscard]] virtual std::string_view PathPrefixOne() = 0;
    [[nodiscard]] virtual std::string_view PathPrefixTwo() = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual bool LoadClump(
        std::uint32_t actor, std::string_view resource_name,
        std::string_view directory, std::string_view action_name,
        std::uint32_t game_context) = 0;
    virtual void LoadAnimationData(
        std::uint32_t state, std::uint32_t clump,
        std::string_view directory, std::string_view action_name) = 0;
    [[nodiscard]] virtual bool LoadShadowClump(
        std::uint32_t actor, std::uint32_t game_context) = 0;
    virtual void AttachHierarchy(std::uint32_t state,
                                 std::uint32_t hierarchy) = 0;
    virtual void InitializeBoundingBox(std::uint32_t box,
                                       std::uint32_t clump) = 0;
    virtual void HideHookAtomics(std::uint32_t clump) = 0;
    virtual void EndResourceContext() = 0;
};

class ActorInstanceAnimation final {
public:
    [[nodiscard]] static std::uint32_t GetFrame(
        std::uint32_t actor, IActorInstanceFrameRuntime& runtime);
    [[nodiscard]] static bool Play(
        std::uint32_t actor, std::string_view resource_name,
        std::string_view action_name,
        IActorInstanceAnimationRuntime& runtime);
    static void Render(std::uint32_t actor, std::uint32_t argument,
                       IActorInstanceRenderRuntime& runtime);
    [[nodiscard]] static std::uint32_t Update(
        std::uint32_t actor, std::uint32_t elapsed_bits,
        IActorInstanceUpdateRuntime& runtime);
};

}  // namespace pal4
