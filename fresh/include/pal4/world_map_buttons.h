#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IWorldMapButtonsRuntime {
public:
    virtual ~IWorldMapButtonsRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    virtual void InitializeScripts() = 0;
    [[nodiscard]] virtual std::uint32_t ScriptMode() = 0;
    [[nodiscard]] virtual std::uint32_t CreateInterpreter(
        std::uint32_t mode, std::string_view name,
        std::string_view entry, std::string_view full_path,
        std::string_view file, std::uint32_t reader) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptEngine() = 0;
    [[nodiscard]] virtual bool AddScriptModule(std::uint32_t engine_context,
                                       std::uint32_t interpreter) = 0;
    virtual void ExecuteImmediate(std::uint32_t engine,
                                 std::uint32_t interpreter) = 0;
    virtual void ReleaseInterpreter(std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupAllScripts() = 0;
};

class IWorldMapButtonStateRuntime {
public:
    virtual ~IWorldMapButtonStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t MakeUppercaseName(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual std::string_view NameText(
        std::uint32_t name) = 0;
    [[nodiscard]] virtual std::string_view RecordText(
        std::uint32_t record) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupName(
        std::uint32_t name) = 0;
};

class IWorldMapExternalButtonRuntime {
public:
    virtual ~IWorldMapExternalButtonRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::uint32_t name) = 0;
};

class WorldMapButtons final {
public:
    static constexpr std::uint32_t kCount = 21u;
    static constexpr std::uint32_t kReader = 0x600d50u;
    static constexpr std::string_view kScriptFile =
        "gamedata\\editData\\script\\worldMap.csb";

    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t frame, IWorldMapButtonsRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetVisibility(
        std::uint32_t frame, std::uint32_t input,
        std::uint32_t state, IWorldMapButtonStateRuntime& runtime);
    [[nodiscard]] static std::uint32_t DispatchExternalVisibility(
        std::uint32_t input, std::uint32_t state,
        IWorldMapExternalButtonRuntime& external,
        IWorldMapButtonStateRuntime& buttons);
};

}  // namespace pal4
