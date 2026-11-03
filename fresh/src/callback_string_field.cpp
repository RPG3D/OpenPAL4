#include "pal4/callback_string_field.h"

namespace pal4 {

int CallbackStringField::Set(CallbackState::Storage& object,
                             const std::uint32_t source,
                             ICallbackStringRuntime& runtime) {
    const auto old_address = CallbackState::ReadDword(object, 0x1c);
    if (old_address != 0) runtime.Free(old_address);

    const auto source_length = runtime.Length(source);
    const auto allocation_size = source_length + std::uint32_t{1};
    const auto destination = runtime.Allocate(allocation_size);
    CallbackState::WriteDword(object, 0x1c, destination);

    runtime.CopyAnsiString(destination, source);
    const auto terminator_address =
        source_length + CallbackState::ReadDword(object, 0x1c);
    runtime.WriteByte(terminator_address, 0);

    return static_cast<int>(runtime.Length(
        CallbackState::ReadDword(object, 0x1c)));
}

}  // namespace pal4
