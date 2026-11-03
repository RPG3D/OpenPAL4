#include "pal4/hook_effect.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

struct FactoryRuntime final : pal4::IHookEffectFactoryRuntime {
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t user_data = 0x7200u;
    std::uint32_t allocation = 0x8000u;
    bool initialized = true;

    std::uint32_t ClumpUserData(std::uint32_t clump,
                                std::string_view name) override {
        Check(clump == 0x5000u && name == "hook_effect",
              "factory looks up hook_effect user data in its clump");
        calls.emplace_back("user data");
        return user_data;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == pal4::HookEffect::kObjectSize,
              "factory allocates the 16-byte hook object");
        calls.emplace_back("allocate");
        return allocation;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == allocation + 4u && value == 0u,
              "constructor clears its state byte");
        calls.emplace_back("state");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back(address == allocation + 8u ? "object" : "vtable");
    }
    bool Initialize(std::uint32_t effect, std::uint32_t config,
                    std::uint32_t data, std::uint32_t context) override {
        Check(effect == allocation && config == 0x1000u &&
              data == user_data && context == 0x3000u &&
              words.at(effect + 8u) == 0u &&
              words.at(effect) == pal4::HookEffect::kVtable,
              "initializer sees the native fields and arguments");
        calls.emplace_back("initialize");
        return initialized;
    }
    void Destroy(std::uint32_t effect, bool free_memory) override {
        Check(effect == allocation && free_memory,
              "failed initializer deletes the allocated hook");
        calls.emplace_back("destroy");
    }
};

struct TimeRuntime final : pal4::IHookEffectTimeRuntime {
    static constexpr std::uint32_t kEffect = 0x8000u;
    static constexpr std::uint32_t kObject = 0x9000u;
    static constexpr std::uint32_t kFrame = 0xa000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {kEffect + 8u, kObject}, {kObject + 44u, kFrame},
        {kFrame + 4u, 0x1111u}};
    std::vector<std::string> calls;
    std::array<float, 16u> transform{};

    TimeRuntime() {
        for (std::size_t index = 0; index < transform.size(); ++index)
            transform[index] = static_cast<float>(index + 1u);
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    float ReadFloat(std::uint32_t address) override {
        if (address == kObject + 276u) return 1.25f;
        Check(address == kObject + 280u,
              "time reads the two adjacent Dialog fields");
        return 2.5f;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == kEffect + 4u && value == 1u,
              "time query marks the effect active");
        calls.emplace_back("active");
    }
    std::array<float, 16u> FrameTransform(std::uint32_t frame) override {
        Check(frame == kFrame, "time query captures the Dialog frame");
        calls.emplace_back("capture");
        return transform;
    }
    void RemoveFrameChild(std::uint32_t frame) override {
        Check(frame == kFrame, "parented Dialog frame is detached");
        calls.emplace_back("detach");
    }
    void SetFrameIdentity(std::uint32_t frame) override {
        Check(frame == kFrame, "frame identity targets the Dialog frame");
        calls.emplace_back("identity");
    }
    void ApplyFrameTransform(
        std::uint32_t frame, const std::array<float, 16u>& matrix,
        std::uint32_t mode) override {
        Check(frame == kFrame && matrix == transform && mode == 0u,
              "frame transform is restored with native mode zero");
        calls.emplace_back("restore");
    }
    void UpdateFrameObjects(std::uint32_t frame) override {
        Check(frame == kFrame, "frame update targets the Dialog frame");
        calls.emplace_back("update");
    }
};

struct ConfigRuntime final : pal4::IHookEffectConfigRuntime {
    static constexpr std::uint32_t kEffect = 0x8000u;
    static constexpr std::uint32_t kObject = 0x9000u;
    static constexpr std::uint32_t kOwner = 0xb000u;
    static constexpr std::uint32_t kFrame = 0xc000u;
    static constexpr std::uint32_t kConfig = 0x1000u;
    std::map<std::string, std::int32_t> ints;
    std::map<std::string, float> floats;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, float> written_floats;
    std::vector<std::string> calls;
    std::uint32_t object = kObject;
    std::uint32_t found_texture = 0u;
    std::uint32_t loaded_texture = 0x7000u;
    bool setup = true;
    bool not_point = false;
    bool axis_xz = false;
    bool alpha_blend = false;
    bool face_point = false;
    std::uint32_t setup_texture = 0u;
    std::uint32_t advances = 0u;
    double last_step = 0.0;

