#include "pal4/string_buffer_pair_object.h"

namespace pal4 {

void StringBufferPairObject::InitializeAt(
    const std::uint32_t self_address, const std::uint32_t source_record,
    IStringBufferRuntime& runtime) {
    const auto first_length = runtime.ReadDword(source_record + 4);
    const auto first_data = runtime.ReadDword(source_record);
    static_cast<void>(StringBuffer::InitializeAt(
        self_address, first_data, first_length, runtime));
    const auto second_length = runtime.ReadDword(source_record + 0x10);
    const auto second_data = runtime.ReadDword(source_record + 0x0c);
    static_cast<void>(StringBuffer::InitializeAt(
        self_address + 0x0c, second_data, second_length, runtime));
}

void StringBufferPairObject::CleanupAt(
    const std::uint32_t self_address, IStringBufferRuntime& runtime) {
    try {
        static_cast<void>(StringBuffer::CleanupAt(self_address + 0x0c, runtime));
    } catch (...) {
        static_cast<void>(StringBuffer::CleanupAt(self_address, runtime));
        throw;
    }
    static_cast<void>(StringBuffer::CleanupAt(self_address, runtime));
}

void StringBufferPairObject::DestroyObject(
    const std::uint32_t object_address, IStringBufferRuntime& runtime) {
    if (object_address == 0) return;
    CleanupAt(object_address, runtime);
    static_cast<void>(runtime.Free(object_address));
}

std::uint32_t StringBufferPairObject::AllocateAndInitializeObjects(
    IStringBufferRuntime& runtime) {
    const auto object = runtime.Allocate(0x18);
    if (object == 0) return 0;

    try {
        static_cast<void>(StringBuffer::InitializeEmptyAt(object, runtime));
    } catch (...) {
        static_cast<void>(runtime.Free(object));
        throw;
    }
    try {
        static_cast<void>(StringBuffer::InitializeEmptyAt(object + 0x0c, runtime));
    } catch (...) {
        static_cast<void>(StringBuffer::CleanupAt(object, runtime));
        static_cast<void>(runtime.Free(object));
        throw;
    }
    return object;
}

}  // namespace pal4
