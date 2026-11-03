#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class ICallbackInvocationOperations {
public:
    virtual ~ICallbackInvocationOperations() = default;
    [[nodiscard]] virtual std::uint32_t CreateObject() = 0;
    [[nodiscard]] virtual int InvokeObject(std::uint32_t object) = 0;
};

struct CallbackInvocationRecord {
    std::uint32_t field_00 = 0;
    std::uint32_t field_04 = 0;
    std::uint32_t object = 0;
    ICallbackInvocationOperations* operations = nullptr;
};

class CallbackInvocation final {
public:
    [[nodiscard]] static CallbackInvocationRecord& Initialize(
        CallbackInvocationRecord& record,
        ICallbackInvocationOperations& operations);
    [[nodiscard]] static int CallVirtualFunction(
        const CallbackInvocationRecord& record);
};

class ObservedObjectState final {
public:
    static constexpr std::size_t kFieldCount = 11;
    using Fields = std::array<std::uint32_t, kFieldCount>;

    [[nodiscard]] static Fields* Clear(Fields& fields) noexcept;
};

}  // namespace pal4