    void Reset(std::uint32_t effect) override {
        Check(effect == kEffect, "configuration resets its hook first");
        calls.emplace_back("reset");
    }
    std::int32_t ReadInt(std::uint32_t config, std::string_view key,
                         std::int32_t fallback) override {
        Check(config == kConfig, "integer attributes use the XML node");
        const auto name = std::string{key};
        calls.push_back("int " + name);
        const auto found = ints.find(name);
        return found == ints.end() ? fallback : found->second;
    }
    float ReadFloat(std::uint32_t config, std::string_view key,
                    float fallback) override {
        Check(config == kConfig, "float attributes use the XML node");
        const auto name = std::string{key};
        calls.push_back("float " + name);
        const auto found = floats.find(name);
        return found == floats.end() ? fallback : found->second;
    }
    std::string ReadText(std::uint32_t config,
                         std::string_view key) override {
        Check(config == kConfig && key == "Attr_Prt_Texture",
              "texture name uses the native attribute");
        calls.emplace_back("texture name");
        return "flare.dds";
    }
    std::uint32_t CreateParticleObject(std::uint32_t size) override {
        Check(size == 0x1b8u, "native particle object occupies 440 bytes");
        calls.emplace_back("create object");
        return object;
    }
    void DestroyParticleObject(std::uint32_t address) override {
        Check(address == kObject, "failed setup destroys its particle object");
        calls.emplace_back("destroy object");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void WriteFloat(std::uint32_t address, float value) override {
        written_floats[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == kOwner + 48u,
              "pre-render reads the owner frame each step");
        return 0xd000u;
    }
    void UpdateParticleFrame(std::uint32_t address) override {
        Check(address == kObject &&
              written_floats.count(kObject + 88u) == 1u,
              "particle frame update follows atomic rotations");
        calls.emplace_back("update frame");
    }
    std::uint32_t FindTexture(std::string_view path) override {
        Check(path == "gamedata\\effect\\tex\\flare.dds",
              "texture lookup receives the native resource path");
        calls.emplace_back("find texture");
        return found_texture;
    }
    std::uint32_t LoadTexture(std::string_view path) override {
        Check(path == "gamedata\\effect\\tex\\flare.dds",
              "texture fallback receives the same resource path");
        calls.emplace_back("load texture");
        return loaded_texture;
    }
    void DestroyTexture(std::uint32_t texture) override {
        Check(texture == (found_texture != 0u ? found_texture :
                          loaded_texture),
              "texture release uses the acquired resource");
        calls.emplace_back("destroy texture");
    }
    void SetProperties(std::uint32_t address,
                       bool enabled) override {
        Check(address == kObject, "properties target the particle object");
        alpha_blend = enabled;
        calls.emplace_back("properties");
    }
    bool SetupObject(std::uint32_t address, bool ordinary,
                     std::uint32_t texture, std::uint32_t frame,
                     bool xz) override {
        Check(address == kObject && frame == kFrame &&
              words.at(kEffect + 12u) == kOwner,
              "setup sees owner and attachment frame");
        not_point = ordinary;
        setup_texture = texture;
        axis_xz = xz;
        calls.emplace_back("setup");
        return setup;
    }
    void SetCollectionState(std::uint32_t address,
                            bool face) override {
        Check(address == kObject,
              "collection state targets the particle object");
        face_point = face;
        calls.emplace_back("collection");
    }
    void Advance(std::uint32_t address, double seconds,
                 std::uint32_t owner_frame) override {
        Check(address == kObject && owner_frame == 0xd000u,
              "pre-render advances the object with its owner frame");
        ++advances;
        last_step = seconds;
    }
};

struct LifecycleRuntime final : pal4::IHookEffectLifecycleRuntime {
    static constexpr std::uint32_t kEffect = 0x8000u;
    static constexpr std::uint32_t kObject = 0x9000u;
    static constexpr std::uint32_t kOwner = 0xb000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {kEffect + 8u, kObject}, {kEffect + 12u, kOwner},
        {kOwner + 48u, 0xd000u}};
    std::vector<std::string> calls;
    bool clear_on_reset = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kEffect + 4u,
              "state update reads the effect's active flag");
        return 1u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back(address == kEffect ?
            (value == pal4::HookEffect::kVtable ?
                "derived vtable" : "base vtable") : "clear object");
    }
    void ResetParticleFields(std::uint32_t object) override {
        Check(object == kObject, "cleanup resets its current particle");
        calls.emplace_back("reset object");
        if (clear_on_reset) words[kEffect + 8u] = 0u;
    }
    void DestroyParticleObject(std::uint32_t object) override {
        Check(object == kObject, "cleanup destroys the reset particle");
        calls.emplace_back("destroy object");
    }
    std::uint32_t UpdateParticle(
        std::uint32_t object, double seconds,
        std::uint32_t frame, std::uint8_t active) override {
        Check(object == kObject && seconds == 0.25 &&
              frame == 0xd000u && active == 1u,
              "update forwards time, owner frame and active byte");
        calls.emplace_back("update");
        return 0x1234u;
    }
    std::uint32_t HandleParticleStateChange(
        std::uint32_t object, std::uint32_t state) override {
        Check(object == kObject && state == 7u,
              "state change forwards object and event");
        calls.emplace_back("state change");
        return 0x5678u;
    }
};

