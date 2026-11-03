#include "pal4/shop_ui_script.h"

namespace pal4 {

std::uint8_t ShopUiScript::ExecuteTask(
    const std::uint32_t frame, const std::uint32_t,
    IShopUiScriptRuntime& runtime) {
    static_cast<void>(ShopUiActions::Close(frame, runtime));
    const auto context = runtime.CopyTextRecord(
        runtime.GameArenaManager() + 0xd4u);
    std::uint32_t path = 0u;
    std::uint32_t full_name = 0u;
    auto stage = 1u;
    try {
        runtime.InitializeScripts();
        path = runtime.TextFromCString("shop-script");
        stage = 2u;
        full_name = runtime.CopyTextRecord(path);
        stage = 3u;
        runtime.AppendLiteral(full_name, "[");
        auto name = runtime.ReadWord(frame + 0x74u);
        if (name == 0u) name = runtime.EmptyText();
        runtime.AppendName(full_name, name);
        runtime.AppendLiteral(full_name, "]");

        const auto cache = runtime.ScriptExecutor() + 0x54u;
        if (!runtime.CacheContains(cache, runtime.TextData(full_name))) {
            name = runtime.ReadWord(frame + 0x74u);
            if (name == 0u) name = runtime.EmptyText();
            const auto context_data = runtime.TextData(context);
            const auto full_data = runtime.TextData(full_name);
            const auto path_data = runtime.TextData(path);
            const auto mode = runtime.ScriptMode();
            const auto interpreter = runtime.CreateInterpreter(
                mode, path_data, full_data, name, context_data, kReader);
            if (interpreter != 0u &&
                runtime.AddScriptModule(
                    runtime.ScriptExecutor() + 0x54u, interpreter)) {
                const auto stack =
                    runtime.ScriptExecutor() + 0x54u;
                const auto function_name = runtime.FunctionName(interpreter);
                const auto full_path = runtime.FullPath(interpreter);
                runtime.PushStackModule(stack, full_path,
                                        function_name);
                runtime.ReleaseInterpreter(interpreter);
            } else {
                runtime.ShowMissingScriptMessage(
                    "not found script files", "no script", 0u);
            }
        }
        runtime.CleanupAllScripts();
        stage = 2u;
        runtime.CleanupTextRecord(full_name);
        stage = 1u;
        runtime.CleanupTextRecord(path);
        stage = 0u;
        runtime.CleanupTextRecord(context);
    } catch (...) {
        if (stage >= 3u) runtime.CleanupTextRecord(full_name);
        if (stage >= 2u) runtime.CleanupTextRecord(path);
        if (stage >= 1u) runtime.CleanupTextRecord(context);
        throw;
    }
    return 1u;
}

}  // namespace pal4
