#include "pal4/clump_object_config.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IClumpObjectConfigRuntime {
    std::vector<std::string> events;
    std::vector<std::string> getters;
    std::map<std::string, std::int32_t> ints;
    std::map<std::string, float> floats;
    std::map<std::string, std::string> strings;
    std::uint8_t ready = 1u;
    std::uint32_t manager = 0x6000u;
    std::uint32_t animation = 0u;
    std::uint32_t hierarchy = 0u;
    std::uint32_t hierarchy_flags = 0x80u;
    bool clump_loaded = true;
    std::string loaded_name;
    std::string loaded_directory;
    std::string animation_path;
    std::array<float, 3> rotation{};
    std::array<float, 3> scale{};
    std::array<float, 3> position{};
    std::uint32_t color = 0u;
    std::string texture_path;
    float first_coefficient = 0.0f;
    float second_coefficient = 0.0f;

    std::uint32_t SystemInstance() override {
        events.push_back("system");
        return 0x5000u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == 0x5001u, "singleton ready-byte offset");
        events.push_back("ready");
        return ready;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0x6014u) {
            events.push_back("clump-pointer");
            return 0x7000u;
        }
        if (address == 0x8000u) {
            events.push_back("hierarchy-flags");
            return hierarchy_flags;
        }
        Check(address == 0x8020u, "hierarchy interpolator offset");
        events.push_back("interpolator");
        return 0x9000u;
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x8000u, "hierarchy flag write address");
        events.push_back("write-flags");
        hierarchy_flags = value;
    }
    std::int32_t GetInt(std::uint32_t document, std::string_view key,
                        std::int32_t fallback) override {
        Check(document == 0x4000u, "integer reader document");
        getters.push_back("int:" + std::string(key) + ":" +
                          std::to_string(fallback));
        const auto found = ints.find(std::string(key));
        return found == ints.end() ? fallback : found->second;
    }
    float GetFloat(std::uint32_t document, std::string_view key,
                   float fallback) override {
        Check(document == 0x4000u, "float reader document");
        getters.push_back("float:" + std::string(key) + ":" +
                          std::to_string(fallback));
        const auto found = floats.find(std::string(key));
        return found == floats.end() ? fallback : found->second;
    }
    std::string GetString(std::uint32_t document, std::string_view key,
                          std::string_view fallback) override {
        Check(document == 0x4000u, "string reader document");
        getters.push_back("string:" + std::string(key) + ":" +
                          std::string(fallback));
        const auto found = strings.find(std::string(key));
        return found == strings.end() ? std::string(fallback) : found->second;
    }
    std::uint32_t CreateManager(std::uint32_t bytes) override {
        Check(bytes == 0x170u, "native resource manager size");
        events.push_back("create-manager");
        return manager;
    }
    bool LoadClump(std::uint32_t object, std::string_view name,
                   std::string_view directory, std::uint32_t world,
                   std::uint32_t first, std::uint32_t second,
                   std::uint32_t third) override {
        Check(object == manager && world == 0x5034u && first == 0u &&
                  second == 0u && third == 1u,
              "clump load manager, world and native flags");
        events.push_back("load-clump");
        loaded_name = name;
        loaded_directory = directory;
        return clump_loaded;
    }
    void DestroyManager(std::uint32_t object,
                        std::uint32_t flag) override {
        Check(object == manager && flag == 1u,
              "failed clump load deletes manager");
        events.push_back("destroy-manager");
    }
    std::uint32_t ReadAnimation(std::string_view path) override {
        events.push_back("read-animation");
        animation_path = path;
        return animation;
    }
    std::uint32_t FindSkinHierarchy(std::uint32_t) override {
        Check(false,"configuration must traverse frame hierarchy instead of skin bindings");return 0u;
    }
    std::uint32_t FindHierarchy(std::uint32_t clump) override {
        Check(clump == 0x7000u, "hierarchy lookup clump");
        events.push_back("find-hierarchy");
        return hierarchy;
    }
    void AttachHierarchy(std::uint32_t address) override {
        Check(address == hierarchy, "attach hierarchy address");
        events.push_back("attach-hierarchy");
    }
    void SetSkinHierarchy(std::uint32_t clump,
                          std::uint32_t address) override {
        Check(clump == 0x7000u && address == hierarchy,
              "all clump atomics receive hierarchy");
        events.push_back("skin-hierarchy");
    }
    void SetCurrentAnimation(std::uint32_t interpolator,
                             std::uint32_t value) override {
        Check(interpolator == 0x9000u && value == animation,
              "interpolator animation binding");
        events.push_back("set-animation");
    }
    void SetCurrentTime(std::uint32_t interpolator,
                        float time) override {
        Check(interpolator == 0x9000u && time == 0.0f,
              "interpolator starts at zero");
        events.push_back("set-time");
    }
    void UpdateHierarchyMatrices(std::uint32_t address) override {
        Check(address == hierarchy, "hierarchy matrix update address");
        events.push_back("update-matrices");
    }
    void Rotate(std::uint32_t object,
                const std::array<float, 3>& values,
                std::uint32_t mode) override {
        Check(object == manager && mode == 0u, "rotation call arguments");
        events.push_back("rotate");
        rotation = values;
    }
    void Scale(std::uint32_t object,
               const std::array<float, 3>& values,
               std::uint32_t mode) override {
        Check(object == manager && mode == 0u, "scale call arguments");
        events.push_back("scale");
        scale = values;
    }
    void Translate(std::uint32_t object,
                   const std::array<float, 3>& values,
                   std::uint32_t mode) override {
        Check(object == manager && mode == 0u, "translation call arguments");
        events.push_back("translate");
        position = values;
    }
    void SetColor(std::uint32_t object,
                  std::uint32_t rgba) override {
        Check(object == manager, "color manager");
        events.push_back("color");
        color = rgba;
    }
    void CleanupTexture(std::uint32_t object) override {
        Check(object == manager, "texture cleanup manager");
        events.push_back("cleanup-texture");
    }
    void SetTexture(std::uint32_t object,
                    std::uint32_t texture) override {
        Check(object == manager && texture == 0u,
              "texture reset arguments");
        events.push_back("clear-texture");
    }
    void LoadDualTexture(std::uint32_t object,
                         std::string_view path,
                         std::uint32_t first,
                         std::uint32_t second) override {
        Check(object == manager && first == 5u && second == 2u,
              "dual texture native flags");
        events.push_back("dual-texture");
        texture_path = path;
    }
    void LoadReflectionAndNormal(
        std::uint32_t object, std::string_view normal,
        std::string_view reflection, std::uint32_t first,
        std::uint32_t second, std::uint32_t third,
        float reflection_coefficient,
        float normal_coefficient) override {
        Check(object == manager && first == 0u && second == 0u &&
                  third == 1u,
              "combined texture native flags");
        events.push_back("reflection-normal");
        texture_path = std::string(normal) + "|" +
                       std::string(reflection);
        first_coefficient = reflection_coefficient;
        second_coefficient = normal_coefficient;
    }
    void LoadReflection(std::uint32_t object,
                        std::string_view path, float coefficient,
                        std::uint32_t flag) override {
        Check(object == manager && flag == 0u,
              "reflection texture native flag");
        events.push_back("reflection-texture");
        texture_path = path;
        first_coefficient = coefficient;
    }
    void LoadNormal(std::uint32_t object,
                    std::string_view path,
                    float coefficient) override {
        Check(object == manager, "normal texture manager");
        events.push_back("normal-texture");
        texture_path = path;
        first_coefficient = coefficient;
    }
    void AssignXml(pal4::ClumpObjectState& state,
                   std::uint32_t offset,
                   std::uint32_t document) override {
        Check(offset == 108u && document == 0x4000u &&
                  state.words[18u] == manager &&
                  (state.words[1u] & 0xffu) == 0u,
              "XML assignment precedes active flag");
        events.push_back("assign-xml");
    }
};

