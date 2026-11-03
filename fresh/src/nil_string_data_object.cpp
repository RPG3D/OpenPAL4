#include "pal4/nil_string_data_object.h"

namespace pal4 {

bool NilStringDataObject::InitializeSentinel(
    CallbackCollectionState& collection, const std::uint32_t self_address,
    ICallbackCollectionRuntime& runtime) {
    if (collection.Read(0x24) != 0) return true;

    const auto sentinel = runtime.Allocate(0x10);
    if (sentinel == 0) {
        collection.Write(0x0c, 1);
        return false;
    }

    runtime.WriteDword(sentinel, 0);
    runtime.WriteDword(sentinel + 4, 0);
    runtime.WriteDword(sentinel + 0x0c, self_address);
    std::uint32_t payload = 0;
    try {
        payload = runtime.InvokeFactory(self_address);
    } catch (...) {
        runtime.Free(sentinel);
        throw;
    }
    runtime.WriteDword(sentinel + 8, payload);

    collection.Write(0x24, sentinel);
    collection.Write(0x20, sentinel);
    runtime.WriteDword(sentinel, sentinel);
    runtime.WriteDword(sentinel + 4, sentinel);
    collection.Write(0x1c, 0);
    collection.Write(0x18, 0);
    collection.Write(0x14, 0);
    collection.Write(0x10, 0);
    return true;
}

NilStringDataObject& NilStringDataObject::Initialize(
    const std::uint32_t self_address, INilStringDataRuntime& runtime) {
    for (const auto offset : {0x0cU, 0x10U, 0x14U, 0x18U, 0x1cU, 0x20U,
                              0x24U, 0x28U}) {
        if (offset == 0x28U) {
            trailing_owned_object_ = 0;
        } else {
            collection_.Write(offset, 0);
        }
    }

    collection_.Write(0x00, kAllocateCallback);
    collection_.Write(0x04, kDestroyCallback);
    collection_.Write(0x08, kInitializeBuffersCallback);
    static_cast<void>(InitializeSentinel(collection_, self_address, runtime));
    CallbackCollectionCursor::Clear(collection_, runtime);
    return *this;
}

void NilStringDataObject::Destroy(INilStringDataRuntime& runtime) {
    CallbackCollectionCursor::Destroy(collection_, trailing_owned_object_,
                                      runtime);
}

CallbackCollectionState& NilStringDataObject::Collection() noexcept {
    return collection_;
}

const CallbackCollectionState& NilStringDataObject::Collection() const noexcept {
    return collection_;
}

std::uint32_t NilStringDataObject::TrailingOwnedObject() const noexcept {
    return trailing_owned_object_;
}

void NilStringDataObject::SetTrailingOwnedObject(
    const std::uint32_t address) noexcept {
    trailing_owned_object_ = address;
}

}  // namespace pal4
