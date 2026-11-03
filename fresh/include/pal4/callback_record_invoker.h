#pragma once

#include <cstdint>

namespace pal4 {

class ICallbackRecordRuntime {
public:
    virtual ~ICallbackRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Invoke(std::uint32_t function,
                                               std::uint32_t argument) = 0;
};

class CallbackRecordInvoker final {
public:
    // IDA 0x4aeb60: function +4, argument +8.
    [[nodiscard]] static std::uint32_t InvokeFunctionFirst(
        std::uint32_t record, ICallbackRecordRuntime& runtime);
    // IDA 0x4aeb70: argument +4, function +8.
    [[nodiscard]] static std::uint32_t InvokeArgumentFirst(
        std::uint32_t record, ICallbackRecordRuntime& runtime);
};

}  // namespace pal4