void TestCreate() {
    FactoryRuntime runtime;
    Check(pal4::HookEffect::Create(0x1000u, 0u, 0x3000u, runtime) == 0u &&
          runtime.calls.empty(), "null clump skips lookup and allocation");
    runtime.user_data = 0u;
    Check(pal4::HookEffect::Create(
              0x1000u, 0x5000u, 0x3000u, runtime) == 0u &&
          runtime.calls == (std::vector<std::string>{"user data"}),
          "clump without hook user data is not allocated");
    runtime.user_data = 0x7200u;
    runtime.allocation = 0u;
    runtime.calls.clear();
    Check(pal4::HookEffect::Create(
              0x1000u, 0x5000u, 0x3000u, runtime) == 0u &&
          runtime.calls == (std::vector<std::string>{
              "user data", "allocate"}),
          "allocation failure returns no hook object");
    runtime.allocation = 0x8000u;
    runtime.initialized = false;
    runtime.calls.clear();
    Check(pal4::HookEffect::Create(
              0x1000u, 0x5000u, 0x3000u, runtime) == 0u &&
          runtime.calls == (std::vector<std::string>{
              "user data", "allocate", "state", "object", "vtable",
              "initialize", "destroy"}),
          "failed initialization destroys a fully constructed object");
    runtime.initialized = true;
    runtime.calls.clear();
    Check(pal4::HookEffect::Create(
              0x1000u, 0x5000u, 0x3000u, runtime) == 0x8000u &&
          runtime.calls == (std::vector<std::string>{
              "user data", "allocate", "state", "object", "vtable",
              "initialize"}),
          "successful factory returns its initialized hook object");
}

void TestTotalTime() {
    TimeRuntime runtime;
    Check(pal4::HookEffect::GetTotalTime(TimeRuntime::kEffect, runtime) ==
              3.75 &&
          runtime.calls == (std::vector<std::string>{
              "active", "capture", "detach", "identity", "restore",
              "update"}),
          "parented frame is detached then restored before time result");
    runtime.words[TimeRuntime::kFrame + 4u] = 0u;
    runtime.calls.clear();
    Check(pal4::HookEffect::GetTotalTime(TimeRuntime::kEffect, runtime) ==
              3.75 &&
          runtime.calls == (std::vector<std::string>{
              "active", "capture", "identity", "restore", "update"}),
          "unparented frame preserves the same time result");
}

void TestConfigGuardsAndFailure() {
    ConfigRuntime runtime;
    Check(!pal4::HookEffect::InitializeFromConfig(
              ConfigRuntime::kEffect, 0u, ConfigRuntime::kFrame,
              ConfigRuntime::kOwner, runtime) &&
          runtime.calls == (std::vector<std::string>{"reset"}),
          "missing config stops after reset");
    runtime.calls.clear();
    runtime.ints["Hook_Tag"] = 1;
    Check(!pal4::HookEffect::InitializeFromConfig(
              ConfigRuntime::kEffect, ConfigRuntime::kConfig,
              ConfigRuntime::kFrame, ConfigRuntime::kOwner, runtime) &&
          runtime.calls == (std::vector<std::string>{
              "reset", "int Hook_Tag"}),
          "nonzero Hook_Tag rejects the configuration");
    runtime.ints.clear();
    runtime.calls.clear();
    runtime.object = 0u;
    Check(!pal4::HookEffect::InitializeFromConfig(
              ConfigRuntime::kEffect, ConfigRuntime::kConfig,
              ConfigRuntime::kFrame, ConfigRuntime::kOwner, runtime) &&
          runtime.calls == (std::vector<std::string>{
              "reset", "int Hook_Tag", "create object"}),
          "particle allocation failure stops before field writes");
    runtime.object = ConfigRuntime::kObject;
    runtime.setup = false;
    runtime.calls.clear();
    Check(!pal4::HookEffect::InitializeFromConfig(
              ConfigRuntime::kEffect, ConfigRuntime::kConfig,
              ConfigRuntime::kFrame, ConfigRuntime::kOwner, runtime) &&
          runtime.words.at(ConfigRuntime::kEffect + 8u) == 0u &&
          runtime.advances == 0u &&
          runtime.calls[runtime.calls.size() - 2u] == "destroy object" &&
          runtime.calls.back() == "destroy texture",
          "failed setup frees object and texture without pre-rendering");
}

