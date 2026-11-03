#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class IGobObstacleConfigRuntime {
public:
    virtual ~IGobObstacleConfigRuntime() = default;
    virtual void CleanupBeforeSceneLoad(std::uint32_t object) = 0;
    [[nodiscard]] virtual bool UsesObjectPackage(std::string_view path) = 0;
    [[nodiscard]] virtual bool UsesWeaponPackage(std::string_view path) = 0;
    virtual void LoadObjectPackage() = 0;
    virtual void LoadWeaponPackage() = 0;
    [[nodiscard]] virtual std::uint32_t CurrentScenePackage() = 0;
    virtual void LoadScenePackage(std::uint32_t package) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeMachineScene(
        std::uint32_t object, std::uint32_t first, std::string_view path,
        std::uint32_t fourth, std::uint32_t fifth) = 0;
    virtual void CleanupScriptPackages() = 0;
    virtual void AttachObstacleHelper(std::uint32_t helper,
                                      std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::uint32_t FindXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
    virtual void RemoveXmlChild(std::uint32_t parent,
                                std::uint32_t child) = 0;
    [[nodiscard]] virtual std::uint32_t CreateXmlChild(
        std::uint32_t parent, std::string_view name) = 0;
    virtual void LoadMachineConfig(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::int32_t ReadConfigInt(
        std::uint32_t node, std::string_view key,
        std::int32_t fallback) = 0;
    [[nodiscard]] virtual float ReadConfigFloat(
        std::uint32_t node, std::string_view key,
        float fallback) = 0;
    [[nodiscard]] virtual std::string ReadConfigString(
        std::uint32_t node, std::string_view key,
        std::string_view fallback) = 0;
    virtual void AssignStringRecord(std::uint32_t record,
                                    std::string_view value) = 0;
    [[nodiscard]] virtual std::uint32_t TimeMilliseconds() = 0;
    [[nodiscard]] virtual float RandomDelay(float minimum, float maximum,
                                            float seed) = 0;
};

}  // namespace pal4
