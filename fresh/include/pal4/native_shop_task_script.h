#pragma once
#include <cstdint>
#include <functional>
#include <string_view>
namespace pal4 {
class NativePkFileManagerRuntime;
class NativeTimedScriptStack;
class IShopUiActionsRuntime;
struct NativeShopTaskScriptServices {
    NativePkFileManagerRuntime& packages;
    NativeTimedScriptStack& scripts;
    IShopUiActionsRuntime& actions;
    std::uint32_t arena,frame;
    std::string_view arena_script,task_name;
    std::function<void(std::string_view,std::string_view,std::uint32_t)> missing_script;
};
class NativeShopTaskScript final {
public:
    // Call inside DispatchUiScript. Original486570 closes trade first, then
    // queues shop-script[task] from the current arena's bytecode; never executes
    // the task synchronously inside the CEGUI callback.
    static bool Execute(std::uint32_t event,NativeShopTaskScriptServices);
};
}
