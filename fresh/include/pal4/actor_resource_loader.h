#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

struct ActorLoadRequest {
    std::uint32_t animation_context = 0;
    std::string_view animation_name;
    std::string_view actor_name;
    std::string_view texture_name;
    std::uint32_t animation_id = 0;
    std::string_view primary_text;
    std::string_view dialog_text;
    std::string_view secondary_text;
    bool alternate_resource = false;
};

class IActorResourceRuntime {
public:
    virtual ~IActorResourceRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::string_view TextAt(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool EqualIgnoringCase(
        std::string_view left, std::string_view right) = 0;
    [[nodiscard]] virtual bool CombatInactive() = 0;
    virtual void ResetActorProperties(std::uint32_t actor) = 0;
    virtual void StartResourceContext(bool alternate) = 0;
    virtual void LoadComponentClump(
        std::uint32_t actor, bool alternate,
        std::string_view actor_name) = 0;
    virtual void SetTexture(
        std::uint32_t actor, std::string_view texture_name) = 0;
    virtual void EndResourceContext() = 0;
    virtual void UpdateAnimation(
        std::uint32_t context, std::string_view name,
        std::uint32_t animation_id) = 0;
    [[nodiscard]] virtual bool StoreDialogText(
        std::uint32_t field, std::string_view value) = 0;
    virtual void StoreText(std::uint32_t field,
                           std::string_view value) = 0;
    virtual void UpdateRenderState(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupComponent(
        std::uint32_t actor) = 0;
};

class ActorResourceLoader final {
public:
    // IDA 0x529d00. String fields use C++ ownership behind the runtime.
    [[nodiscard]] static std::uint32_t Load(
        std::uint32_t actor, const ActorLoadRequest& request,
        IActorResourceRuntime& runtime);
};

class IActorPropertyResetRuntime {
public:
    virtual ~IActorPropertyResetRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual bool StoreEmptyText(std::uint32_t field) = 0;
    virtual void RemoveEffect(std::uint32_t handle) = 0;
    virtual void HandleEffectEvent(std::uint32_t handle,
                                   std::uint32_t context) = 0;
    virtual void RemoveFrameChild(std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint8_t CleanupComponent(
        std::uint32_t actor) = 0;
};

class ActorPropertyReset final {
public:
    // IDA 0x52a180.
    [[nodiscard]] static std::uint8_t Reset(
        std::uint32_t actor, IActorPropertyResetRuntime& runtime);
};

}  // namespace pal4
