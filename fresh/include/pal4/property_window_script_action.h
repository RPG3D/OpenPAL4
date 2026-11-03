#pragma once

#include "pal4/string_buffer.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

enum class EarthballLiteral { ScriptPath, PathSuffix };

class IPropertyWindowScriptRuntime : public IStringBufferRuntime {
public:
    virtual void PlayMusic(std::string_view file, std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t ArenaManager() = 0;
    [[nodiscard]] virtual StringBufferState ReadStringState(
        std::uint32_t address) = 0;
    virtual void InitializeScripts() = 0;
    [[nodiscard]] virtual std::uint32_t LiteralAddress(
        EarthballLiteral literal) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptEngine() = 0;
    [[nodiscard]] virtual bool HasStackModule(std::uint32_t cache,
                                             std::uint32_t full_path) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptMode() = 0;
    [[nodiscard]] virtual std::uint32_t CreateInterpreter(
        std::uint32_t mode, std::uint32_t base_path,
        std::uint32_t full_path, std::string_view function,
        std::uint32_t context, std::uint32_t reader_callback) = 0;
    [[nodiscard]] virtual bool AddScriptModule(std::uint32_t cache,
                                               std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::uint32_t FunctionName(
        std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::uint32_t FullPath(
        std::uint32_t interpreter) = 0;
    virtual void PushStackModule(std::uint32_t cache,
                                 std::uint32_t full_path,
                                 std::uint32_t function_name) = 0;
    virtual void ReleaseInterpreter(std::uint32_t interpreter) = 0;
    virtual void CleanupAllScripts() = 0;
    [[nodiscard]] virtual std::uint32_t UIFrameManager() = 0;
    virtual void ProcessEvents(std::uint32_t event_table,
                               std::uint32_t event_id,
                               const std::array<std::uint32_t, 2>& payload) = 0;
};

class PropertyWindowScriptAction final {
public:
    static constexpr std::uint32_t kEventId = 0x8a80b4u;
    static constexpr std::uint32_t kReadToBufferCallback = 0x600d50u;
    [[nodiscard]] static std::uint32_t ExecuteEarthballSkip(
        IPropertyWindowScriptRuntime& runtime);
};

}  // namespace pal4
