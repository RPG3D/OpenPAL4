#pragma once

#include "pal4/effect_render_parameters.h"
#include "pal4/preterit_effect.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace pal4 {

struct PalScreenEffectState {
    EffectRenderParameters render{};
    PreteritEffectState preterit{};
};
static_assert(sizeof(PalScreenEffectState) == 0x98u);
static_assert(offsetof(PalScreenEffectState, preterit) == 0x3cu);

struct PalScreenEffectAccessState {
    std::uint8_t guard{};
    std::uint32_t instance{};
};

class IPalScreenEffectRuntime {
public:
    virtual ~IPalScreenEffectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetSingletonManager() = 0;
    [[nodiscard]] virtual std::uint32_t Register(
        std::uint32_t manager, std::string_view name,
        std::uint32_t id, std::uint32_t factory,
        std::uint32_t destroy) = 0;
    [[nodiscard]] virtual PalScreenEffectState* Allocate(
        std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t Free(
        PalScreenEffectState* state) = 0;
    [[nodiscard]] virtual std::int32_t ScreenWidth() = 0;
    [[nodiscard]] virtual std::int32_t ScreenHeight() = 0;
    virtual void EmptyDestructor(const void* subobject) = 0;
    virtual void Log(std::string_view format, std::string_view source_file,
                     std::uint32_t source_line) = 0;
};

class PalScreenEffectRuntimeStub final : public IPalScreenEffectRuntime {
public:
    [[nodiscard]] std::uint32_t GetSingletonManager() override;
    [[nodiscard]] std::uint32_t Register(
        std::uint32_t manager, std::string_view name,
        std::uint32_t id, std::uint32_t factory,
        std::uint32_t destroy) override;
    [[nodiscard]] PalScreenEffectState* Allocate(
        std::uint32_t bytes) override;
    [[nodiscard]] std::uint32_t Free(
        PalScreenEffectState* state) override;
    [[nodiscard]] std::int32_t ScreenWidth() override;
    [[nodiscard]] std::int32_t ScreenHeight() override;
    void EmptyDestructor(const void* subobject) override;
    void Log(std::string_view format, std::string_view source_file,
             std::uint32_t source_line) override;
};

class PalScreenEffect final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e4140u;
    static constexpr std::uint32_t kInstanceAddress = 0x8e413cu;
    static constexpr std::uint32_t kFactoryAddress = 0x41f770u;
    static constexpr std::uint32_t kDestroyAddress = 0x41f7d0u;

    [[nodiscard]] static std::uint32_t GetSingleton(
        PalScreenEffectAccessState& access,
        IPalScreenEffectRuntime& runtime);
    [[nodiscard]] static PalScreenEffectState* Create(
        IPalScreenEffectRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        PalScreenEffectState* state, std::uint32_t incoming_eax,
        IPalScreenEffectRuntime& runtime);
    [[nodiscard]] static PalScreenEffectState& Construct(
        PalScreenEffectState& state, IPalScreenEffectRuntime& runtime);
    static void Destruct(PalScreenEffectState& state,
                         IPalScreenEffectRuntime& runtime);
    static void ReleaseResources(PalScreenEffectState& state,
                                 IEffectRenderRuntime& runtime);
    [[nodiscard]] static std::uint32_t ResetParameters(
        PalScreenEffectState& state);
    static void InitializeResources(
        PalScreenEffectState& state,
        IPalScreenEffectRuntime& runtime,
        IEffectRenderRuntime& render_runtime,
        IPreteritRuntime& preterit_runtime);
    [[nodiscard]] static std::uint32_t StartFade(
        PalScreenEffectState& state, float duration,
        std::uint32_t keep_active,
        std::uint8_t red, std::uint8_t green, std::uint8_t blue,
        float alpha_start, float alpha_end,
        float alpha_on_finish);
    [[nodiscard]] static std::uint32_t Draw(
        const PalScreenEffectState& state, std::uint32_t context,
        IEffectRenderRuntime& runtime);
    [[nodiscard]] static std::uint32_t Advance(
        PalScreenEffectState& state, float delta,
        IEffectRenderRuntime& render_runtime,
        IPreteritRuntime& preterit_runtime);
};

}  // namespace pal4
