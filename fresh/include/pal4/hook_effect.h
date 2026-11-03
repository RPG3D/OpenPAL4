#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

class IHookEffectFactoryRuntime {
public:
    virtual ~IHookEffectFactoryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ClumpUserData(
        std::uint32_t clump, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual bool Initialize(std::uint32_t effect,
                                          std::uint32_t config,
                                          std::uint32_t user_data,
                                          std::uint32_t context) = 0;
    virtual void Destroy(std::uint32_t effect, bool free_memory) = 0;
};

class IHookEffectTimeRuntime {
public:
    virtual ~IHookEffectTimeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::array<float, 16u> FrameTransform(
        std::uint32_t) { return {}; }
    virtual void RemoveFrameChild(std::uint32_t) {}
    virtual void SetFrameIdentity(std::uint32_t) {}
    virtual void ApplyFrameTransform(
        std::uint32_t, const std::array<float, 16u>&,
        std::uint32_t) {}
    virtual void UpdateFrameObjects(std::uint32_t) {}
};

class IHookEffectConfigRuntime {
public:
    virtual ~IHookEffectConfigRuntime() = default;
    virtual void Reset(std::uint32_t effect) = 0;
    [[nodiscard]] virtual std::int32_t ReadInt(
        std::uint32_t config, std::string_view key,
        std::int32_t default_value) = 0;
    [[nodiscard]] virtual float ReadFloat(
        std::uint32_t config, std::string_view key,
        float default_value) = 0;
    [[nodiscard]] virtual std::string ReadText(
        std::uint32_t config, std::string_view key) = 0;
    [[nodiscard]] virtual std::uint32_t CreateParticleObject(
        std::uint32_t size) = 0;
    virtual void DestroyParticleObject(std::uint32_t object) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void UpdateParticleFrame(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t FindTexture(
        std::string_view path) = 0;
    [[nodiscard]] virtual std::uint32_t LoadTexture(
        std::string_view path) = 0;
    virtual void DestroyTexture(std::uint32_t texture) = 0;
    virtual void SetProperties(std::uint32_t object,
                               bool alpha_blend_enabled) = 0;
    [[nodiscard]] virtual bool SetupObject(
        std::uint32_t object, bool not_point,
        std::uint32_t texture, std::uint32_t frame,
        bool axis_xz) = 0;
    virtual void SetCollectionState(std::uint32_t object,
                                    bool face_point) = 0;
    virtual void Advance(std::uint32_t object, double seconds,
                         std::uint32_t owner_frame) = 0;
};

class IHookEffectLifecycleRuntime {
public:
    virtual ~IHookEffectLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void ResetParticleFields(std::uint32_t object) = 0;
    virtual void DestroyParticleObject(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t UpdateParticle(
        std::uint32_t object, double seconds,
        std::uint32_t owner_frame, std::uint8_t active) = 0;
    [[nodiscard]] virtual std::uint32_t HandleParticleStateChange(
        std::uint32_t object, std::uint32_t state) = 0;
};

class HookEffect final {
public:
    static constexpr std::uint32_t kObjectSize = 16u;
    static constexpr std::uint32_t kVtable = 0x84b1f0u;

    // IDA 0x63aff0: construct and initialize a hook_effect clump effect.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t config, std::uint32_t clump,
        std::uint32_t context, IHookEffectFactoryRuntime& runtime);
    // IDA 0x63b060: restore its frame and return the sum of two time fields.
    [[nodiscard]] static double GetTotalTime(
        std::uint32_t effect, IHookEffectTimeRuntime& runtime);
    // IDA 0x63b0d0: load the hook's particle configuration and resources.
    [[nodiscard]] static bool InitializeFromConfig(
        std::uint32_t effect, std::uint32_t config,
        std::uint32_t frame, std::uint32_t owner,
        IHookEffectConfigRuntime& runtime);
    // IDA 0x63c5c0: reset and destroy the owned particle object.
    static void CleanupObject(
        std::uint32_t effect, IHookEffectLifecycleRuntime& runtime);
    // IDA 0x63c600/0x63c630: forward state changes to a live particle.
    [[nodiscard]] static std::uint32_t UpdateState(
        std::uint32_t effect, double seconds,
        IHookEffectLifecycleRuntime& runtime);
    [[nodiscard]] static std::optional<std::uint32_t> HandleStateChange(
        std::uint32_t effect, std::uint32_t state,
        IHookEffectLifecycleRuntime& runtime);
    // IDA 0x63c660/0x63c6d0: derived and base destructor stages.
    static void Destruct(
        std::uint32_t effect, IHookEffectLifecycleRuntime& runtime);
    static void DestructBase(
        std::uint32_t effect, IHookEffectLifecycleRuntime& runtime);
};

}  // namespace pal4
