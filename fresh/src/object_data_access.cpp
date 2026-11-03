#include "pal4/object_data_access.h"

namespace pal4 {

std::int32_t ObjectMethodRuntimeStub::CallVirtual(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0; }

std::int32_t ObjectMethodForwarder::CallSlot10(
    std::uint32_t context, std::uint32_t receiver,
    IObjectMethodRuntime& runtime) {
    return runtime.CallVirtual(receiver, 0x10u, context);
}

std::int32_t ObjectMethodForwarder::CallSlot28(
    std::uint32_t context, std::uint32_t receiver,
    IObjectMethodRuntime& runtime) {
    return runtime.CallVirtual(receiver, 0x28u, context);
}

void IndexedDataRecord::ResetVtable() {
    vtable = 0x842bd4u;
}

std::uint32_t IndexedDataRecord::GetElement(std::uint32_t index) const {
    return index < values.size() ? values[index] : index;
}

}  // namespace pal4
