#pragma once

#include "pal4/shop_ui_actions.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IShopUiScriptRuntime : public IShopUiActionsRuntime {
public:
    ~IShopUiScriptRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t GameArenaManager() = 0;
    [[nodiscard]] virtual std::uint32_t CopyTextRecord(
        std::uint32_t source) = 0;
    virtual void InitializeScripts() = 0;
    [[nodiscard]] virtual std::uint32_t TextFromCString(
        std::string_view text) = 0;
    virtual void AppendLiteral(std::uint32_t text,
                               std::string_view literal) = 0;
    virtual void AppendName(std::uint32_t text,
                            std::uint32_t name) = 0;
    [[nodiscard]] virtual std::uint32_t TextData(
        std::uint32_t text) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyText() = 0;
    [[nodiscard]] virtual std::uint32_t ScriptExecutor() = 0;
    [[nodiscard]] virtual bool CacheContains(
        std::uint32_t cache, std::uint32_t full_name) = 0;
    [[nodiscard]] virtual std::uint32_t ScriptMode() = 0;
    [[nodiscard]] virtual std::uint32_t CreateInterpreter(
        std::uint32_t mode, std::uint32_t path,
        std::uint32_t full_name, std::uint32_t script_name,
        std::uint32_t context, std::uint32_t reader) = 0;
    [[nodiscard]] virtual bool AddScriptModule(
        std::uint32_t cache, std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::uint32_t FunctionName(
        std::uint32_t interpreter) = 0;
    [[nodiscard]] virtual std::uint32_t FullPath(
        std::uint32_t interpreter) = 0;
    virtual void PushStackModule(std::uint32_t cache,
                                 std::uint32_t full_path,
                                 std::uint32_t function_name) = 0;
    virtual void ReleaseInterpreter(std::uint32_t interpreter) = 0;
    virtual void ShowMissingScriptMessage(
        std::string_view message, std::string_view title,
        std::uint32_t flags) = 0;
    virtual void CleanupAllScripts() = 0;
    virtual void CleanupTextRecord(std::uint32_t text) = 0;
};

class ShopUiScript final {
public:
    static constexpr std::uint32_t kReader = 0x600d50u;

    // IDA 0x486570.
    [[nodiscard]] static std::uint8_t ExecuteTask(
        std::uint32_t frame, std::uint32_t event,
        IShopUiScriptRuntime& runtime);
};

}  // namespace pal4
