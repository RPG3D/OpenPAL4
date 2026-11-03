#include "pal4/script_pal_test_result.h"

namespace pal4 {

std::uint32_t ScriptPalTestResult::Get(
    const std::int32_t index, IScriptPalTestResultRuntime& runtime) {
    if (index < 0 || index > 2) return 0u;
    return runtime.ReadWord(runtime.GlobalData() +
                            0x1cu + static_cast<std::uint32_t>(index) * 4u);
}

}  // namespace pal4
