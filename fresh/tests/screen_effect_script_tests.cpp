#include "pal4/screen_effect_script.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <new>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IScreenEffectRuntime {
public:
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.push_back("read byte");
        Check(address == 0x5000u, "bloom input byte address");
        return input_byte;
    }
    void SelectTechnique(std::uint32_t script,
                         std::string_view name) override {
        Check(script == 0x9000u && name == expected_technique,
              "effect selects observed technique");
        events.push_back("select technique");
    }
    void BindTexture(std::uint32_t script, std::string_view name,
                     std::uint32_t texture) override {
        Check(script == 0x9000u && name == "texNoise" &&
                  texture == 0x8100u,
              "glass binds observed noise texture before technique");
        events.push_back("bind texture");
    }
    void ExecuteTechnique(std::uint32_t script, std::uint32_t& input,
                          std::uint32_t flags) override {
        Check(script == 0x9000u && input == 0x5000u && flags == 0u,
              "bloom calls virtual +0x104 with input by address");
        events.push_back("execute technique");
        input = 0x6000u;
    }
    void Reset(pal4::ScreenEffectState& state) override {
        Check(state.vtable == 0x1234u,
              "reset receives screen-effect object");
        events.push_back("reset");
    }
    bool CheckCapability(std::uint32_t version) override {
        Check(version == expected_capability, "D3D capability version");
        events.push_back("capability");
        return capable;
    }
    void ProcessPackageScript() override {
        events.push_back("process package");
    }
    void CleanupScriptEnvironment(std::uint32_t& environment) override {
        Check(environment == 0x7000u, "unchanged incoming stack word");
        events.push_back("cleanup environment");
    }
    std::uint32_t OpenFile(std::string_view path,
                           std::string_view mode) override {
        const auto expected = expected_paths.empty() ? expected_path :
            expected_paths.at(open_count);
        Check(path == expected && mode == "r",
              "effect script path and read mode");
        Check(std::count(path.begin(), path.end(), '\\') == 4,
              "effect path uses four single directory separators");
        events.push_back("open");
        const auto result = file_results.empty() ? file_result :
            file_results.at(open_count);
        ++open_count;
        return result;
    }
    void SeekFile(std::uint32_t file, std::uint32_t offset,
                  std::uint32_t origin) override {
        Check(file == 0x8000u && offset == 0u,
              "seek uses open file and zero offset");
        events.push_back(origin == 2u ? "seek end" : "seek start");
        Check(origin == 2u || origin == 0u, "seek origin");
    }
    std::uint32_t FileSize(std::uint32_t file) override {
        Check(file == 0x8000u, "size uses open file");
        events.push_back("size");
        return 64u;
    }
    std::uint32_t AllocateBuffer(std::uint32_t bytes) override {
        Check(bytes == 64u, "buffer allocation uses script size");
        events.push_back("allocate");
        return buffer_result;
    }
    void ReadFile(std::uint32_t buffer, std::uint32_t bytes,
                  std::uint32_t count, std::uint32_t file) override {
        Check(buffer == 0xa000u && bytes == 64u && count == 1u &&
                  file == 0x8000u,
              "read receives whole file in one item");
        events.push_back("read");
    }
    void CloseFile(std::uint32_t file) override {
        Check(file == 0x8000u, "close uses open file");
        events.push_back("close");
    }
    std::uint32_t CurrentD3DDevice() override {
        events.push_back("device");
        return 0xb000u;
    }
    void LoadTexture(std::uint32_t device, std::uint32_t buffer,
                     std::uint32_t bytes,
                     std::uint32_t& output_texture) override {
        Check(device == 0xb000u && buffer == 0xa000u && bytes == 64u,
              "glass texture loader receives first device and file bytes");
        events.push_back("load texture");
        output_texture = texture_result;
    }
    void ReleaseEffectResource(std::uint32_t resource) override {
        Check(resource == 0x9000u || resource == 0x8100u,
              "cleanup releases only observed script and noise texture");
        events.push_back(resource == 0x9000u ? "release script" :
                                               "release texture");
    }
    std::uint32_t InvokeRenderSlot(std::uint32_t script,
                                   std::uint32_t slot_offset) override {
        Check(script == 0x9000u &&
                  (slot_offset == 272u || slot_offset == 276u),
              "effect rendering calls one of the observed script slots");
        events.push_back(slot_offset == 276u ? "render +276" :
                                                "render +272");
        return render_result;
    }
    std::uint32_t InvokeParameterSlot(
        const std::uint32_t script,
        const std::uint32_t value) override {
        Check(script == 0x9000u && value == 0x1357u,
              "tiling parameter uses script virtual +0x108");
        events.push_back("parameter +264");
        return 0x1234abcd;
    }
    std::uint32_t ReleaseScript(const std::uint32_t script) override {
        Check(script == 0x9000u,
              "single-script cleanup uses virtual +8");
        events.push_back("release script +8");
        return 0x1234abceu;
    }
    std::uint32_t LoadScript(
        std::uint32_t device, std::uint32_t buffer,
        std::uint32_t bytes, std::uint32_t compile_flags,
        std::uint32_t compile_options, std::uint32_t configuration,
        std::uint32_t execution_flags, std::uint32_t& output_script,
        std::uint32_t& error) override {
        Check(device == 0xb000u && buffer == 0xa000u && bytes == 64u &&
                  compile_flags == 0u && compile_options == 0u &&
                  configuration == expected_configuration &&
                  execution_flags == 0u &&
                  error == 0u,
              "script load receives observed D3D arguments");
        events.push_back("load");
        output_script = 0x9000u;
        error = 0xc000u;
        return load_result;
    }
    std::string_view ErrorText(std::uint32_t error,
                               std::string_view label) override {
        Check(error == 0xc000u && label == "Error",
              "load failure asks error object for Error text");
        events.push_back("error text");
        return "bad effect";
    }
    void ShowError(std::string_view text,
                   std::uint32_t message_box_type) override {
        Check(text == "bad effect" && message_box_type == 0x55u,
              "MessageBox receives error text and incoming EDI");
        events.push_back("message box");
    }
    void ReleaseEnvironmentOnLoadError(
        std::uint32_t environment) override {
        Check(environment == expected_release_argument,
              "load failure forwards observed release target");
        events.push_back("release environment");
    }
    void FreeBuffer(std::uint32_t buffer) override {
        Check(buffer == 0xa000u, "script buffer free address");
        events.push_back("free buffer");
    }

    std::uint8_t input_byte{1u};
    bool capable{true};
    std::uint32_t file_result{0x8000u};
    std::uint32_t buffer_result{0xa000u};
    std::uint32_t load_result{};
    std::uint32_t texture_result{0x8100u};
    std::uint32_t expected_release_argument{0x7000u};
    std::uint32_t expected_capability{0xffff0200u};
    std::uint32_t expected_configuration{0x104u};
    std::uint32_t render_result{0x1234abcdu};
    std::size_t open_count{};
    std::string_view expected_technique{"techBloom"};
    std::string_view expected_path{
        "gamedata\\2D\\ScreenEffect\\BloomScreen\\Bloom.fx"};
    std::vector<std::string_view> expected_paths;
    std::vector<std::uint32_t> file_results;
    std::vector<std::string> events;
};

