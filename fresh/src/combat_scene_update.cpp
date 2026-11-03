#include "pal4/combat_scene_update.h"

namespace pal4 {
namespace {
void AddAttribute(std::uint32_t document, std::uint32_t key,
                  std::uint32_t value, ICombatSceneUpdateRuntime& runtime) {
    const auto attribute = runtime.ConstructXmlAttribute(key, value);
    try { runtime.AddXmlAttribute(document, attribute); }
    catch (...) { runtime.DestroyXmlAttribute(attribute); throw; }
    runtime.DestroyXmlAttribute(attribute);
}

// Preserve the original facade's acquisition count, key fallback, attribute
// node/field distinction and incidental document-destructor return value.
class LegacyServices final : public ICombatSceneUpdateServices {
    ICombatSceneUpdateRuntime& runtime;
public:
    explicit LegacyServices(ICombatSceneUpdateRuntime& r) : runtime(r) {}
    void AdvanceCombat(float delta) override {
        runtime.AdvanceCombat(runtime.CombatSystem(), delta);
    }
    void UpdateUI(double delta) override {
        runtime.UpdateUI(runtime.UIFrameManager(), delta);
    }
    std::uint32_t State() override {
        return runtime.ReadWord(runtime.CombatSystem() + 508u);
    }
    bool Alternate() override {
        return runtime.ReadByte(runtime.CombatSystem() + 500u) != 0u;
    }
    std::uint32_t Result() override {
        return runtime.ReadWord(runtime.CombatSystem() + 504u);
    }
    void CleanupScriptCache() override {
        runtime.CleanupScriptCache(runtime.ScriptExecutor() + 0x54u);
    }
    std::uint32_t ConstructXmlDocument() override { return runtime.ConstructXmlDocument(); }
    std::uint32_t DestroyXmlDocument(std::uint32_t d) override { return runtime.DestroyXmlDocument(d); }
    void WriteResult(std::uint32_t document, std::uint32_t value, bool tree) override {
        auto key = runtime.ReadWord(CombatSceneUpdate::kMessageGlobal);
        if (!key) key = CombatSceneUpdate::kEmptyMessage;
        auto attribute = tree ? runtime.FindTreeAttribute(document, key)
                              : runtime.GetConfigStringValue(document, key);
        if (attribute) runtime.SetXmlAttributeInt(attribute + (tree ? 28u : 0u), value);
        else AddAttribute(document, key, value, runtime);
    }
    void Transition(std::uint32_t state, std::uint32_t document) override {
        runtime.InitializeStateAndUI(runtime.GameInstance(), state, document);
    }
};
}  // namespace

std::uint32_t CombatSceneUpdate::Advance(float delta, ICombatSceneUpdateRuntime& runtime) {
    LegacyServices services(runtime);
    return Advance(delta, services);
}

std::uint32_t CombatSceneUpdate::Advance(float delta, ICombatSceneUpdateServices& services) {
    services.AdvanceCombat(delta);
    services.UpdateUI(static_cast<double>(delta));
    const auto state = services.State();
    if (state != 7u) return state;
    const bool alternate = services.Alternate();
    const auto alternate_result = alternate ? services.Result() : 0u;
    if (alternate && !alternate_result) services.CleanupScriptCache();
    const auto document = services.ConstructXmlDocument();
    try {
        const auto next_state = alternate && !alternate_result ? 7u : 1u;
        if (next_state == 1u)
            services.WriteResult(document, services.Result() != 0u ? 1u : 0u, alternate);
        services.Transition(next_state, document);
    } catch (...) {
        (void)services.DestroyXmlDocument(document);
        throw;
    }
    return services.DestroyXmlDocument(document);
}
}  // namespace pal4
