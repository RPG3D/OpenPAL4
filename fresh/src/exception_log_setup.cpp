#include "pal4/exception_log_setup.h"

namespace pal4 {

std::uint32_t ExceptionLogSetup::Install(
    const char* path, ExceptionLog& log, IExceptionLogRuntime& log_runtime,
    IExceptionLogSetupRuntime& runtime) {
    (void)log.Create(path, log_runtime);

    const std::uint32_t executable = runtime.GetModuleHandle(nullptr);
    if (runtime.AddModule(executable)) {
        const std::uint32_t dbghelp = runtime.GetModuleHandle("DBGHELP.DLL");
        if (runtime.AddModule(dbghelp)) {
            const std::uint32_t kernel32 = runtime.GetModuleHandle("KERNEL32.DLL");
            (void)runtime.AddModule(kernel32);
        }
    }
    return runtime.SetSeTranslator(kGlobalHandlerAddress);
}

}  // namespace pal4