void TestEarlyReturns() {
    pal4::ClumpObjectState state{};
    Runtime runtime;
    Check(pal4::ClumpObject::LoadConfiguration(state, 0u, runtime) == 0u &&
              runtime.events.empty() && runtime.getters.empty(),
          "null document returns before singleton access");
    runtime.ready = 0u;
    runtime.ints["Attr_Dumy"] = 1;
    state.words[1u] = 0xa5a50000u;
    Check(pal4::ClumpObject::LoadConfiguration(
              state, 0x4000u, runtime) == 1u &&
              state.words[1u] == 0xa5a50100u &&
              runtime.events == std::vector<std::string>{"system", "ready"} &&
              runtime.getters ==
                  std::vector<std::string>{"int:Attr_Dumy:0"},
          "dummy shortcut sets byte +5 without allocating resources");

    Runtime normal_load;
    normal_load.ready = 0u;
    normal_load.ints["Attr_Dumy"] = -1;
    normal_load.clump_loaded = false;
    Check(pal4::ClumpObject::LoadConfiguration(
              state, 0x4000u, normal_load) == 0u &&
              normal_load.getters.front() == "int:Attr_Dumy:0" &&
              normal_load.getters[1u] ==
                  "string:Attr_Clump_FileName:101\\101" &&
              normal_load.events.back() == "destroy-manager",
          "nonpositive dummy value continues into normal load");
}

