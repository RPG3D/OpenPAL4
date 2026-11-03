#include "pal4/script_event_system.h"

#include <cstddef>

namespace pal4 {
namespace {

void WriteDword(ScriptEventObjectImage& image, std::uint32_t offset,
                std::uint32_t value) {
    for (std::uint32_t i = 0; i < 4; ++i) {
        image.bytes[offset + i] = static_cast<std::uint8_t>(value >> (i * 8));
        image.written[offset + i] = true;
    }
}

void BindAdapter(IScriptEventSystemRuntime& runtime, std::uint32_t object,
                 std::uint32_t member_offset, std::uint32_t callback,
                 std::uint32_t index) {
    const auto address = runtime.Allocate(12u);
    if (address != 0u)
        runtime.StoreAdapter(address, ScriptEventSystem::MakeAdapter(
            callback, index));
    bool owned = address != 0u;
    try {
        runtime.AssignAdapter(object, member_offset, address, owned);
    } catch (...) {
        runtime.ReleaseTemporaryAdapter(address, owned);
        throw;
    }
    runtime.ReleaseTemporaryAdapter(address, owned);
}

}  // namespace

std::uint32_t ScriptEventObjectImage::Dword(std::uint32_t offset) const {
    std::uint32_t value = 0u;
    for (std::uint32_t i = 0; i < 4; ++i)
        value |= static_cast<std::uint32_t>(bytes[offset + i]) << (i * 8);
    return value;
}

void ScriptEventSystemRuntimeStub::RegisterAtExit(std::uint32_t) {}
std::uint32_t ScriptEventSystemRuntimeStub::Allocate(std::uint32_t) {
    return 0u;
}
void ScriptEventSystemRuntimeStub::Free(std::uint32_t) {}
void ScriptEventSystemRuntimeStub::StoreObject(
    std::uint32_t, const ScriptEventObjectImage&) {}
void ScriptEventSystemRuntimeStub::AssignName(
    std::uint32_t, std::uint32_t, bool) {}
void ScriptEventSystemRuntimeStub::InitializeErrorString(
    StringBufferState&, const char*, std::uint8_t) {}
void ScriptEventSystemRuntimeStub::StoreAdapter(
    std::uint32_t, const ScriptIndexedAdapterImage&) {}
void ScriptEventSystemRuntimeStub::AssignAdapter(
    std::uint32_t, std::uint32_t, std::uint32_t, bool& owned) {
    owned = false;
}
void ScriptEventSystemRuntimeStub::ReleaseTemporaryAdapter(
    std::uint32_t, bool) {}
void ScriptEventSystemRuntimeStub::PushBack(std::uint32_t) {}
void ScriptEventSystemRuntimeStub::Sort() {}
void ScriptEventSystemRuntimeStub::CleanupCollection(std::uint32_t) {}
std::uint8_t ScriptEventErrorCleanupRuntimeStub::ReadByte(
    std::uint32_t) { return 0u; }
void ScriptEventErrorCleanupRuntimeStub::WriteByte(
    std::uint32_t, std::uint8_t) {}
void ScriptEventErrorCleanupRuntimeStub::Free(std::uint32_t) {}
void ScriptIndexedAdapterLifecycleRuntimeStub::Free(
    std::uint32_t, const ScriptIndexedAdapterImage&) {}

void ScriptIndexedAdapterLifecycle::ResetReadBase(
    ScriptIndexedAdapterImage& image) {
    image.vtable = kReadBaseVtable;
}

void ScriptIndexedAdapterLifecycle::ResetWriteBase(
    ScriptIndexedAdapterImage& image) {
    image.vtable = kWriteBaseVtable;
}

std::uint32_t ScriptIndexedAdapterLifecycle::DeleteRead(
    ScriptIndexedAdapterImage& image, std::uint32_t object_address,
    std::uint8_t flags, IScriptIndexedAdapterLifecycleRuntime& runtime) {
    ResetReadBase(image);
    if ((flags & 1u) != 0u)
        runtime.Free(object_address, image);
    return object_address;
}

std::uint32_t ScriptIndexedAdapterLifecycle::DeleteWrite(
    ScriptIndexedAdapterImage& image, std::uint32_t object_address,
    std::uint8_t flags, IScriptIndexedAdapterLifecycleRuntime& runtime) {
    ResetWriteBase(image);
    if ((flags & 1u) != 0u)
        runtime.Free(object_address, image);
    return object_address;
}

std::uint32_t ScriptIndexedAdapterLifecycle::DeleteReadBase(
    ScriptIndexedAdapterImage& image, std::uint32_t object_address,
    std::uint8_t flags, IScriptIndexedAdapterLifecycleRuntime& runtime) {
    image.vtable = kReadBaseVtable;
    if ((flags & 1u) != 0u)
        runtime.Free(object_address, image);
    return object_address;
}

std::uint32_t ScriptIndexedAdapterLifecycle::DeleteWriteBase(
    ScriptIndexedAdapterImage& image, std::uint32_t object_address,
    std::uint8_t flags, IScriptIndexedAdapterLifecycleRuntime& runtime) {
    image.vtable = kWriteBaseVtable;
    if ((flags & 1u) != 0u)
        runtime.Free(object_address, image);
    return object_address;
}

ScriptEventObjectImage ScriptEventSystem::MakeObject(
    std::uint8_t stack_byte) {
    ScriptEventObjectImage image{};
    image.base_vtable = 0x842bbcu;
    image.final_vtable = 0x842ba4u;
    WriteDword(image, 0u, image.final_vtable);
    image.bytes[4] = 0u;
    image.written[4] = true;
    WriteDword(image, 8u, 0u);
    image.bytes[12] = 0u;
    image.written[12] = true;
    WriteDword(image, 16u, 0u);
    image.bytes[20] = stack_byte;
    image.written[20] = true;
    WriteDword(image, 24u, 0u);
    WriteDword(image, 28u, 0u);
    WriteDword(image, 32u, 0u);
    return image;
}

ScriptIndexedAdapterImage ScriptEventSystem::MakeAdapter(
    std::uint32_t callback, std::uint32_t index) {
    if (callback == 0x42f140u)
        return {0x842b9cu, callback, index};
    ScriptIndexedAdapterImage image{};
    (void)ConstructWriteAdapter(image, 0u, callback, index);
    return image;
}

std::uint32_t ScriptEventSystem::ConstructWriteAdapter(
    ScriptIndexedAdapterImage& image, std::uint32_t object_address,
    std::uint32_t callback, std::uint32_t index) {
    image = {0x842be4u, callback, index};
    return object_address;
}

std::uint32_t ScriptEventSystem::Initialize(
    ScriptEventSystemState& state, std::uint8_t stack_byte,
    IScriptEventSystemRuntime& runtime, std::uint32_t descriptor_begin,
    std::uint32_t descriptor_end) {
    if ((state.setup_guard & 1u) == 0u) {
        state.setup_guard |= 1u;
        for (auto& collection : state.collections) {
            collection[0] = (collection[0] & 0xffffff00u) | stack_byte;
            collection[1] = 0u;
            collection[2] = 0u;
            collection[3] = 0u;
        }
        runtime.RegisterAtExit(kCollectionCleanup);
    }
    if (state.population_guard != 0u)
        return kCollectionAddress;

    state.population_guard = 1u;
    auto descriptor = descriptor_begin;
    std::uint32_t index = 0u;
    do {
        const auto object = runtime.Allocate(36u);
        if (object != 0u) {
            try {
                runtime.StoreObject(object, MakeObject(stack_byte));
            } catch (...) {
                runtime.Free(object);
                throw;
            }
        }

        if (static_cast<std::int32_t>(descriptor) >=
            static_cast<std::int32_t>(descriptor_end)) {
            if ((state.error_guard & 1u) == 0u) {
                state.error_guard |= 1u;
                state.error_first_byte = stack_byte;
                runtime.InitializeErrorString(
                    state.error_text, "getEnumStringInArray Error",
                    stack_byte);
                runtime.RegisterAtExit(kErrorStringCleanup);
            }
            runtime.AssignName(object, 0x978b20u, true);
        } else {
            runtime.AssignName(object, descriptor, false);
        }

        BindAdapter(runtime, object, 4u, 0x42f140u, index);
        BindAdapter(runtime, object, 12u, 0x42f160u, index);
        runtime.PushBack(object);
        descriptor += 16u;
        ++index;
    } while (static_cast<std::int32_t>(descriptor) <
             static_cast<std::int32_t>(descriptor_end));
    runtime.Sort();
    return kCollectionAddress;
}

void ScriptEventSystem::Cleanup(IScriptEventSystemRuntime& runtime) {
    runtime.CleanupCollection(kCollectionAddress);
}

std::uint32_t ScriptEventSystem::GetEnumStringItem(
    std::uint32_t base, std::int32_t index, std::int32_t count,
    ScriptEventSystemState& state,
    IScriptEventSystemRuntime& runtime) {
    if (index < count)
        return base + static_cast<std::uint32_t>(index) * 16u;
    if ((state.error_guard & 1u) == 0u) {
        state.error_guard = static_cast<std::uint8_t>(
            state.error_guard | 1u);
        state.error_first_byte = static_cast<std::uint8_t>(index);
        state.error_text = {};
        runtime.InitializeErrorString(
            state.error_text, "getEnumStringInArray Error",
            state.error_first_byte);
        runtime.RegisterAtExit(kErrorStringCleanup);
    }
    return 0x978b20u;
}

void ScriptEventSystem::CleanupErrorString(
    ScriptEventSystemState& state,
    IScriptEventErrorCleanupRuntime& runtime) {
    if (state.error_text.data != 0u) {
        const auto header = state.error_text.data - 1u;
        const auto reference = runtime.ReadByte(header);
        if (reference == 0u || reference == 0xffu)
            runtime.Free(header);
        else
            runtime.WriteByte(header,
                              static_cast<std::uint8_t>(reference - 1u));
    }
    state.error_text = {};
}

}  // namespace pal4
