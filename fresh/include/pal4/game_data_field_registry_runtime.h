#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

enum class GameDataFieldKind { Integer, Text };

class IGameDataFieldRegistryRuntime {
public:
    virtual ~IGameDataFieldRegistryRuntime() = default;
    virtual void InitializeCollection(std::uint32_t address) = 0;
    virtual void RegisterCleanup(std::uint32_t callback) = 0;
    virtual void BeginTemporaryText() = 0;
    virtual void ReleaseTemporaryText() = 0;
    virtual void AddProperty(GameDataFieldKind kind,
                             std::string_view name,
                             std::uint32_t offset) = 0;
    virtual void Sort(std::uint32_t collection) = 0;
    virtual void CleanupCollection(std::uint32_t collection) = 0;
    [[nodiscard]] virtual std::uint8_t ProcessEvent(
        std::uint32_t input, std::uint32_t collection) = 0;
};

}  // namespace pal4
