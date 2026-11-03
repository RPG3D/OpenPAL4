#include "pal4/screen_effect_script.h"

#include <new>

namespace pal4 {

std::uint8_t ScreenEffectRuntimeStub::ReadByte(
    std::uint32_t /*address*/) { return 0u; }
void ScreenEffectRuntimeStub::SelectTechnique(
    std::uint32_t /*script*/, std::string_view /*name*/) {}
void ScreenEffectRuntimeStub::BindTexture(
    std::uint32_t /*script*/, std::string_view /*name*/,
    std::uint32_t /*texture*/) {}
void ScreenEffectRuntimeStub::ExecuteTechnique(
    std::uint32_t /*script*/, std::uint32_t& /*input*/,
    std::uint32_t /*flags*/) {}
void ScreenEffectRuntimeStub::Reset(ScreenEffectState& /*state*/) {}
bool ScreenEffectRuntimeStub::CheckCapability(
    std::uint32_t /*version*/) { return false; }
void ScreenEffectRuntimeStub::ProcessPackageScript() {}
void ScreenEffectRuntimeStub::CleanupScriptEnvironment(
    std::uint32_t& /*local_word*/) {}
std::uint32_t ScreenEffectRuntimeStub::OpenFile(
    std::string_view /*path*/, std::string_view /*mode*/) { return 0u; }
void ScreenEffectRuntimeStub::SeekFile(
    std::uint32_t /*file*/, std::uint32_t /*offset*/,
    std::uint32_t /*origin*/) {}
std::uint32_t ScreenEffectRuntimeStub::FileSize(
    std::uint32_t /*file*/) { return 0u; }
std::uint32_t ScreenEffectRuntimeStub::AllocateBuffer(
    std::uint32_t /*bytes*/) { return 0u; }
void ScreenEffectRuntimeStub::ReadFile(
    std::uint32_t /*buffer*/, std::uint32_t /*bytes*/,
    std::uint32_t /*count*/, std::uint32_t /*file*/) {}
void ScreenEffectRuntimeStub::CloseFile(std::uint32_t /*file*/) {}
std::uint32_t ScreenEffectRuntimeStub::CurrentD3DDevice() { return 0u; }
void ScreenEffectRuntimeStub::LoadTexture(
    std::uint32_t /*device*/, std::uint32_t /*buffer*/,
    std::uint32_t /*bytes*/, std::uint32_t& /*output_texture*/) {}
void ScreenEffectRuntimeStub::ReleaseEffectResource(
    std::uint32_t /*resource*/) {}
std::uint32_t ScreenEffectRuntimeStub::InvokeRenderSlot(
    std::uint32_t /*script*/, std::uint32_t /*slot_offset*/) { return 0u; }
std::uint32_t ScreenEffectRuntimeStub::InvokeParameterSlot(
    std::uint32_t /*script*/, std::uint32_t /*value*/) { return 0u; }
std::uint32_t ScreenEffectRuntimeStub::ReleaseScript(
    std::uint32_t /*script*/) { return 0u; }
std::uint32_t ScreenEffectRuntimeStub::LoadScript(
    std::uint32_t /*device*/, std::uint32_t /*buffer*/,
    std::uint32_t /*bytes*/, std::uint32_t /*compile_flags*/,
    std::uint32_t /*compile_options*/, std::uint32_t /*configuration*/,
    std::uint32_t /*execution_flags*/, std::uint32_t& /*output_script*/,
    std::uint32_t& /*error*/) { return 1u; }
std::string_view ScreenEffectRuntimeStub::ErrorText(
    std::uint32_t /*error*/, std::string_view /*label*/) { return {}; }
void ScreenEffectRuntimeStub::ShowError(
    std::string_view /*text*/, std::uint32_t /*message_box_type*/) {}
void ScreenEffectRuntimeStub::ReleaseEnvironmentOnLoadError(
    std::uint32_t /*environment*/) {}
void ScreenEffectRuntimeStub::FreeBuffer(
    std::uint32_t /*buffer*/) {}

std::uint32_t ScreenEffectScript::ApplyBloom(
    const ScreenEffectState& state, std::uint32_t input,
    IScreenEffectRuntime& runtime) {
    return ApplyTechnique(state, input, "techBloom", runtime);
}

std::uint32_t ScreenEffectScript::ApplyBlur(
    const ScreenEffectState& state, std::uint32_t input,
    IScreenEffectRuntime& runtime) {
    return ApplyTechnique(state, input, "techBlur", runtime);
}

std::uint32_t ScreenEffectScript::ApplyEmbossed(
    const ScreenEffectState& state, std::uint32_t input,
    IScreenEffectRuntime& runtime) {
    return ApplyTechnique(state, input, "techEmbossed", runtime);
}

std::uint32_t ScreenEffectScript::ApplyGlass(
    const GlassEffectState& state, std::uint32_t input,
    IScreenEffectRuntime& runtime) {
    if (state.loaded == 0u || state.enabled == 0u ||
        input == 0u || runtime.ReadByte(input) == 0u) {
        return 0u;
    }
    runtime.BindTexture(state.script, "texNoise", state.noise_texture);
    runtime.SelectTechnique(state.script, "techGlass");
    runtime.ExecuteTechnique(state.script, input, 0u);
    return input;
}

void ScreenEffectScript::CleanupGlass(
    GlassEffectState& state, IScreenEffectRuntime& runtime) {
    if (state.loaded == 0u) return;
    state.loaded = 0u;
    state.enabled = 0u;
    if (state.script != 0u) {
        runtime.ReleaseEffectResource(state.script);
        state.script = 0u;
    }
    if (state.noise_texture != 0u) {
        runtime.ReleaseEffectResource(state.noise_texture);
        state.noise_texture = 0u;
    }
}

std::uint32_t ScreenEffectScript::ApplyGray(
    const ScreenEffectState& state, std::uint32_t input,
    IScreenEffectRuntime& runtime) {
    return ApplyTechnique(state, input, "techGray", runtime);
}

std::uint32_t ScreenEffectScript::ApplyHilight(
    const ScreenEffectState& state, std::uint32_t input,
    IScreenEffectRuntime& runtime) {
    return ApplyTechnique(state, input, "techHilight", runtime);
}

std::uint32_t ScreenEffectScript::ApplyInvert(
    const ScreenEffectState& state, std::uint32_t input,
    IScreenEffectRuntime& runtime) {
    return ApplyTechnique(state, input, "techInvert", runtime);
}

std::uint32_t ScreenEffectScript::ApplyLaplace(
    const ScreenEffectState& state, std::uint32_t input,
    IScreenEffectRuntime& runtime) {
    return ApplyTechnique(state, input, "techLaplace", runtime);
}

std::uint32_t ScreenEffectScript::ApplyOldTV(
    const ScreenEffectState& state, std::uint32_t input,
    IScreenEffectRuntime& runtime) {
    return ApplyTechnique(state, input, "techOldTV", runtime);
}

std::uint32_t ScreenEffectScript::ApplyPosterize(
    const ScreenEffectState& state, std::uint32_t input,
    IScreenEffectRuntime& runtime) {
    return ApplyTechnique(state, input, "techPosterize", runtime);
}

std::uint32_t ScreenEffectScript::ApplyRed(
    const ScreenEffectState& state, std::uint32_t input,
    IScreenEffectRuntime& runtime) {
    return ApplyTechnique(state, input, "techRed", runtime);
}

std::uint32_t ScreenEffectScript::ApplySharpenEdges(
    const ScreenEffectState& state, std::uint32_t input,
    IScreenEffectRuntime& runtime) {
    return ApplyTechnique(state, input, "techSharepenEdges", runtime);
}

std::uint32_t ScreenEffectScript::ApplyTiling(
    const ScreenEffectState& state, std::uint32_t input,
    IScreenEffectRuntime& runtime) {
    return ApplyTechnique(state, input, "techTiling", runtime);
}

std::uint8_t ScreenEffectScript::SetTilingParameter(
    const ScreenEffectState& state, const std::uint32_t value,
    IScreenEffectRuntime& runtime) {
    if (state.loaded == 0u) return state.loaded;
    if (state.enabled == 0u) return state.enabled;
    return static_cast<std::uint8_t>(
        runtime.InvokeParameterSlot(state.script, value));
}

std::uint8_t ScreenEffectScript::CleanupSingleScript(
    ScreenEffectState& state, IScreenEffectRuntime& runtime) {
    if (state.loaded == 0u) return state.loaded;
    const auto script = state.script;
    state.loaded = 0u;
    state.enabled = 0u;
    if (script == 0u) return 0u;
    const auto result = runtime.ReleaseScript(script);
    state.script = 0u;
    return static_cast<std::uint8_t>(result);
}

std::uint32_t ScreenEffectScript::ApplyTechnique(
    const ScreenEffectState& state, std::uint32_t input,
    const std::string_view technique, IScreenEffectRuntime& runtime) {
    if (state.loaded == 0u || state.enabled == 0u ||
        input == 0u || runtime.ReadByte(input) == 0u) {
        return 0u;
    }
    runtime.SelectTechnique(state.script, technique);
    runtime.ExecuteTechnique(state.script, input, 0u);
    return input;
}

bool ScreenEffectScript::LoadBloom(
    ScreenEffectState& state, const std::uint32_t incoming_edi,
    const std::uint32_t incoming_environment_word,
    IScreenEffectRuntime& runtime) {
    return LoadFromPath(
        state, incoming_edi, incoming_environment_word,
        "gamedata\\2D\\ScreenEffect\\BloomScreen\\Bloom.fx",
        0xffff0200u, 0x104u, runtime);
}

void ScreenEffectScript::NoOp(std::uint32_t /*first*/,
                              std::uint32_t /*second*/) {}

bool ScreenEffectScript::LoadBlur(
    ScreenEffectState& state, const std::uint32_t incoming_edi,
    const std::uint32_t incoming_environment_word,
    IScreenEffectRuntime& runtime) {
    return LoadFromPath(
        state, incoming_edi, incoming_environment_word,
        "gamedata\\2D\\ScreenEffect\\BlurScreen\\Blur.fx",
        0xffff0200u, 0x104u, runtime);
}

bool ScreenEffectScript::LoadEmbossed(
    ScreenEffectState& state, const std::uint32_t incoming_edi,
    const std::uint32_t incoming_environment_word,
    IScreenEffectRuntime& runtime) {
    return LoadFromPath(
        state, incoming_edi, incoming_environment_word,
        "gamedata\\2D\\ScreenEffect\\EmbossedScreen\\Embossed.fx",
        0xffff0200u, 0x104u, runtime);
}

bool ScreenEffectScript::LoadGray(
    ScreenEffectState& state, const std::uint32_t incoming_edi,
    const std::uint32_t incoming_environment_word,
    IScreenEffectRuntime& runtime) {
    return LoadFromPath(
        state, incoming_edi, incoming_environment_word,
        "gamedata\\2D\\ScreenEffect\\GrayScreen\\Gray.fx",
        0xffff0101u, 0x104u, runtime);
}

bool ScreenEffectScript::LoadHilight(
    ScreenEffectState& state, const std::uint32_t incoming_edi,
    const std::uint32_t incoming_environment_word,
    IScreenEffectRuntime& runtime) {
    return LoadFromPath(
        state, incoming_edi, incoming_environment_word,
        "gamedata\\2D\\ScreenEffect\\HilightScreen\\Hilight.fx",
        0xffff0101u, 0x104u, runtime);
}

bool ScreenEffectScript::LoadInvert(
    ScreenEffectState& state, const std::uint32_t incoming_edi,
    const std::uint32_t incoming_environment_word,
    IScreenEffectRuntime& runtime) {
    return LoadFromPath(
        state, incoming_edi, incoming_environment_word,
        "gamedata\\2D\\ScreenEffect\\InvertScreen\\Invert.fx",
        0xffff0101u, 0x105u, runtime);
}

bool ScreenEffectScript::LoadLaplace(
    ScreenEffectState& state, const std::uint32_t incoming_edi,
    const std::uint32_t incoming_environment_word,
    IScreenEffectRuntime& runtime) {
    return LoadFromPath(
        state, incoming_edi, incoming_environment_word,
        "gamedata\\2D\\ScreenEffect\\LaplaceScreen\\Laplace.fx",
        0xffff0200u, 0x104u, runtime);
}

bool ScreenEffectScript::LoadOldTV(
    ScreenEffectState& state, const std::uint32_t incoming_edi,
    const std::uint32_t incoming_environment_word,
    IScreenEffectRuntime& runtime) {
    return LoadFromPath(
        state, incoming_edi, incoming_environment_word,
        "gamedata\\2D\\ScreenEffect\\OldTVScreen\\OldTV.fx",
        0xffff0101u, 0x105u, runtime);
}

bool ScreenEffectScript::LoadPosterize(
    ScreenEffectState& state, const std::uint32_t incoming_edi,
    const std::uint32_t incoming_environment_word,
    IScreenEffectRuntime& runtime) {
    return LoadFromPath(
        state, incoming_edi, incoming_environment_word,
        "gamedata\\2D\\ScreenEffect\\PosterizeScreen\\Posterize.fx",
        0xffff0200u, 0x104u, runtime);
}

bool ScreenEffectScript::LoadRed(
    ScreenEffectState& state, const std::uint32_t incoming_edi,
    const std::uint32_t incoming_environment_word,
    IScreenEffectRuntime& runtime) {
    return LoadFromPath(
        state, incoming_edi, incoming_environment_word,
        "gamedata\\2D\\ScreenEffect\\RedScreen\\Red.fx",
        0xffff0101u, 0x104u, runtime);
}

bool ScreenEffectScript::LoadSharpenEdges(
    ScreenEffectState& state, const std::uint32_t incoming_edi,
    const std::uint32_t incoming_environment_word,
    IScreenEffectRuntime& runtime) {
    return LoadFromPath(
        state, incoming_edi, incoming_environment_word,
        "gamedata\\2D\\ScreenEffect\\SharpenEdgesScreen\\SharpenEdges.fx",
        0xffff0200u, 0x104u, runtime);
}

bool ScreenEffectScript::LoadTiling(
    ScreenEffectState& state, const std::uint32_t incoming_edi,
    const std::uint32_t incoming_environment_word,
    IScreenEffectRuntime& runtime) {
    return LoadFromPath(
        state, incoming_edi, incoming_environment_word,
        "gamedata\\2D\\ScreenEffect\\TilingScreen\\Tiling.fx",
        0xffff0200u, 0x104u, runtime);
}

bool ScreenEffectScript::LoadGlass(
    GlassEffectState& state, const std::uint32_t incoming_edi,
    const std::uint32_t incoming_environment_word,
    IScreenEffectRuntime& runtime) {
    runtime.Reset(state);
    if (!runtime.CheckCapability(0xffff0200u)) return false;

    auto environment_word = incoming_environment_word;
    static_cast<void>(InitializeEnvironmentRecord(environment_word, runtime));
    try {
        const auto texture_device = runtime.CurrentD3DDevice();
        const auto texture_file = runtime.OpenFile(
            "gamedata\\2D\\ScreenEffect\\GlassScreen\\Glass.dds", "r");
        if (texture_file == 0u) {
            runtime.CleanupScriptEnvironment(environment_word);
            return false;
        }
        runtime.SeekFile(texture_file, 0u, 2u);
        const auto texture_bytes = runtime.FileSize(texture_file);
        runtime.SeekFile(texture_file, 0u, 0u);
        const auto texture_buffer = runtime.AllocateBuffer(texture_bytes);
        if (texture_buffer == 0u) throw std::bad_alloc();
        runtime.ReadFile(texture_buffer, texture_bytes, 1u, texture_file);
        runtime.CloseFile(texture_file);
        runtime.LoadTexture(texture_device, texture_buffer, texture_bytes,
                            state.noise_texture);
        runtime.FreeBuffer(texture_buffer);
        if (state.noise_texture == 0u) {
            runtime.CleanupScriptEnvironment(environment_word);
            return false;
        }

        const auto script_file = runtime.OpenFile(
            "gamedata\\2D\\ScreenEffect\\GlassScreen\\Glass.fx", "r");
        if (script_file == 0u) {
            runtime.CleanupScriptEnvironment(environment_word);
            return false;
        }
        runtime.SeekFile(script_file, 0u, 2u);
        const auto script_bytes = runtime.FileSize(script_file);
        runtime.SeekFile(script_file, 0u, 0u);
        const auto script_buffer = runtime.AllocateBuffer(script_bytes);
        if (script_buffer == 0u) throw std::bad_alloc();
        runtime.ReadFile(script_buffer, script_bytes, 1u, script_file);
        runtime.CloseFile(script_file);

        std::uint32_t error = 0u;
        const auto script_device = runtime.CurrentD3DDevice();
        const auto load_result = runtime.LoadScript(
            script_device, script_buffer, script_bytes, 0u, 0u,
            0x104u, 0u, state.script, error);
        state.enabled = 0u;
        if (load_result == 0u) {
            state.loaded = 1u;
        } else {
            state.loaded = 0u;
            state.script = 0u;
            runtime.ShowError(runtime.ErrorText(error, "Error"),
                              incoming_edi);
            runtime.ReleaseEnvironmentOnLoadError(texture_device);
        }
        runtime.FreeBuffer(script_buffer);
        const auto loaded = state.loaded != 0u;
        runtime.CleanupScriptEnvironment(environment_word);
        return loaded;
    } catch (...) {
        runtime.CleanupScriptEnvironment(environment_word);
        throw;
    }
}

std::uint32_t& ScreenEffectScript::InitializeEnvironmentRecord(
    std::uint32_t& local_word, IScreenEffectRuntime& runtime) {
    runtime.ProcessPackageScript();
    return local_word;
}

bool ScreenEffectScript::LoadFromPath(
    ScreenEffectState& state, const std::uint32_t incoming_edi,
    const std::uint32_t incoming_environment_word,
    const std::string_view path, const std::uint32_t capability,
    const std::uint32_t script_configuration,
    IScreenEffectRuntime& runtime) {
    runtime.Reset(state);
    if (!runtime.CheckCapability(capability)) {
        return false;
    }

    auto environment_word = incoming_environment_word;
    static_cast<void>(InitializeEnvironmentRecord(environment_word, runtime));
    try {
        const auto file = runtime.OpenFile(path, "r");
        if (file == 0u) {
            runtime.CleanupScriptEnvironment(environment_word);
            return false;
        }

        runtime.SeekFile(file, 0u, 2u);
        const auto bytes = runtime.FileSize(file);
        runtime.SeekFile(file, 0u, 0u);
        const auto buffer = runtime.AllocateBuffer(bytes);
        if (buffer == 0u) {
            throw std::bad_alloc();
        }
        runtime.ReadFile(buffer, bytes, 1u, file);
        runtime.CloseFile(file);

        std::uint32_t error = 0u;
        const auto device = runtime.CurrentD3DDevice();
        const auto load_result = runtime.LoadScript(
            device, buffer, bytes, 0u, 0u, script_configuration, 0u,
            state.script, error);
        state.enabled = 0u;
        if (load_result == 0u) {
            state.loaded = 1u;
        } else {
            state.loaded = 0u;
            state.script = 0u;
            runtime.ShowError(runtime.ErrorText(error, "Error"),
                              incoming_edi);
            runtime.ReleaseEnvironmentOnLoadError(environment_word);
        }
        runtime.FreeBuffer(buffer);
        const auto loaded = state.loaded != 0u;
        runtime.CleanupScriptEnvironment(environment_word);
        return loaded;
    } catch (...) {
        runtime.CleanupScriptEnvironment(environment_word);
        throw;
    }
}

std::uint8_t ScreenEffectControl::Enable(ScreenEffectState& state) {
    if (state.loaded == 0u) return state.loaded;
    const auto result = state.enabled;
    if (result == 0u) state.enabled = 1u;
    return result;
}

std::uint8_t ScreenEffectControl::Disable(ScreenEffectState& state) {
    if (state.loaded == 0u) return state.loaded;
    const auto result = state.enabled;
    if (result != 0u) state.enabled = 0u;
    return result;
}

std::uint8_t ScreenEffectControl::Render(
    const ScreenEffectState& state, IScreenEffectRuntime& runtime) {
    if (state.loaded == 0u) return state.loaded;
    if (state.enabled == 0u) return state.enabled;
    return static_cast<std::uint8_t>(
        runtime.InvokeRenderSlot(state.script, 276u));
}

std::uint8_t ScreenEffectControl::StopRendering(
    const ScreenEffectState& state, IScreenEffectRuntime& runtime) {
    if (state.loaded == 0u) return state.loaded;
    if (state.enabled == 0u) return state.enabled;
    return static_cast<std::uint8_t>(
        runtime.InvokeRenderSlot(state.script, 272u));
}

}  // namespace pal4
