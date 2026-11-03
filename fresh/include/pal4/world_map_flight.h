#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IRenderWareVector2;

class IWorldMapFlightRuntime {
public:
    virtual ~IWorldMapFlightRuntime() = default;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    virtual void SetPosition(std::uint32_t window, float x, float y) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void RemoveFlyMember(std::uint32_t collection,
                                 std::uint32_t name_address) = 0;
    virtual void InitializeScripts() = 0;
    [[nodiscard]] virtual std::uint32_t ScriptMode() = 0;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    [[nodiscard]] virtual std::uint32_t CreateInterpreter(
        std::uint32_t mode, std::string_view object_name,
        std::uint32_t script_name, std::uint32_t full_path,
        std::string_view file, std::uint32_t reader) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptEngine() = 0;
    [[nodiscard]] virtual bool AddScriptModule(std::uint32_t context,
                                       std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::uint32_t FunctionName(
        std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::uint32_t FullPath(
        std::uint32_t interpreter) = 0;
    virtual void PushPreparedModule(std::uint32_t context,
                                       std::uint32_t message,
                                       std::uint32_t function_name) = 0;
    virtual void ReleaseInterpreter(std::uint32_t interpreter) = 0;
    virtual void CallHide(std::uint32_t frame,
                          std::uint32_t function) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupAllScripts() = 0;
};

class WorldMapFlight final {
public:
    static constexpr std::string_view kScriptFile =
        "gamedata\\editData\\script\\worldMap.csb";
    static constexpr std::uint32_t kReader = 0x600d50u;

    [[nodiscard]] static std::uint32_t Advance(
        std::uint32_t frame, std::uint32_t script_name,
        IWorldMapFlightRuntime& runtime, IRenderWareVector2& renderware);
};

}  // namespace pal4