pal4::ScreenEffectState State() {
    return {0x1234u, 1u, 1u, {0u, 0u}, 0x9000u};
}

void BloomGuardsAndPointerMutation() {
    Runtime runtime;
    auto state = State();
    state.loaded = 0u;
    Check(pal4::ScreenEffectScript::ApplyBloom(
              state, 0x5000u, runtime) == 0u &&
              runtime.events.empty(),
          "unloaded bloom makes no runtime calls");
    state.loaded = 1u;
    state.enabled = 0u;
    Check(pal4::ScreenEffectScript::ApplyBloom(
              state, 0x5000u, runtime) == 0u &&
              runtime.events.empty(),
          "disabled bloom makes no runtime calls");
    state.enabled = 1u;
    Check(pal4::ScreenEffectScript::ApplyBloom(
              state, 0u, runtime) == 0u &&
              runtime.events.empty(),
          "null input makes no runtime calls");
    runtime.input_byte = 0u;
    Check(pal4::ScreenEffectScript::ApplyBloom(
              state, 0x5000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>{"read byte"},
          "empty string stops before virtual calls");
    runtime.input_byte = 1u;
    runtime.events.clear();
    Check(pal4::ScreenEffectScript::ApplyBloom(
              state, 0x5000u, runtime) == 0x6000u &&
              runtime.events == std::vector<std::string>({
                  "read byte", "select technique", "execute technique"}),
          "successful bloom returns pointer updated by virtual +0x104");
}

void CapabilityAndOpenFailures() {
    Runtime runtime;
    auto state = State();
    runtime.capable = false;
    Check(!pal4::ScreenEffectScript::LoadBloom(
              state, 0x55u, 0x7000u, runtime) &&
              runtime.events == std::vector<std::string>({
                  "reset", "capability"}),
          "unsupported D3D exits before script environment");
    runtime.capable = true;
    runtime.file_result = 0u;
    runtime.events.clear();
    Check(!pal4::ScreenEffectScript::LoadBloom(
              state, 0x55u, 0x7000u, runtime) &&
              runtime.events == std::vector<std::string>({
                  "reset", "capability", "process package",
                  "open", "cleanup environment"}),
          "missing file cleans script environment and returns false");
}

void SuccessfulLoad() {
    Runtime runtime;
    auto state = State();
    state.loaded = 0u;
    Check(pal4::ScreenEffectScript::LoadBloom(
              state, 0x55u, 0x7000u, runtime) &&
              state.loaded == 1u && state.enabled == 0u &&
              state.script == 0x9000u,
          "successful load sets loaded and clears enabled flag");
    Check(runtime.events == std::vector<std::string>({
              "reset", "capability", "process package", "open",
              "seek end", "size", "seek start", "allocate", "read",
              "close", "device", "load", "free buffer",
              "cleanup environment"}),
          "successful load preserves file, D3D and cleanup order");
}

void FailedCompileReportsAndCleans() {
    Runtime runtime;
    runtime.load_result = 1u;
    auto state = State();
    Check(!pal4::ScreenEffectScript::LoadBloom(
              state, 0x55u, 0x7000u, runtime) &&
              state.loaded == 0u && state.enabled == 0u &&
              state.script == 0u,
          "failed load clears script and both flags");
    Check(runtime.events == std::vector<std::string>({
              "reset", "capability", "process package", "open",
              "seek end", "size", "seek start", "allocate", "read",
              "close", "device", "load", "error text", "message box",
              "release environment", "free buffer", "cleanup environment"}),
          "failure displays error and releases environment before buffer");
}

void AllocationFailureCleansEnvironment() {
    Runtime runtime;
    runtime.buffer_result = 0u;
    auto state = State();
    try {
        static_cast<void>(pal4::ScreenEffectScript::LoadBloom(
            state, 0x55u, 0x7000u, runtime));
        Check(false, "null buffer must not be read");
    } catch (const std::bad_alloc&) {
        Check(runtime.events == std::vector<std::string>({
                  "reset", "capability", "process package", "open",
                  "seek end", "size", "seek start", "allocate",
                  "cleanup environment"}),
              "allocation failure cleans environment without reading buffer");
    }
    pal4::ScreenEffectRuntimeStub stub;
    Check(!pal4::ScreenEffectScript::LoadBloom(
              state, 0u, 0x7000u, stub),
          "linkable empty runtime exits at capability gate");
}

void BlurUsesItsOwnTechniqueAndScript() {
    Runtime runtime;
    runtime.expected_technique = "techBlur";
    runtime.expected_path =
        "gamedata\\2D\\ScreenEffect\\BlurScreen\\Blur.fx";
    auto state = State();
    Check(pal4::ScreenEffectScript::ApplyBlur(
              state, 0x5000u, runtime) == 0x6000u &&
              runtime.events == std::vector<std::string>({
                  "read byte", "select technique", "execute technique"}),
          "blur selects techBlur and returns updated input pointer");
    runtime.events.clear();
    state.loaded = 0u;
    Check(pal4::ScreenEffectScript::ApplyBlur(
              state, 0x5000u, runtime) == 0u &&
              runtime.events.empty(),
          "blur shares the observed loaded-state gate");
    runtime.events.clear();
    Check(pal4::ScreenEffectScript::LoadBlur(
              state, 0x55u, 0x7000u, runtime) && state.loaded == 1u &&
              state.enabled == 0u && state.script == 0x9000u,
          "blur loads its own script with the same state transitions");
    Check(runtime.events == std::vector<std::string>({
              "reset", "capability", "process package", "open",
              "seek end", "size", "seek start", "allocate", "read",
              "close", "device", "load", "free buffer",
              "cleanup environment"}),
          "blur load preserves shared call order");
    pal4::ScreenEffectScript::NoOp(0x1234u, 0x5678u);
}

void EnvironmentWrapperReturnsUnchangedRecord() {
    Runtime runtime;
    std::uint32_t local_word = 0x7000u;
    auto& result = pal4::ScreenEffectScript::InitializeEnvironmentRecord(
        local_word, runtime);
    Check(&result == &local_word && local_word == 0x7000u &&
              runtime.events ==
                  std::vector<std::string>{"process package"},
          "0x41a9d0 calls global package processor and returns this");
}

void EmbossedUsesItsOwnTechniqueAndScript() {
    Runtime runtime;
    runtime.expected_technique = "techEmbossed";
    runtime.expected_path =
        "gamedata\\2D\\ScreenEffect\\EmbossedScreen\\Embossed.fx";
    auto state = State();
    Check(pal4::ScreenEffectScript::ApplyEmbossed(
              state, 0x5000u, runtime) == 0x6000u &&
              runtime.events == std::vector<std::string>({
                  "read byte", "select technique", "execute technique"}),
          "embossed effect selects technique and returns updated input");
    runtime.events.clear();
    state.enabled = 0u;
    Check(pal4::ScreenEffectScript::ApplyEmbossed(
              state, 0x5000u, runtime) == 0u && runtime.events.empty(),
          "disabled embossed effect exits before reading input");
    runtime.events.clear();
    Check(pal4::ScreenEffectScript::LoadEmbossed(
              state, 0x55u, 0x7000u, runtime) &&
              state.loaded == 1u && state.enabled == 0u &&
              runtime.events == std::vector<std::string>({
                  "reset", "capability", "process package", "open",
                  "seek end", "size", "seek start", "allocate", "read",
                  "close", "device", "load", "free buffer",
                  "cleanup environment"}),
          "embossed script uses exact path and shared load order");
}

void GlassBindsNoiseTextureBeforeExecution() {
    Runtime runtime;
    runtime.expected_technique = "techGlass";
    pal4::GlassEffectState state{};
    state.loaded = 1u;
    state.enabled = 1u;
    state.script = 0x9000u;
    state.noise_texture = 0x8100u;
    Check(pal4::ScreenEffectScript::ApplyGlass(
              state, 0x5000u, runtime) == 0x6000u &&
              runtime.events == std::vector<std::string>({
                  "read byte", "bind texture", "select technique",
                  "execute technique"}),
          "glass binds texture before selecting and executing technique");
    runtime.events.clear();
    state.enabled = 0u;
    Check(pal4::ScreenEffectScript::ApplyGlass(
              state, 0x5000u, runtime) == 0u && runtime.events.empty(),
          "disabled glass effect does not access runtime or input");
    state.enabled = 1u;
    runtime.input_byte = 0u;
    Check(pal4::ScreenEffectScript::ApplyGlass(
              state, 0x5000u, runtime) == 0u &&
              runtime.events == std::vector<std::string>({"read byte"}),
          "empty input byte prevents texture and script calls");
    runtime.events.clear();
    Check(pal4::ScreenEffectScript::ApplyGlass(
              state, 0u, runtime) == 0u && runtime.events.empty(),
          "null input exits before byte read");
}

pal4::GlassEffectState GlassState() {
    pal4::GlassEffectState state{};
    state.vtable = 0x1234u;
    state.loaded = 0u;
    state.enabled = 1u;
    state.script = 0u;
    state.noise_texture = 0u;
    return state;
}

void ConfigureGlassPaths(Runtime& runtime) {
    runtime.expected_paths = {
        "gamedata\\2D\\ScreenEffect\\GlassScreen\\Glass.dds",
        "gamedata\\2D\\ScreenEffect\\GlassScreen\\Glass.fx"};
}

void GlassLoadsTextureBeforeScript() {
    Runtime runtime;
    ConfigureGlassPaths(runtime);
    auto state = GlassState();
    Check(pal4::ScreenEffectScript::LoadGlass(
              state, 0x55u, 0x7000u, runtime) &&
              state.noise_texture == 0x8100u && state.script == 0x9000u &&
              state.loaded == 1u && state.enabled == 0u,
          "glass load stores texture and script at observed offsets");
    Check(runtime.events == std::vector<std::string>({
              "reset", "capability", "process package", "device",
              "open", "seek end", "size", "seek start", "allocate",
              "read", "close", "load texture", "free buffer",
              "open", "seek end", "size", "seek start", "allocate",
              "read", "close", "device", "load", "free buffer",
              "cleanup environment"}),
          "glass queries two devices and processes texture before script");
}

void GlassLoadFailurePaths() {
    Runtime runtime;
    ConfigureGlassPaths(runtime);
    auto state = GlassState();
    runtime.capable = false;
    Check(!pal4::ScreenEffectScript::LoadGlass(
              state, 0x55u, 0x7000u, runtime) &&
              runtime.events == std::vector<std::string>({
                  "reset", "capability"}),
          "capability failure stops before environment and device lookup");

    runtime = Runtime{};
    ConfigureGlassPaths(runtime);
    runtime.file_results = {0u};
    Check(!pal4::ScreenEffectScript::LoadGlass(
              state, 0x55u, 0x7000u, runtime) &&
              runtime.events == std::vector<std::string>({
                  "reset", "capability", "process package", "device",
                  "open", "cleanup environment"}),
          "missing texture file cleans environment before returning");

    runtime = Runtime{};
    ConfigureGlassPaths(runtime);
    runtime.texture_result = 0u;
    Check(!pal4::ScreenEffectScript::LoadGlass(
              state, 0x55u, 0x7000u, runtime) &&
              runtime.open_count == 1u &&
              runtime.events.back() == "cleanup environment" &&
              state.noise_texture == 0u,
          "failed texture creation prevents script-file open");

    runtime = Runtime{};
    ConfigureGlassPaths(runtime);
    runtime.file_results = {0x8000u, 0u};
    Check(!pal4::ScreenEffectScript::LoadGlass(
              state, 0x55u, 0x7000u, runtime) &&
              runtime.open_count == 2u &&
              runtime.events.back() == "cleanup environment",
          "missing script file cleans environment after texture load");

    runtime = Runtime{};
    ConfigureGlassPaths(runtime);
    runtime.load_result = 1u;
    runtime.expected_release_argument = 0xb000u;
    Check(!pal4::ScreenEffectScript::LoadGlass(
              state, 0x55u, 0x7000u, runtime) &&
              state.loaded == 0u && state.enabled == 0u &&
              state.script == 0u && state.noise_texture == 0x8100u,
          "failed glass script clears script state but retains texture");
    Check(runtime.events.end() - 5 ==
              std::find(runtime.events.begin(), runtime.events.end(),
                        "error text") &&
              runtime.events.back() == "cleanup environment",
          "glass compile failure reports error and releases first device");
}

void GlassCleanupGatesAndReleaseOrder() {
    Runtime runtime;
    auto state = GlassState();
    state.script = 0x9000u;
    state.noise_texture = 0x8100u;
    state.enabled = 1u;
    pal4::ScreenEffectScript::CleanupGlass(state, runtime);
    Check(state.loaded == 0u && state.enabled == 1u &&
              state.script == 0x9000u && state.noise_texture == 0x8100u &&
              runtime.events.empty(),
          "unloaded glass cleanup leaves flags and resources untouched");
    state.loaded = 1u;
    pal4::ScreenEffectScript::CleanupGlass(state, runtime);
    Check(state.loaded == 0u && state.enabled == 0u &&
              state.script == 0u && state.noise_texture == 0u &&
              runtime.events == std::vector<std::string>({
                  "release script", "release texture"}),
          "loaded glass cleanup releases script before texture");
    runtime.events.clear();
    state.loaded = 1u;
    pal4::ScreenEffectScript::CleanupGlass(state, runtime);
    Check(runtime.events.empty() && state.loaded == 0u,
          "null resources are not released again");
}

void GrayUsesOlderCapabilityAndOwnTechnique() {
    Runtime runtime;
    runtime.expected_capability = 0xffff0101u;
    runtime.expected_technique = "techGray";
    runtime.expected_path =
        "gamedata\\2D\\ScreenEffect\\GrayScreen\\Gray.fx";
    auto state = State();
    Check(pal4::ScreenEffectScript::ApplyGray(
              state, 0x5000u, runtime) == 0x6000u &&
              runtime.events == std::vector<std::string>({
                  "read byte", "select technique", "execute technique"}),
          "gray effect selects techGray and returns updated input");
    runtime.events.clear();
    state.loaded = 0u;
    Check(pal4::ScreenEffectScript::ApplyGray(
              state, 0x5000u, runtime) == 0u && runtime.events.empty(),
          "gray input is untouched until effect is loaded");
    Check(pal4::ScreenEffectScript::LoadGray(
              state, 0x55u, 0x7000u, runtime) &&
              state.loaded == 1u && state.enabled == 0u &&
              runtime.events == std::vector<std::string>({
                  "reset", "capability", "process package", "open",
                  "seek end", "size", "seek start", "allocate", "read",
                  "close", "device", "load", "free buffer",
                  "cleanup environment"}),
          "gray loader uses version 0xffff0101 and exact script path");
}

void HilightAndInvertUseObservedTechniques() {
    Runtime runtime;
    runtime.expected_capability = 0xffff0101u;
    runtime.expected_technique = "techHilight";
    runtime.expected_path =
        "gamedata\\2D\\ScreenEffect\\HilightScreen\\Hilight.fx";
    auto state = State();
    Check(pal4::ScreenEffectScript::ApplyHilight(
              state, 0x5000u, runtime) == 0x6000u &&
              runtime.events == std::vector<std::string>({
                  "read byte", "select technique", "execute technique"}),
          "hilight selects its observed technique and updates input");
    runtime.events.clear();
    Check(pal4::ScreenEffectScript::LoadHilight(
              state, 0x55u, 0x7000u, runtime) &&
              state.loaded == 1u && state.enabled == 0u &&
              runtime.events.front() == "reset" &&
              runtime.events.back() == "cleanup environment",
          "hilight uses version 0xffff0101 and exact file path");
    runtime.events.clear();
    runtime.expected_technique = "techInvert";
    state.enabled = 1u;
    Check(pal4::ScreenEffectScript::ApplyInvert(
              state, 0x5000u, runtime) == 0x6000u &&
              runtime.events == std::vector<std::string>({
                  "read byte", "select technique", "execute technique"}),
          "invert selects its own technique with common execution order");
}

void InvertLoaderUsesDistinctScriptConfiguration() {
    Runtime runtime;
    runtime.expected_capability = 0xffff0101u;
    runtime.expected_configuration = 0x105u;
    runtime.expected_path =
        "gamedata\\2D\\ScreenEffect\\InvertScreen\\Invert.fx";
    auto state = State();
    state.loaded = 0u;
    Check(pal4::ScreenEffectScript::LoadInvert(
              state, 0x55u, 0x7000u, runtime) &&
              state.loaded == 1u && state.enabled == 0u &&
              state.script == 0x9000u &&
              runtime.events == std::vector<std::string>({
                  "reset", "capability", "process package", "open",
                  "seek end", "size", "seek start", "allocate", "read",
                  "close", "device", "load", "free buffer",
                  "cleanup environment"}),
          "invert loader uses version 0xffff0101 and script config 0x105");
}

void LaplaceUsesObservedTechniqueAndScript() {
    Runtime runtime;
    runtime.expected_technique = "techLaplace";
    runtime.expected_capability = 0xffff0200u;
    runtime.expected_configuration = 0x104u;
    runtime.expected_path =
        "gamedata\\2D\\ScreenEffect\\LaplaceScreen\\Laplace.fx";
    auto state = State();
    Check(pal4::ScreenEffectScript::ApplyLaplace(
              state, 0x5000u, runtime) == 0x6000u &&
              runtime.events == std::vector<std::string>({
                  "read byte", "select technique", "execute technique"}),
          "laplace uses techLaplace and forwards the updated input");
    runtime.events.clear();
    state.enabled = 0u;
    Check(pal4::ScreenEffectScript::ApplyLaplace(
              state, 0x5000u, runtime) == 0u && runtime.events.empty(),
          "laplace skips script calls when disabled");
    Check(pal4::ScreenEffectScript::LoadLaplace(
              state, 0x55u, 0x7000u, runtime) &&
              state.loaded == 1u && state.enabled == 0u &&
              state.script == 0x9000u &&
              runtime.events == std::vector<std::string>({
                  "reset", "capability", "process package", "open",
                  "seek end", "size", "seek start", "allocate", "read",
                  "close", "device", "load", "free buffer",
                  "cleanup environment"}),
          "laplace loads exact path with version 0xffff0200 and config 0x104");
}

void OldTVUsesOwnTechniqueAndScriptConfiguration() {
    Runtime runtime;
    runtime.expected_technique = "techOldTV";
    runtime.expected_capability = 0xffff0101u;
    runtime.expected_configuration = 0x105u;
    runtime.expected_path =
        "gamedata\\2D\\ScreenEffect\\OldTVScreen\\OldTV.fx";
    auto state = State();
    Check(pal4::ScreenEffectScript::ApplyOldTV(
              state, 0x5000u, runtime) == 0x6000u &&
              runtime.events == std::vector<std::string>({
                  "read byte", "select technique", "execute technique"}),
          "old TV selects its own technique and forwards input by reference");
    runtime.events.clear();
    Check(pal4::ScreenEffectScript::LoadOldTV(
              state, 0x55u, 0x7000u, runtime) &&
              state.loaded == 1u && state.enabled == 0u &&
              runtime.events == std::vector<std::string>({
                  "reset", "capability", "process package", "open",
                  "seek end", "size", "seek start", "allocate", "read",
                  "close", "device", "load", "free buffer",
                  "cleanup environment"}),
          "old TV loads exact path with version 0xffff0101 and config 0x105");
}

void PosterizeUsesOwnTechniqueAndScriptConfiguration() {
    Runtime runtime;
    runtime.expected_technique = "techPosterize";
    runtime.expected_capability = 0xffff0200u;
    runtime.expected_configuration = 0x104u;
    runtime.expected_path =
        "gamedata\\2D\\ScreenEffect\\PosterizeScreen\\Posterize.fx";
    auto state = State();
    Check(pal4::ScreenEffectScript::ApplyPosterize(
              state, 0x5000u, runtime) == 0x6000u &&
              runtime.events == std::vector<std::string>({
                  "read byte", "select technique", "execute technique"}),
          "posterize selects its own technique and forwards input");
    runtime.events.clear();
    Check(pal4::ScreenEffectScript::LoadPosterize(
              state, 0x55u, 0x7000u, runtime) &&
              state.loaded == 1u && state.enabled == 0u &&
              runtime.events == std::vector<std::string>({
                  "reset", "capability", "process package", "open",
                  "seek end", "size", "seek start", "allocate", "read",
                  "close", "device", "load", "free buffer",
                  "cleanup environment"}),
          "posterize loads exact path with version 0xffff0200 and config 0x104");
}

void RedAndSharpenUseObservedStrings() {
    Runtime red;
    red.expected_technique = "techRed";
    red.expected_capability = 0xffff0101u;
    red.expected_path =
        "gamedata\\2D\\ScreenEffect\\RedScreen\\Red.fx";
    auto red_state = State();
    Check(pal4::ScreenEffectScript::ApplyRed(
              red_state, 0x5000u, red) == 0x6000u &&
              red.events == std::vector<std::string>({
                  "read byte", "select technique", "execute technique"}),
          "red effect selects techRed");
    red.events.clear();
    Check(pal4::ScreenEffectScript::LoadRed(
              red_state, 0x55u, 0x7000u, red) &&
              red_state.loaded == 1u && red_state.enabled == 0u &&
              red.events.back() == "cleanup environment",
          "red loads its own path at version 0xffff0101 and config 0x104");

    Runtime sharpen;
    sharpen.expected_technique = "techSharepenEdges";
    sharpen.expected_capability = 0xffff0200u;
    sharpen.expected_path =
        "gamedata\\2D\\ScreenEffect\\SharpenEdgesScreen\\SharpenEdges.fx";
    auto sharpen_state = State();
    Check(pal4::ScreenEffectScript::ApplySharpenEdges(
              sharpen_state, 0x5000u, sharpen) == 0x6000u &&
              sharpen.events == std::vector<std::string>({
                  "read byte", "select technique", "execute technique"}),
          "sharpen effect preserves binary's techSharepenEdges spelling");
    sharpen.events.clear();
    Check(pal4::ScreenEffectScript::LoadSharpenEdges(
              sharpen_state, 0x55u, 0x7000u, sharpen) &&
              sharpen_state.loaded == 1u && sharpen_state.enabled == 0u &&
              sharpen.events.back() == "cleanup environment",
          "sharpen loads exact path at version 0xffff0200 and config 0x104");
}

void TilingUsesParameterSlotAndOwnScript() {
    Runtime runtime;
    runtime.expected_technique = "techTiling";
    runtime.expected_path =
        "gamedata\\2D\\ScreenEffect\\TilingScreen\\Tiling.fx";
    auto state = State();
    Check(pal4::ScreenEffectScript::ApplyTiling(
              state, 0x5000u, runtime) == 0x6000u &&
              runtime.events == std::vector<std::string>({
                  "read byte", "select technique", "execute technique"}),
          "tiling uses techTiling and updates input pointer");
    runtime.events.clear();
    Check(pal4::ScreenEffectScript::SetTilingParameter(
              state, 0x1357u, runtime) == 0xcdu &&
              runtime.events == std::vector<std::string>{"parameter +264"},
          "tiling parameter returns low byte of virtual result");
    state.enabled = 0u;
    runtime.events.clear();
    Check(pal4::ScreenEffectScript::SetTilingParameter(
              state, 0x1357u, runtime) == 0u && runtime.events.empty(),
          "disabled tiling parameter has no script call");
    Check(pal4::ScreenEffectScript::LoadTiling(
              state, 0x55u, 0x7000u, runtime) &&
              state.loaded == 1u && state.enabled == 0u &&
              runtime.events == std::vector<std::string>({
                  "reset", "capability", "process package", "open",
                  "seek end", "size", "seek start", "allocate", "read",
                  "close", "device", "load", "free buffer",
                  "cleanup environment"}),
          "tiling loader preserves exact path and 0xffff0200/0x104 settings");
}

void SingleScriptCleanupPreservesGateAndReleaseResult() {
    Runtime runtime;
    auto state = State();
    state.loaded = 0u;
    Check(pal4::ScreenEffectScript::CleanupSingleScript(
              state, runtime) == 0u && state.script == 0x9000u &&
              state.enabled == 1u && runtime.events.empty(),
          "unloaded cleanup leaves fields untouched");
    state.loaded = 1u;
    state.script = 0u;
    Check(pal4::ScreenEffectScript::CleanupSingleScript(
              state, runtime) == 0u && state.loaded == 0u &&
              state.enabled == 0u && runtime.events.empty(),
          "loaded cleanup clears flags without a script pointer");
    state.loaded = 1u;
    state.enabled = 1u;
    state.script = 0x9000u;
    Check(pal4::ScreenEffectScript::CleanupSingleScript(
              state, runtime) == 0xceu && state.loaded == 0u &&
              state.enabled == 0u && state.script == 0u &&
              runtime.events ==
                  std::vector<std::string>{"release script +8"},
          "script cleanup returns low byte of virtual release and nulls pointer");
}

void EffectControlUsesOldFlagAndRenderSlots() {
    auto state = State();
    Runtime runtime;
    state.loaded = 0u;
    Check(pal4::ScreenEffectControl::Enable(state) == 0u &&
              pal4::ScreenEffectControl::Disable(state) == 0u &&
              state.enabled == 1u &&
              pal4::ScreenEffectControl::Render(state, runtime) == 0u &&
              pal4::ScreenEffectControl::StopRendering(state, runtime) == 0u &&
              runtime.events.empty(),
          "unloaded effect leaves enable byte and runtime untouched");
    state.loaded = 1u;
    state.enabled = 0u;
    Check(pal4::ScreenEffectControl::Enable(state) == 0u &&
              state.enabled == 1u &&
              pal4::ScreenEffectControl::Enable(state) == 1u,
          "enable returns the prior enabled byte");
    Check(pal4::ScreenEffectControl::Render(state, runtime) == 0xcdu &&
              pal4::ScreenEffectControl::StopRendering(state, runtime) ==
                  0xcdu &&
              runtime.events == std::vector<std::string>({
                  "render +276", "render +272"}),
          "enabled rendering forwards both virtual slots and low result byte");
    runtime.events.clear();
    Check(pal4::ScreenEffectControl::Disable(state) == 1u &&
              state.enabled == 0u &&
              pal4::ScreenEffectControl::Disable(state) == 0u &&
              pal4::ScreenEffectControl::Render(state, runtime) == 0u &&
              runtime.events.empty(),
          "disable returns old byte and gates rendering");
}

}  // namespace

int main() {
    BloomGuardsAndPointerMutation();
    CapabilityAndOpenFailures();
    SuccessfulLoad();
    FailedCompileReportsAndCleans();
    AllocationFailureCleansEnvironment();
    BlurUsesItsOwnTechniqueAndScript();
    EnvironmentWrapperReturnsUnchangedRecord();
    EmbossedUsesItsOwnTechniqueAndScript();
    GlassBindsNoiseTextureBeforeExecution();
    GlassLoadsTextureBeforeScript();
    GlassLoadFailurePaths();
    GlassCleanupGatesAndReleaseOrder();
    GrayUsesOlderCapabilityAndOwnTechnique();
    HilightAndInvertUseObservedTechniques();
    InvertLoaderUsesDistinctScriptConfiguration();
    LaplaceUsesObservedTechniqueAndScript();
    OldTVUsesOwnTechniqueAndScriptConfiguration();
    PosterizeUsesOwnTechniqueAndScriptConfiguration();
    RedAndSharpenUseObservedStrings();
    TilingUsesParameterSlotAndOwnScript();
    SingleScriptCleanupPreservesGateAndReleaseResult();
    EffectControlUsesOldFlagAndRenderSlots();
    std::cout << "21 screen effect script test cases passed\n";
}
