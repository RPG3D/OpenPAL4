#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IGameDataParameterBindingRefreshRuntime {
public:
    virtual ~IGameDataParameterBindingRefreshRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t BeginLog(
        std::string_view prefix, std::uint32_t table_name) = 0;
    virtual void AssignEffects(std::uint32_t destination,
                               std::uint32_t source) = 0;
    virtual void InitializeUpgradeStructures(std::uint32_t object) = 0;
    virtual void InitializeDropStructures(std::uint32_t object) = 0;
    virtual void BindEffects(std::uint32_t object) = 0;
    virtual void EndLog(std::uint32_t log,
                        std::string_view prefix,
                        std::uint32_t table_name) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseLog(std::uint32_t log) = 0;
};

class GameDataParameterBindingRefresh final {
public:
    // IDA 0x4d1860: refresh UpgradeParam bound fields when counts differ.
    [[nodiscard]] static std::uint32_t RefreshUpgrade(
        std::uint32_t table,
        IGameDataParameterBindingRefreshRuntime& runtime);
    // IDA 0x4d1a90: refresh DropProperty bound fields when counts differ.
    [[nodiscard]] static std::uint32_t RefreshDrop(
        std::uint32_t table,
        IGameDataParameterBindingRefreshRuntime& runtime);
};

}  // namespace pal4