void TestDefaultLoad() {
    pal4::ClumpObjectState state{};
    Runtime runtime;
    Check(pal4::ClumpObject::LoadConfiguration(
              state, 0x4000u, runtime) == 1u,
          "default clump load succeeds");
    Check(runtime.loaded_name == "101" &&
              runtime.loaded_directory ==
                  "gamedata\\effect\\clump\\101\\" &&
              runtime.animation_path ==
                  "gamedata\\effect\\clump\\101\\101.anm",
          "backslash path is split and animation path is appended");
    Check(runtime.events == std::vector<std::string>{
              "system", "ready", "create-manager", "system",
              "load-clump", "read-animation", "rotate", "scale",
              "translate", "color", "cleanup-texture", "clear-texture",
              "assign-xml"},
          "default load follows native service order");
    Check(runtime.rotation == std::array<float, 3>{0.0f, 0.0f, 0.0f} &&
              runtime.scale == std::array<float, 3>{1.0f, 1.0f, 1.0f} &&
              runtime.position == std::array<float, 3>{0.0f, 0.0f, 0.0f} &&
              runtime.color == 0xfeffffffu,
          "default transforms and alpha clamp");
    Check(state.words[18u] == 0x6000u && state.words[19u] == 0u &&
              state.words[17u] == 0xfeffffffu &&
              state.words[67u] == 0xfeffffffu &&
              state.words[68u] == 0u &&
              state.words[71u] == std::bit_cast<std::uint32_t>(1.0f) &&
              state.words[74u] == 0u &&
              (state.words[1u] & 0xffu) == 1u &&
              (state.words[2u] >> 24u) == 0u,
          "successful load caches fields at native offsets");
    constexpr std::string_view unset = "\xCE\xB4\xC9\xE8\xD6\xC3";
    const std::vector<std::string> expected_getters{
        "string:Attr_Clump_FileName:101\\101",
        "float:Attr_Clump_Position_x:0.000000",
        "float:Attr_Clump_Position_y:0.000000",
        "float:Attr_Clump_Position_z:0.000000",
        "float:Attr_Clump_Scale_x:1.000000",
        "float:Attr_Clump_Scale_y:1.000000",
        "float:Attr_Clump_Scale_z:1.000000",
        "float:Attr_Clump_Rotate_x:0.000000",
        "float:Attr_Clump_Rotate_y:0.000000",
        "float:Attr_Clump_Rotate_z:0.000000",
        "int:Attr_Clump_Color_Red:255",
        "int:Attr_Clump_Color_Green:255",
        "int:Attr_Clump_Color_Blue:255",
        "int:Attr_Clump_Color_Alpha:255",
        "int:Attr_Clump_NorMap_Use:0",
        "string:Attr_Clump_NorMap_Texture:" + std::string(unset),
        "float:Attr_Clump_NorMap_Coeff:1.000000",
        "int:Attr_Clump_RefMap_Use:0",
        "string:Attr_Clump_RefMap_Texture:" + std::string(unset),
        "float:ATTR_Clump_RefMap_Coeff:1.000000",
        "int:Attr_Clump_DualMap_Use:0",
        "string:Attr_Clump_DualMap_Texture:" + std::string(unset)};
    Check(runtime.getters == expected_getters,
          "configuration keys, defaults, types and order match IDA");
}

void TestLoadFailures() {
    pal4::ClumpObjectState state{};
    Runtime runtime;
    runtime.clump_loaded = false;
    Check(pal4::ClumpObject::LoadConfiguration(
              state, 0x4000u, runtime) == 0u &&
              state.words[18u] == 0u && state.words[19u] == 0u &&
              runtime.events.back() == "destroy-manager",
          "failed clump load destroys allocated manager");

    runtime = Runtime{};
    state = {};
    runtime.animation = 0x7100u;
    Check(pal4::ClumpObject::LoadConfiguration(
              state, 0x4000u, runtime) == 0u &&
              state.words[18u] == 0x6000u &&
              state.words[19u] == 0x7100u &&
              runtime.events.back() == "find-hierarchy" &&
              (state.words[1u] & 0xffu) == 0u,
          "animation without hierarchy returns with native owned pointers");
}

