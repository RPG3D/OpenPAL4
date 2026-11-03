#pragma once

#include <cstdint>

namespace pal4 {

class IScriptPlayerLevelRuntime {
public:
    virtual ~IScriptPlayerLevelRuntime() = default;
    [[nodiscard]] virtual std::uint32_t PlayerManager() = 0;
    [[nodiscard]] virtual std::uint32_t PlayerByIndex(
        std::uint32_t manager, std::int32_t index) = 0;
    [[nodiscard]] virtual std::int32_t ReadStat(
        std::uint32_t component, std::uint32_t stat) = 0;
};

class ScriptPlayerLevelQuery final {
public:
    // IDA 0x5d5560, giGetPlayerLevel.
    [[nodiscard]] static std::int32_t Get(
        std::int32_t index, IScriptPlayerLevelRuntime& runtime);
};

class IScriptLogoNavigationRuntime {
public:
    virtual ~IScriptLogoNavigationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GameInstance() = 0;
    [[nodiscard]] virtual std::uint32_t EnterState(
        std::uint32_t game, std::uint32_t state,
        std::uint32_t argument) = 0;
};

class ScriptLogoNavigation final {
public:
    // IDA 0x5d5590, giGotoLogo.
    [[nodiscard]] static std::uint32_t Go(
        IScriptLogoNavigationRuntime& runtime);
};

class IScriptCameraDistanceRuntime {
public:
    virtual ~IScriptCameraDistanceRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SetDistanceOptimization(
        std::uint32_t enabled) = 0;
};

class ScriptCameraDistanceOptimization final {
public:
    // IDA 0x5d5620, giCameraSetDistOptEnable.
    [[nodiscard]] static std::uint32_t Set(
        std::uint32_t enabled, IScriptCameraDistanceRuntime& runtime);
};

}  // namespace pal4
