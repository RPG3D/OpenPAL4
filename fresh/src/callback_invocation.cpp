#include "pal4/callback_invocation.h"

#include <array>
#include <cstddef>

namespace pal4 {

CallbackInvocationRecord& CallbackInvocation::Initialize(
    CallbackInvocationRecord& record,
    ICallbackInvocationOperations& operations) {
    record.field_00 = 0;
    record.field_04 = 0;
    record.operations = &operations;
    record.object = operations.CreateObject();
    return record;
}

int CallbackInvocation::CallVirtualFunction(
    const CallbackInvocationRecord& record) {
    return record.operations->InvokeObject(record.object);
}

ObservedObjectState::Fields* ObservedObjectState::Clear(
    Fields& fields) noexcept {
    constexpr std::array<std::size_t, 8> kClearedFields{
        3, 9, 8, 7, 6, 5, 4, 10};
    for (const auto index : kClearedFields) fields[index] = 0;
    return &fields;
}

}  // namespace pal4