void TestConfigFieldsAndResources() {
    ConfigRuntime runtime;
    runtime.ints = {
        {"Attr_Prt_MaxPrt", 64}, {"Attr_Prt_PrtEmit", -2},
        {"Attr_Prt_PrtEmit_Bias", 3},
        {"Attr_Prt_Alpha_AutoGradual", 0},
        {"Attr_Prt_Color_Red", 128},
        {"Attr_Prt_StartCol_Red", 255},
        {"Attr_Prt_ColorGradual", 0},
        {"Attr_Prt_SizeGradual", 0},
        {"Attr_Prt_2DRotateGradual", 1},
        {"Attr_Prt_Is_Billboard", 1},
        {"Attr_Prt_Is_Point", 1},
        {"Attr_Prt_Axis_XZ", 1},
        {"Attr_Prt_Axis_FacePoint", 1},
        {"Attr_Prt_AlphaBlend_off", 1},
        {"Attr_prt_Inherit_Frame", 1},
        {"Attr_Prt_Strip", 1},
        {"Attr_Prt_Strip_Twice", 1},
    };
    runtime.floats = {
        {"Attr_Prt_EmitGap", 1.5f},
        {"Attr_PrtSize_x", 8.0f},
        {"Attr_Prt_InitDir_Bias_z", 2.0f},
        {"Attr_Prt_Pos_Bias_y", -3.0f},
        {"Attr_Prt_Force_x", 9.0f},
        {"Attr_Prt_Force_y", 11.0f},
        {"Attr_Prt_Force_z", 12.0f},
        {"Attr_Prt_StartSize_x", 6.0f},
        {"Attr_Prt_End2DRotateBias", 7.0f},
        {"Attr_Prt_Atomic_Rotate_y", 5.0f},
        {"Attr_PreRender_Time", 2.0f},
    };
    Check(pal4::HookEffect::InitializeFromConfig(
              ConfigRuntime::kEffect, ConfigRuntime::kConfig,
              ConfigRuntime::kFrame, ConfigRuntime::kOwner, runtime),
          "valid configuration initializes the hook");
    const auto object = ConfigRuntime::kObject;
    Check(runtime.words.at(object + 200u) == 64u &&
          runtime.words.at(object + 260u) == static_cast<std::uint32_t>(-2) &&
          runtime.words.at(object + 264u) == 3u &&
          runtime.words.at(object + 124u) == 1u &&
          runtime.words.at(ConfigRuntime::kEffect + 8u) == object &&
          runtime.words.at(ConfigRuntime::kEffect + 12u) ==
              ConfigRuntime::kOwner,
          "integer and owner fields retain the native offsets");
    Check(runtime.bytes.at(object + 2u) == 0u &&
          runtime.bytes.at(object + 4u) == 1u &&
          runtime.bytes.at(object + 53u) == 1u &&
          runtime.bytes.at(object + 54u) == 1u &&
          runtime.bytes.at(object + 316u) == 0u &&
          runtime.bytes.at(object + 384u) == 0u &&
          runtime.bytes.at(object + 420u) == 1u,
          "all seven tested flag fields use their own byte offsets");
    Check(runtime.written_floats.at(object + 268u) == 1.5f &&
          runtime.written_floats.at(object + 228u) == 8.0f &&
          runtime.written_floats.at(object + 232u) == 4.0f &&
          runtime.written_floats.at(object + 312u) == 2.0f &&
          runtime.written_floats.at(object + 80u) == -3.0f &&
          runtime.written_floats.at(object + 240u) == 9.0f &&
          runtime.written_floats.at(object + 388u) == 6.0f &&
          runtime.written_floats.at(object + 436u) == 7.0f &&
          runtime.written_floats.at(object + 92u) == 5.0f,
          "float groups retain override/default values and native offsets");
    const auto int_reads = std::count_if(
        runtime.calls.begin(), runtime.calls.end(),
        [](const std::string& call) { return call.starts_with("int "); });
    const auto float_reads = std::count_if(
        runtime.calls.begin(), runtime.calls.end(),
        [](const std::string& call) { return call.starts_with("float "); });
    Check(int_reads == 36 && float_reads == 41 &&
          std::count(runtime.calls.begin(), runtime.calls.end(),
                     "texture name") == 1 &&
          runtime.written_floats.size() == 58u,
          "configuration reads all 78 attributes and writes 58 floats");
    Check(std::fabs(runtime.written_floats.at(object + 212u) -
                    128.0f / 255.0f) < 0.00001f &&
          runtime.written_floats.at(object + 224u) == 1.0f &&
          runtime.written_floats.at(object + 320u) == 1.0f &&
          runtime.written_floats.at(object + 324u) == 0.0f,
          "initial and gradual colors normalize byte channels");
    Check(!runtime.alpha_blend && !runtime.not_point &&
          runtime.axis_xz && runtime.face_point &&
          runtime.setup_texture == runtime.loaded_texture &&
          runtime.advances == 500u &&
          std::fabs(runtime.last_step - 0.00400000019) < 0.000001 &&
          runtime.calls.back() == "destroy texture" &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "float Attr_Prt_Force_y") != runtime.calls.end() &&
          std::find(runtime.calls.begin(), runtime.calls.end(),
                    "float Attr_Prt_Force_z") != runtime.calls.end(),
          "resource, flags and 500-step prerender follow config values");

    ConfigRuntime cached;
    cached.found_texture = 0x7100u;
    cached.loaded_texture = 0x7200u;
    Check(pal4::HookEffect::InitializeFromConfig(
              ConfigRuntime::kEffect, ConfigRuntime::kConfig,
              ConfigRuntime::kFrame, ConfigRuntime::kOwner, cached) &&
          cached.setup_texture == 0x7100u &&
          cached.advances == 0u &&
          cached.alpha_blend && cached.not_point &&
          cached.bytes.at(object + 2u) == 1u &&
          cached.bytes.at(object + 316u) == 1u &&
          cached.bytes.at(object + 384u) == 1u &&
          std::find(cached.calls.begin(), cached.calls.end(),
                    "load texture") == cached.calls.end(),
          "default config uses cached texture, normal flags and no prerender");
}

