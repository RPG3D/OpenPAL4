#include "pal4/clump_object_config.h"

#include <bit>

namespace pal4 {
namespace {

constexpr std::string_view kUnsetTexture = "\xCE\xB4\xC9\xE8\xD6\xC3";
constexpr std::string_view kClumpDirectory =
    "gamedata\\effect\\clump\\";
constexpr std::string_view kNormalDirectory =
    "gamedata\\effect\\MatFx\\NorTexture\\";
constexpr std::string_view kReflectionDirectory =
    "gamedata\\effect\\MatFx\\ReflectionTexture\\";
constexpr std::string_view kDualDirectory =
    "gamedata\\effect\\MatFx\\DualTexture\\";

struct Configuration final {
    std::string name;
    std::array<float, 3> position{};
    std::array<float, 3> scale{};
    std::array<float, 3> rotation{};
    std::array<std::uint8_t, 4> color{};
    bool normal = false;
    bool reflection = false;
    bool dual = false;
    std::string normal_texture;
    std::string reflection_texture;
    std::string dual_texture;
    float normal_coefficient = 1.0f;
    float reflection_coefficient = 1.0f;
};

Configuration ReadConfiguration(const std::uint32_t document,
                                IClumpObjectConfigRuntime& runtime) {
    Configuration config{};
    config.name = runtime.GetString(document, "Attr_Clump_FileName",
                                    "101\\101");
    constexpr std::array position_keys{
        "Attr_Clump_Position_x", "Attr_Clump_Position_y",
        "Attr_Clump_Position_z"};
    constexpr std::array scale_keys{
        "Attr_Clump_Scale_x", "Attr_Clump_Scale_y",
        "Attr_Clump_Scale_z"};
    constexpr std::array rotation_keys{
        "Attr_Clump_Rotate_x", "Attr_Clump_Rotate_y",
        "Attr_Clump_Rotate_z"};
    constexpr std::array color_keys{
        "Attr_Clump_Color_Red", "Attr_Clump_Color_Green",
        "Attr_Clump_Color_Blue", "Attr_Clump_Color_Alpha"};
    for (std::size_t i = 0u; i < 3u; ++i)
        config.position[i] = runtime.GetFloat(document, position_keys[i], 0.0f);
    for (std::size_t i = 0u; i < 3u; ++i)
        config.scale[i] = runtime.GetFloat(document, scale_keys[i], 1.0f);
    for (std::size_t i = 0u; i < 3u; ++i)
        config.rotation[i] = runtime.GetFloat(document, rotation_keys[i], 0.0f);
    for (std::size_t i = 0u; i < config.color.size(); ++i)
        config.color[i] = static_cast<std::uint8_t>(
            runtime.GetInt(document, color_keys[i], 255));

    config.normal = runtime.GetInt(document, "Attr_Clump_NorMap_Use", 0) > 0;
    config.normal_texture = std::string(kNormalDirectory) +
        runtime.GetString(document, "Attr_Clump_NorMap_Texture",
                          kUnsetTexture);
    config.normal_coefficient = runtime.GetFloat(
        document, "Attr_Clump_NorMap_Coeff", 1.0f);
    config.reflection = runtime.GetInt(document, "Attr_Clump_RefMap_Use", 0) > 0;
    config.reflection_texture = std::string(kReflectionDirectory) +
        runtime.GetString(document, "Attr_Clump_RefMap_Texture",
                          kUnsetTexture);
    config.reflection_coefficient = runtime.GetFloat(
        document, "ATTR_Clump_RefMap_Coeff", 1.0f);
    config.dual = runtime.GetInt(document, "Attr_Clump_DualMap_Use", 0) > 0;
    config.dual_texture = std::string(kDualDirectory) +
        runtime.GetString(document, "Attr_Clump_DualMap_Texture",
                          kUnsetTexture);
    return config;
}

std::uint32_t PackColor(const std::array<std::uint8_t, 4>& color) {
    return static_cast<std::uint32_t>(color[0u]) |
           (static_cast<std::uint32_t>(color[1u]) << 8u) |
           (static_cast<std::uint32_t>(color[2u]) << 16u) |
           (static_cast<std::uint32_t>(color[3u]) << 24u);
}

void LoadMaterialTextures(const std::uint32_t manager,
                          const Configuration& config,
                          IClumpObjectConfigRuntime& runtime) {
    runtime.CleanupTexture(manager);
    runtime.SetTexture(manager, 0u);
    if (config.dual) {
        runtime.LoadDualTexture(manager, config.dual_texture, 5u, 2u);
    } else if (config.reflection && config.normal) {
        runtime.LoadReflectionAndNormal(
            manager, config.normal_texture, config.reflection_texture,
            0u, 0u, 1u, config.reflection_coefficient,
            config.normal_coefficient);
    } else if (config.reflection) {
        runtime.LoadReflection(manager, config.reflection_texture,
                               config.reflection_coefficient, 0u);
    } else if (config.normal) {
        runtime.LoadNormal(manager, config.normal_texture,
                           config.normal_coefficient);
    }
}

void CacheConfiguration(ClumpObjectState& state,
                        const Configuration& config) {
    state.words[67u] = state.words[17u];
    state.words[68u] = std::bit_cast<std::uint32_t>(config.rotation[0u]);
    state.words[1u] = (state.words[1u] & 0xffffff00u) | 1u;
    state.words[69u] = std::bit_cast<std::uint32_t>(config.rotation[1u]);
    state.words[2u] &= 0x00ffffffu;
    state.words[70u] = std::bit_cast<std::uint32_t>(config.rotation[2u]);
    for (std::size_t i = 0u; i < 3u; ++i)
        state.words[71u + i] = std::bit_cast<std::uint32_t>(config.scale[i]);
    for (std::size_t i = 0u; i < 3u; ++i)
        state.words[74u + i] = std::bit_cast<std::uint32_t>(config.position[i]);
}

}  // namespace

std::uint8_t ClumpObject::LoadConfiguration(
    ClumpObjectState& state, const std::uint32_t document,
    IClumpObjectConfigRuntime& runtime) {
    if (document == 0u)
        return 0u;

    const auto system = runtime.SystemInstance();
    if (runtime.ReadByte(system + 1u) == 0u &&
        runtime.GetInt(document, "Attr_Dumy", 0) > 0) {
        state.words[1u] = (state.words[1u] & 0xffff00ffu) | 0x100u;
        return 1u;
    }

    auto config = ReadConfiguration(document, runtime);
    std::string directory(kClumpDirectory);
    const auto separator = config.name.find_last_of('\\');
    if (separator != std::string::npos && separator > 0u) {
        directory.append(config.name, 0u, separator + 1u);
        config.name.erase(0u, separator + 1u);
    }

    const auto manager = runtime.CreateManager(0x170u);
    state.words[18u] = manager;
    const auto world = runtime.SystemInstance() + 52u;
    if (!runtime.LoadClump(manager, config.name, directory, world,
                           0u, 0u, 1u)) {
        if (manager != 0u) {
            runtime.DestroyManager(manager, 1u);
            state.words[18u] = 0u;
        }
        return 0u;
    }

    const auto animation = runtime.ReadAnimation(
        directory + config.name + ".anm");
    state.words[19u] = animation;
    if (animation != 0u) {
        const auto clump = runtime.ReadWord(manager + 20u);
        const auto hierarchy = runtime.FindHierarchy(clump);
        if (hierarchy == 0u)
            return 0u;
        runtime.WriteWord(hierarchy,
                          runtime.ReadWord(hierarchy) | 0x3000u);
        runtime.AttachHierarchy(hierarchy);
        runtime.SetSkinHierarchy(clump, hierarchy);
        const auto interpolator = runtime.ReadWord(hierarchy + 32u);
        runtime.SetCurrentAnimation(interpolator, animation);
        runtime.SetCurrentTime(interpolator, 0.0f);
        runtime.UpdateHierarchyMatrices(hierarchy);
    }

    runtime.Rotate(manager, config.rotation, 0u);
    runtime.Scale(manager, config.scale, 0u);
    runtime.Translate(manager, config.position, 0u);
    if (config.color[3u] >= 0xfeu)
        config.color[3u] = 0xfeu;
    state.words[17u] = PackColor(config.color);
    runtime.SetColor(manager, state.words[17u]);
    if (animation == 0u)
        LoadMaterialTextures(manager, config, runtime);

    runtime.AssignXml(state, 108u, document);
    CacheConfiguration(state, config);
    return 1u;
}

}  // namespace pal4
