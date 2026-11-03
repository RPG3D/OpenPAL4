#include "pal4/callback_record_invoker.h"

namespace pal4 {

std::uint32_t CallbackRecordInvoker::InvokeFunctionFirst(
    const std::uint32_t record, ICallbackRecordRuntime& runtime) {
    const auto function = runtime.ReadWord(record + 4u);
    const auto argument = runtime.ReadWord(record + 8u);
    return runtime.Invoke(function, argument);
}

std::uint32_t CallbackRecordInvoker::InvokeArgumentFirst(
    const std::uint32_t record, ICallbackRecordRuntime& runtime) {
    const auto function = runtime.ReadWord(record + 8u);
    const auto argument = runtime.ReadWord(record + 4u);
    return runtime.Invoke(function, argument);
}

}  // namespace pal4