void TestAnimation() {
    pal4::ClumpObjectState state{};
    Runtime runtime;
    runtime.animation = 0x7100u;
    runtime.hierarchy = 0x8000u;
    runtime.strings["Attr_Clump_FileName"] = "single";
    Check(pal4::ClumpObject::LoadConfiguration(
              state, 0x4000u, runtime) == 1u &&
              runtime.loaded_name == "single" &&
              runtime.loaded_directory == "gamedata\\effect\\clump\\" &&
              runtime.animation_path ==
                  "gamedata\\effect\\clump\\single.anm",
          "animation load preserves unsplit filename");
    Check(runtime.hierarchy_flags == 0x3080u &&
              runtime.events == std::vector<std::string>{
                  "system", "ready", "create-manager", "system",
                  "load-clump", "read-animation", "clump-pointer",
                  "find-hierarchy", "hierarchy-flags", "write-flags",
                  "attach-hierarchy", "skin-hierarchy", "interpolator",
                  "set-animation", "set-time", "update-matrices",
                  "rotate", "scale", "translate", "color", "assign-xml"},
          "HAnim binding precedes transforms and skips material textures");
    Check(state.words[19u] == 0x7100u &&
              (state.words[1u] & 0xffu) == 1u,
          "animation pointer and active byte persist");
}

void TestTextureBranches() {
    constexpr std::string_view normal_path =
        "gamedata\\effect\\MatFx\\NorTexture\\n.dds";
    constexpr std::string_view reflection_path =
        "gamedata\\effect\\MatFx\\ReflectionTexture\\r.dds";
    constexpr std::string_view dual_path =
        "gamedata\\effect\\MatFx\\DualTexture\\d.dds";
    for (unsigned mode = 0u; mode < 4u; ++mode) {
        pal4::ClumpObjectState state{};
        Runtime runtime;
        runtime.strings["Attr_Clump_FileName"] = "folder\\asset";
        runtime.strings["Attr_Clump_NorMap_Texture"] = "n.dds";
        runtime.strings["Attr_Clump_RefMap_Texture"] = "r.dds";
        runtime.strings["Attr_Clump_DualMap_Texture"] = "d.dds";
        runtime.floats["Attr_Clump_NorMap_Coeff"] = 0.25f;
        runtime.floats["ATTR_Clump_RefMap_Coeff"] = 0.75f;
        runtime.floats["Attr_Clump_Rotate_x"] = 10.0f;
        runtime.floats["Attr_Clump_Scale_y"] = 2.0f;
        runtime.floats["Attr_Clump_Position_z"] = -3.0f;
        runtime.ints["Attr_Clump_Color_Red"] = 0x123;
        runtime.ints["Attr_Clump_Color_Alpha"] = 0xfd;
        runtime.ints["Attr_Clump_NorMap_Use"] = mode == 0u || mode == 1u ||
                                                     mode == 3u ? 1 : 0;
        runtime.ints["Attr_Clump_RefMap_Use"] = mode == 1u || mode == 2u ? 1 : 0;
        runtime.ints["Attr_Clump_DualMap_Use"] = mode == 0u ? 1 : 0;
        Check(pal4::ClumpObject::LoadConfiguration(
                  state, 0x4000u, runtime) == 1u &&
                  runtime.loaded_name == "asset" &&
                  runtime.loaded_directory ==
                      "gamedata\\effect\\clump\\folder\\" &&
                  runtime.rotation[0u] == 10.0f &&
                  runtime.scale[1u] == 2.0f &&
                  runtime.position[2u] == -3.0f &&
                  runtime.color == 0xfdffff23u &&
                  state.words[67u] == 0xfdffff23u &&
                  state.words[68u] == std::bit_cast<std::uint32_t>(10.0f) &&
                  state.words[72u] == std::bit_cast<std::uint32_t>(2.0f) &&
                  state.words[76u] == std::bit_cast<std::uint32_t>(-3.0f),
              "custom transform, color truncation and cache offsets");
        const auto& call = runtime.events[runtime.events.size() - 2u];
        if (mode == 0u)
            Check(call == "dual-texture" &&
                      runtime.texture_path == dual_path,
                  "dual map takes priority over all other texture flags");
        else if (mode == 1u)
            Check(call == "reflection-normal" &&
                      runtime.texture_path ==
                          std::string(normal_path) + "|" +
                          std::string(reflection_path) &&
                      runtime.first_coefficient == 0.75f &&
                      runtime.second_coefficient == 0.25f,
                  "combined textures preserve path and coefficient order");
        else if (mode == 2u)
            Check(call == "reflection-texture" &&
                      runtime.texture_path == reflection_path &&
                      runtime.first_coefficient == 0.75f,
                  "reflection only path");
        else
            Check(call == "normal-texture" &&
                      runtime.texture_path == normal_path &&
                      runtime.first_coefficient == 0.25f,
                  "normal only path");
    }
}

}  // namespace

int main() {
    TestEarlyReturns();
    TestDefaultLoad();
    TestLoadFailures();
    TestAnimation();
    TestTextureBranches();
}
