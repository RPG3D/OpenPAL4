#pragma once

#include <cstdint>

namespace pal4 {

class IActorInstanceRuntime {
public:
    virtual ~IActorInstanceRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    // Release through the text owner. Native std::string views have no
    // reference-count byte before their data pointer.
    virtual void ReleaseText(std::uint32_t record) = 0;
    virtual void InitializeResourceManager(std::uint32_t actor) = 0;
    virtual void InitializeObjectState(std::uint32_t state) = 0;
    virtual void InitializeTga(std::uint32_t header) = 0;
    virtual void InitializeComponent(std::uint32_t component) = 0;
    virtual void InitializeNameNode(std::uint32_t node) = 0;
    virtual void SetTextFlag(std::uint32_t record,
                             std::uint8_t flag) = 0;
    virtual void ResetEffect(std::uint32_t handle) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyComponent(
        std::uint32_t component) = 0;
    virtual void ResetActorProperties(std::uint32_t actor) = 0;
    virtual void CleanupObjectData(std::uint32_t state) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint32_t CleanupClump(
        std::uint32_t actor, std::uint32_t game_context) = 0;
    virtual void CleanupObjectState(std::uint32_t state) = 0;
    virtual void DestroyResourceManager(std::uint32_t actor) = 0;
};

class ActorInstanceLifecycle final {
public:
    static constexpr std::uint32_t kMinimumSize = 1548u;
    // IDA 0x52a540.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t actor, IActorInstanceRuntime& runtime);
    // IDA 0x52a6f0 (empty-string thunk, not a destructor).
    [[nodiscard]] static const char* EmptyErrorString();
    // IDA 0x52a720.
    [[nodiscard]] static std::uint32_t CleanupComponentText(
        std::uint32_t component, IActorInstanceRuntime& runtime);
    // IDA 0x52a840.
    static void Destroy(std::uint32_t actor, IActorInstanceRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetFrameFlags(
        std::uint32_t actor, std::uint32_t value,
        IActorInstanceRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetFrame(
        std::uint32_t actor, IActorInstanceRuntime& runtime);
};

}  // namespace pal4
