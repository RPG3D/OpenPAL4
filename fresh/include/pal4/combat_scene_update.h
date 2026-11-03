#pragma once

#include <cstdint>

namespace pal4 {

// Typed host boundary for the same58AFD0 algorithm. Native owners need no
// fabricated VC6 string globals, ScriptExecutor image or temporary attributes.
class ICombatSceneUpdateServices {
public:
    virtual ~ICombatSceneUpdateServices() = default;
    virtual void AdvanceCombat(float delta) = 0;
    virtual void UpdateUI(double delta) = 0;
    [[nodiscard]] virtual std::uint32_t State() = 0;
    [[nodiscard]] virtual bool Alternate() = 0;
    [[nodiscard]] virtual std::uint32_t Result() = 0;
    virtual void CleanupScriptCache() = 0;
    [[nodiscard]] virtual std::uint32_t ConstructXmlDocument() = 0;
    [[nodiscard]] virtual std::uint32_t DestroyXmlDocument(std::uint32_t) = 0;
    virtual void WriteResult(std::uint32_t document, std::uint32_t value,
                             bool tree_lookup) = 0;
    virtual void Transition(std::uint32_t state, std::uint32_t document) = 0;
};

class ICombatSceneUpdateRuntime {
public:
    virtual ~ICombatSceneUpdateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CombatSystem() = 0;
    virtual void AdvanceCombat(std::uint32_t system, float delta) = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void UpdateUI(std::uint32_t manager, double delta) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructXmlDocument() = 0;
    [[nodiscard]] virtual std::uint32_t DestroyXmlDocument(
        std::uint32_t document) = 0;
    [[nodiscard]] virtual std::uint32_t GetConfigStringValue(
        std::uint32_t document, std::uint32_t key) = 0;
    // IDA58B19D searches the new document's embedded tree at document+20.
    // Return the tree node (attribute at +28), or zero for a miss/end sentinel.
    [[nodiscard]] virtual std::uint32_t FindTreeAttribute(
        std::uint32_t document, std::uint32_t key) = 0;
    virtual void SetXmlAttributeInt(std::uint32_t attribute,
                                    std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructXmlAttribute(
        std::uint32_t key, std::uint32_t value) = 0;
    virtual void AddXmlAttribute(std::uint32_t document,
                                 std::uint32_t attribute) = 0;
    virtual void DestroyXmlAttribute(std::uint32_t attribute) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    virtual void CleanupScriptCache(std::uint32_t cache) = 0;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    virtual void InitializeStateAndUI(std::uint32_t game,
                                      std::uint32_t state,
                                      std::uint32_t document) = 0;
};

class CombatSceneUpdate final {
public:
    static constexpr std::uint32_t kMessageGlobal = 0x8fa238u;
    static constexpr std::uint32_t kEmptyMessage = 0x841964u;

    // IDA 0x58afd0. The remaining two native arguments are unused.
    [[nodiscard]] static std::uint32_t Advance(
        float delta, ICombatSceneUpdateRuntime& runtime);
    [[nodiscard]] static std::uint32_t Advance(
        float delta, ICombatSceneUpdateServices& services);
};

}  // namespace pal4