void TestLifecycle() {
    LifecycleRuntime runtime;
    runtime.words[LifecycleRuntime::kEffect + 8u] = 0u;
    pal4::HookEffect::CleanupObject(LifecycleRuntime::kEffect, runtime);
    Check(runtime.calls.empty(), "empty hook cleanup skips native object work");
    Check(pal4::HookEffect::UpdateState(
              LifecycleRuntime::kEffect, 0.25, runtime) ==
              LifecycleRuntime::kEffect &&
          !pal4::HookEffect::HandleStateChange(
              LifecycleRuntime::kEffect, 7u, runtime).has_value() &&
          runtime.calls.empty(),
          "empty hook state methods do not call particle services");
    runtime.words[LifecycleRuntime::kEffect + 8u] =
        LifecycleRuntime::kObject;
    Check(pal4::HookEffect::UpdateState(
              LifecycleRuntime::kEffect, 0.25, runtime) == 0x1234u &&
          pal4::HookEffect::HandleStateChange(
              LifecycleRuntime::kEffect, 7u, runtime) == 0x5678u &&
          runtime.calls == (std::vector<std::string>{
              "update", "state change"}),
          "live hook forwards both state entries");
    runtime.calls.clear();
    runtime.clear_on_reset = true;
    pal4::HookEffect::CleanupObject(LifecycleRuntime::kEffect, runtime);
    Check(runtime.calls == (std::vector<std::string>{"reset object"}) &&
          runtime.words.at(LifecycleRuntime::kEffect + 8u) == 0u,
          "cleanup re-reads the object after native reset");

    runtime.words[LifecycleRuntime::kEffect + 8u] =
        LifecycleRuntime::kObject;
    runtime.calls.clear();
    runtime.clear_on_reset = false;
    pal4::HookEffect::Destruct(LifecycleRuntime::kEffect, runtime);
    Check(runtime.calls == (std::vector<std::string>{
              "derived vtable", "reset object", "destroy object",
              "clear object", "base vtable"}) &&
          runtime.words.at(LifecycleRuntime::kEffect) == 0x84b208u &&
          runtime.words.at(LifecycleRuntime::kEffect + 8u) == 0u,
          "derived destructor cleans particle before base vtable transition");
    runtime.calls.clear();
    pal4::HookEffect::DestructBase(LifecycleRuntime::kEffect, runtime);
    Check(runtime.calls == (std::vector<std::string>{"base vtable"}),
          "base destructor writes only its vtable");
}

}  // namespace

int main() {
    TestCreate();
    TestTotalTime();
    TestConfigGuardsAndFailure();
    TestConfigFieldsAndResources();
    TestLifecycle();
}
