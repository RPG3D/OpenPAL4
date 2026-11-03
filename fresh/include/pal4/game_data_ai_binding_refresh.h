#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameDataAiBindingRefreshRuntime {
public:
    virtual ~IGameDataAiBindingRefreshRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t BeginLog(
        std::string_view prefix, std::uint32_t table_name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void InitializeConfigStructures(std::uint32_t object) = 0;
    virtual void InitializePropertyStructures(std::uint32_t object) = 0;
    virtual void BindEffects(std::uint32_t object) = 0;
    virtual void EndLog(std::uint32_t log,
                        std::string_view prefix,
                        std::uint32_t table_name) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseLog(std::uint32_t log) = 0;
};

class GameDataAiBindingRefresh final {
public:
    // IDA 0x4ce370: refresh AiConfig bound fields when counts differ.
    [[nodiscard]] static std::uint32_t RefreshConfig(
        std::uint32_t table, IGameDataAiBindingRefreshRuntime& runtime);
    // IDA 0x4ce5a0: refresh AiProperty bound fields when counts differ.
    [[nodiscard]] static std::uint32_t RefreshProperty(
        std::uint32_t table, IGameDataAiBindingRefreshRuntime& runtime);
};

}  // namespace pal4
