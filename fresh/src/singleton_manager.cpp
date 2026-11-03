#include "pal4/singleton_manager.h"

namespace pal4 {

std::uint32_t SingletonManager::Initialize(
    const std::uint32_t object_address) {
    state_.first_word = (state_.first_word & 0xffffff00u) |
                        (object_address >> 24u);
    state_.begin = 0u;
    state_.end = 0u;
    state_.capacity = 0u;
    return object_address;
}

std::uint32_t SingletonManager::Cleanup(ISingletonManagerRuntime& runtime) {
    runtime.DestroyAll();
    const std::uint32_t begin = state_.begin;
    const std::uint32_t end = state_.end;
    for (std::uint32_t element = begin; element != end; element += 28u) {
        const std::uint32_t value = runtime.ReadDword(element + 4u);
        if (value != 0u) {
            const std::uint32_t first = runtime.ResolveReferenceByte(
                element, value);
            bool dispose = runtime.ReadByte(first) == 0u;
            if (!dispose) {
                const std::uint32_t second = runtime.ResolveReferenceByte(
                    element, value);
                dispose = runtime.ReadByte(second) == 0xffu;
            }
            if (dispose) {
                const std::uint32_t field = runtime.ReadDword(element + 12u);
                runtime.DisposeElement(element, value - 1u, field + 2u);
            } else {
                const std::uint32_t third = runtime.ResolveReferenceByte(
                    element, value);
                runtime.WriteByte(third,
                                  static_cast<std::uint8_t>(
                                      runtime.ReadByte(third) - 1u));
            }
        }
        runtime.WriteDword(element + 4u, 0u);
        runtime.WriteDword(element + 8u, 0u);
        runtime.WriteDword(element + 12u, 0u);
    }
    const std::uint32_t result = runtime.Free(begin);
    state_.begin = 0u;
    state_.end = 0u;
    state_.capacity = 0u;
    return result;
}

std::uint32_t SingletonManager::CompareBySingletonId(
    const std::uint32_t left, const std::uint32_t right,
    ISingletonManagerRuntime& runtime) {
    const auto left_id = static_cast<std::int32_t>(
        runtime.ReadDword(left + 16u));
    const auto right_id = static_cast<std::int32_t>(
        runtime.ReadDword(right + 16u));
    return left_id > right_id ? 1u : 0u;
}

std::uint32_t SingletonManager::DestroyAll(
    const std::uint32_t object_address,
    ISingletonDestroyRuntime& runtime) {
    if (state_.begin == 0u) {
        return object_address;
    }
    const auto difference = static_cast<std::int32_t>(
        state_.end - state_.begin);
    if (difference / 28 == 0) {
        return 0u;
    }

    runtime.SortByDescendingId(state_.begin, state_.end);
    for (std::uint32_t element = state_.begin;
         element != state_.end; element += 28u) {
        const std::uint32_t name = runtime.ReadDword(element + 4u);
        const std::uint32_t id = runtime.ReadDword(element + 16u);
        runtime.LogDestroy(id,
            name != 0u ? name : kEmptyLogMessageAddress,
            "D:\\VcProject\\Pal4\\project\\Sourcecode\\Scl\\scl_Singleton.cpp",
            "FILE:  [%s]\nLINE:  [%d]\nINFO:  singleton-destroy: [ %d ], [ %s ]\n\n\n",
            64u);
        const std::uint32_t argument = runtime.ReadDword(element + 20u);
        const std::uint32_t callback = runtime.ReadDword(element + 24u);
        runtime.InvokeDestroy(callback, argument);
    }

    const std::uint32_t begin = state_.begin;
    const std::uint32_t end = state_.end;
    std::uint32_t result = object_address;
    for (std::uint32_t element = begin; element != end; element += 28u) {
        result = runtime.ReleaseTextRecord(element);
    }
    state_.end = begin;
    return result;
}

std::uint32_t SingletonManager::Register(
    const char* type_name, const std::uint32_t singleton_id,
    const std::uint32_t factory, const std::uint32_t type_info,
    ISingletonRegistrationRuntime& runtime) {
    const std::string label = runtime.FormatTypeLabel(
        type_name, factory, type_info);
    SingletonRegistrationEntry entry;
    runtime.SetErrorFlag(entry, 0u);
    entry.singleton_id = singleton_id;
    const auto label_length = static_cast<std::uint32_t>(label.size());

    const auto release_text = [&]() {
        if (entry.text == 0u) {
            return;
        }
        const std::uint32_t reference = entry.text - 1u;
        const std::uint8_t count = runtime.ReadByte(reference);
        if (count == 0u || count == 0xffu) {
            runtime.FreeText(reference);
        } else {
            runtime.WriteByte(reference,
                static_cast<std::uint8_t>(count - 1u));
        }
    };

    try {
        if (runtime.AllocateText(entry, label_length, 1u)) {
            runtime.CopyText(entry.text, label);
            runtime.SetTextSize(entry, label_length);
        }

        const std::uint32_t scan_end = state_.end;
        std::uint32_t element = state_.begin;
        while (element != scan_end) {
            if (runtime.ReadDword(element + 16u) == singleton_id) {
                const std::uint32_t text = entry.text != 0u
                    ? entry.text : kEmptyLogMessageAddress;
                const std::uint32_t element_text =
                    runtime.ReadDword(element + 4u);
                const std::uint32_t element_length =
                    runtime.ReadDword(element + 8u);
                const StringBuffer stored({element_text, element_length, 0u});
                if (stored.CompareRange(0u, element_length,
                                        text, entry.text_length,
                                        runtime) == 0) {
                    break;
                }
            }
            element += 28u;
        }

        std::uint32_t result;
        if (element == state_.end) {
            entry.instance = runtime.Create(factory);
            entry.type_info = type_info;
            runtime.Insert(state_.end, 1u, entry);
            result = entry.instance;
        } else {
            result = runtime.ReadDword(element + 20u);
        }
        release_text();
        return result;
    } catch (...) {
        release_text();
        throw;
    }
}

std::uint32_t SingletonManager::CleanupCollection(
    ISingletonCollectionCleanupRuntime& runtime) {
    const std::uint32_t begin = state_.begin;
    const std::uint32_t end = state_.end;
    for (std::uint32_t element = begin; element != end; element += 28u) {
        runtime.ReleaseTextRecord(element);
    }
    runtime.Free(begin);
    state_.begin = 0u;
    state_.end = 0u;
    state_.capacity = 0u;
    return 0u;
}

std::uint32_t SingletonManagerAccess::GetInstance(
    ISingletonManagerAccessRuntime& runtime) {
    if ((state_.initialization_bits & 1u) == 0u) {
        state_.initialization_bits |= 1u;
        const std::uint32_t allocated = runtime.Allocate(16u);
        const std::uint32_t instance = allocated != 0u
            ? runtime.Initialize(allocated) : 0u;
        state_.owns_instance = instance != 0u ? 1u : 0u;
        state_.instance = instance;
        runtime.RegisterExit(kExitCallbackAddress);
    }
    return state_.instance;
}

void SingletonManagerAccess::ExitCleanup(
    ISingletonManagerAccessRuntime& runtime) {
    if (state_.owns_instance != 0u && state_.instance != 0u) {
        runtime.Cleanup(state_.instance);
        runtime.Free(state_.instance);
    }
}

}  // namespace pal4
