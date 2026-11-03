#include "pal4/pal_screen_effect.h"
#include "pal4/screen_fade_state.h"

namespace pal4 {

namespace {
constexpr std::string_view kSourceFile =
    "D:\\VcProject\\Pal4\\project\\Sourcecode\\PAL4\\PAL4Game\\ScreenEffect\\palScreenEffect.cpp";
constexpr std::string_view kBeginLog =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  palScreenEffect initializing begin...\n\n\n";
constexpr std::string_view kCompleteLog =
    "FILE:  [%s]\nLINE:  [%d]\nINFO:  palScreenEffect initialize ok\n\n\n";
}

std::uint32_t PalScreenEffectRuntimeStub::GetSingletonManager() { return 0u; }
std::uint32_t PalScreenEffectRuntimeStub::Register(
    std::uint32_t, std::string_view, std::uint32_t,
    std::uint32_t, std::uint32_t) { return 0u; }
PalScreenEffectState* PalScreenEffectRuntimeStub::Allocate(
    std::uint32_t) { return nullptr; }
std::uint32_t PalScreenEffectRuntimeStub::Free(
    PalScreenEffectState*) { return 0u; }
std::int32_t PalScreenEffectRuntimeStub::ScreenWidth() { return 0; }
std::int32_t PalScreenEffectRuntimeStub::ScreenHeight() { return 0; }
void PalScreenEffectRuntimeStub::EmptyDestructor(const void*) {}
void PalScreenEffectRuntimeStub::Log(
    std::string_view, std::string_view, std::uint32_t) {}

std::uint32_t PalScreenEffect::GetSingleton(
    PalScreenEffectAccessState& access,
    IPalScreenEffectRuntime& runtime) {
    if ((access.guard & 1u) != 0u) return access.instance;
    access.guard = static_cast<std::uint8_t>(access.guard | 1u);
    access.instance = runtime.Register(
        runtime.GetSingletonManager(), "palScreenEffect", 1018u,
        kFactoryAddress, kDestroyAddress);
    return access.instance;
}

PalScreenEffectState* PalScreenEffect::Create(
    IPalScreenEffectRuntime& runtime) {
    auto* state = runtime.Allocate(0x98u);
    if (state == nullptr) return nullptr;
    try {
        return &Construct(*state, runtime);
    } catch (...) {
        static_cast<void>(runtime.Free(state));
        throw;
    }
}

std::uint32_t PalScreenEffect::Destroy(
    PalScreenEffectState* state, std::uint32_t incoming_eax,
    IPalScreenEffectRuntime& runtime) {
    if (state == nullptr) return incoming_eax;
    Destruct(*state, runtime);
    return runtime.Free(state);
}

PalScreenEffectState& PalScreenEffect::Construct(
    PalScreenEffectState& state, IPalScreenEffectRuntime& runtime) {
    const auto render_width = runtime.ScreenWidth();
    const auto render_height = runtime.ScreenHeight();
    static_cast<void>(EffectRenderControl::Initialize(
        state.render, render_width, render_height));
    const auto preterit_width = runtime.ScreenWidth();
    const auto preterit_height = runtime.ScreenHeight();
    static_cast<void>(PreteritEffect::Initialize(
        state.preterit, preterit_width, preterit_height));
    return state;
}

void PalScreenEffect::Destruct(
    PalScreenEffectState& state, IPalScreenEffectRuntime& runtime) {
    runtime.EmptyDestructor(&state.preterit);
    runtime.EmptyDestructor(&state.render);
}

void PalScreenEffect::ReleaseResources(
    PalScreenEffectState& state, IEffectRenderRuntime& runtime) {
    static_cast<void>(EffectRenderControl::DestroyTexture(state.render, runtime));
    EffectTextureArray::Cleanup(state.preterit, runtime);
}

std::uint32_t PalScreenEffect::ResetParameters(
    PalScreenEffectState& state) {
    EffectRenderControl::ResetAnimation(state.render);
    return 0u;
}

void PalScreenEffect::InitializeResources(
    PalScreenEffectState& state, IPalScreenEffectRuntime& runtime,
    IEffectRenderRuntime& render_runtime,
    IPreteritRuntime& preterit_runtime) {
    runtime.Log(kBeginLog, kSourceFile, 33u);
    EffectRenderControl::InitializeTexture(state.render, render_runtime);
    PreteritEffect::LoadTextures(
        state.preterit, render_runtime, preterit_runtime);
    runtime.Log(kCompleteLog, kSourceFile, 39u);
}

std::uint32_t PalScreenEffect::StartFade(
    PalScreenEffectState& state, float duration,
    std::uint32_t keep_active,
    std::uint8_t red, std::uint8_t green, std::uint8_t blue,
    float alpha_start, float alpha_end,
    float alpha_on_finish) {
    auto& render = state.render;
    if (render.texture == 0u) return 0u;
    ConfigureScreenFade(render,duration,keep_active,red,green,blue,
                        alpha_start,alpha_end,alpha_on_finish);
    return keep_active;
}

std::uint32_t PalScreenEffect::Draw(
    const PalScreenEffectState& state, std::uint32_t context,
    IEffectRenderRuntime& runtime) {
    static_cast<void>(EffectRenderControl::Draw(
        state.render, context, runtime));
    return PreteritEffect::Draw(state.preterit, context, runtime);
}

std::uint32_t PalScreenEffect::Advance(
    PalScreenEffectState& state, float delta,
    IEffectRenderRuntime& render_runtime,
    IPreteritRuntime& preterit_runtime) {
    EffectRenderControl::AdvanceFade(state.render, delta, render_runtime);
    return PreteritEffect::Advance(state.preterit, delta, preterit_runtime);
}

}  // namespace pal4
