#include "pal4/cegui_file_buffer_lifecycle.h"

namespace pal4 {

std::uint32_t CeguiFileBufferRuntimeStub::ReadDword(
    const std::uint32_t address) { return words_.at(address); }
void CeguiFileBufferRuntimeStub::WriteDword(
    const std::uint32_t address, const std::uint32_t value) {
    words_[address] = value;
}
std::uint8_t CeguiFileBufferRuntimeStub::ReadByte(
    const std::uint32_t address) { return bytes_.at(address); }
void CeguiFileBufferRuntimeStub::WriteByte(
    const std::uint32_t address, const std::uint8_t value) {
    bytes_[address] = value;
}
std::int32_t CeguiFileBufferRuntimeStub::CloseCrtFile(
    std::uint32_t /*file*/) { return 0; }
void CeguiFileBufferRuntimeStub::InitializeWideStreambuf(
    std::uint32_t /*object*/) {}
std::uint32_t CeguiFileBufferRuntimeStub::GlobalLocaleValue() {
    return 0u;
}
void CeguiFileBufferRuntimeStub::SetErrorFlag(
    std::uint32_t /*object*/, std::uint8_t /*value*/) {}
void CeguiFileBufferRuntimeStub::Free(
    std::uint32_t /*address*/) {}
std::uint32_t CeguiFileBufferRuntimeStub::DestroyReference(
    std::uint32_t /*object*/, std::uint8_t /*flags*/) { return 0u; }
void CeguiFileBufferRuntimeStub::InitializeFileBuffer(
    std::uint32_t /*filebuf*/, std::uint32_t /*first*/,
    std::uint32_t /*mode*/) {}
std::uint32_t CeguiFileBufferRuntimeStub::DecrementLocaleFacet(
    std::uint32_t /*facet*/) { return 0u; }
std::uint32_t CeguiFileBufferRuntimeStub::InitializeLocale() { return 0u; }
std::uint32_t CeguiFileBufferRuntimeStub::GlobalLocaleObject() { return 0u; }
void CeguiFileBufferRuntimeStub::ConfigureFilePatterns(
    const std::uint32_t filebuf, const std::uint32_t file) {
    (void)CeguiFileBufferLifecycle::SetPatternPointers(
        filebuf, file + 8u, file, file + 4u,
        file + 8u, file, file + 4u, *this);
}
std::uint32_t CeguiFileBufferRuntimeStub::OpenCrtFile(
    std::uint32_t /*name*/, std::uint32_t /*mode*/) { return 0u; }
void CeguiFileBufferRuntimeStub::AcquireCodecvtFacet(
    std::uint32_t /*filebuf*/) {}

void CeguiFileBufferLifecycle::IncrementGlobalLocaleReference(
    ICeguiFileBufferRuntime& runtime) {
    const auto locale = runtime.GlobalLocaleObject();
    const auto count = runtime.ReadDword(locale + 4u);
    if (count != 0xffffffffu) {
        runtime.WriteDword(locale + 4u, count + 1u);
    }
}

std::uint32_t CeguiFileBufferLifecycle::SetGetArea(
    const std::uint32_t object, const std::uint32_t begin,
    const std::uint32_t current, const std::uint32_t end,
    ICeguiFileBufferRuntime& runtime) {
    runtime.WriteDword(runtime.ReadDword(object + 12u), begin);
    runtime.WriteDword(runtime.ReadDword(object + 28u), current);
    const auto count_address = runtime.ReadDword(object + 44u);
    runtime.WriteDword(count_address, end - current);
    return count_address;
}

std::uint32_t CeguiFileBufferLifecycle::SetPutArea(
    const std::uint32_t object, const std::uint32_t begin,
    const std::uint32_t end, ICeguiFileBufferRuntime& runtime) {
    runtime.WriteDword(runtime.ReadDword(object + 16u), begin);
    runtime.WriteDword(runtime.ReadDword(object + 32u), begin);
    const auto count_address = runtime.ReadDword(object + 48u);
    runtime.WriteDword(count_address, end - begin);
    return count_address;
}

std::uint32_t CeguiFileBufferLifecycle::ConstructFileBuffer(
    const std::uint32_t object, const std::uint32_t file,
    ICeguiFileBufferRuntime& runtime) {
    runtime.WriteDword(object + 52u, runtime.InitializeLocale());
    IncrementGlobalLocaleReference(runtime);
    runtime.WriteDword(object + 28u, object + 20u);
    runtime.WriteDword(object + 32u, object + 24u);
    runtime.WriteDword(object + 44u, object + 36u);
    runtime.WriteDword(object, kStreambufVtable);
    runtime.WriteDword(object + 12u, object + 4u);
    runtime.WriteDword(object + 16u, object + 8u);
    runtime.WriteDword(object + 48u, object + 40u);
    runtime.WriteDword(object + 8u, 0u);
    runtime.WriteDword(runtime.ReadDword(object + 32u), 0u);
    runtime.WriteDword(runtime.ReadDword(object + 48u), 0u);
    (void)SetGetArea(object, 0u, 0u, 0u, runtime);

    runtime.WriteDword(object + 76u, runtime.InitializeLocale());
    IncrementGlobalLocaleReference(runtime);
    runtime.WriteDword(object, kFilebufVtable);
    runtime.WriteByte(object + 72u, 0u);
    (void)ReleaseReference(runtime.ReadDword(object + 76u), runtime);
    if (object + 76u != 0u) {
        runtime.WriteDword(object + 76u, runtime.InitializeLocale());
        IncrementGlobalLocaleReference(runtime);
    }

    runtime.WriteDword(object + 28u, object + 20u);
    runtime.WriteDword(object + 32u, object + 24u);
    runtime.WriteDword(object + 44u, object + 36u);
    runtime.WriteDword(object + 68u, 0u);
    runtime.WriteDword(object + 12u, object + 4u);
    runtime.WriteDword(object + 16u, object + 8u);
    runtime.WriteDword(object + 48u, object + 40u);
    (void)SetPutArea(object, 0u, 0u, runtime);
    (void)SetGetArea(object, 0u, 0u, 0u, runtime);
    if (file != 0u && runtime.ReadByte(object + 72u) == 0u) {
        runtime.ConfigureFilePatterns(object, file);
    }
    runtime.WriteDword(object + 80u, file);
    runtime.WriteDword(object + 64u, runtime.GlobalLocaleValue());
    runtime.WriteDword(object + 60u, runtime.GlobalLocaleValue());
    runtime.WriteDword(object + 56u, 0u);
    return object;
}

void CeguiFileBufferLifecycle::ResetBufferPointers(
    const std::uint32_t object, const bool mark_open,
    ICeguiFileBufferRuntime& runtime) {
    runtime.WriteDword(object + 12u, object + 4u);
    runtime.WriteDword(object + 28u, object + 20u);
    runtime.WriteDword(object + 32u, object + 24u);
    runtime.WriteDword(object + 44u, object + 36u);
    if (mark_open) {
        runtime.WriteByte(object + 72u, 1u);
    }
    runtime.WriteDword(object + 16u, object + 8u);
    runtime.WriteDword(object + 48u, object + 40u);
    runtime.WriteDword(object + 8u, 0u);
    runtime.WriteDword(runtime.ReadDword(object + 32u), 0u);
    runtime.WriteDword(runtime.ReadDword(object + 48u), 0u);
    runtime.WriteDword(runtime.ReadDword(object + 12u), 0u);
    runtime.WriteDword(runtime.ReadDword(object + 28u), 0u);
    runtime.WriteDword(runtime.ReadDword(object + 44u), 0u);
}

std::uint32_t CeguiFileBufferLifecycle::Open(
    const std::uint32_t object, const std::uint32_t name,
    const std::uint32_t mode, ICeguiFileBufferRuntime& runtime) {
    if (runtime.ReadDword(object + 80u) != 0u) {
        return 0u;
    }
    const auto file = runtime.OpenCrtFile(name, mode);
    if (file == 0u) {
        return 0u;
    }
    ResetBufferPointers(object, true, runtime);
    if (runtime.ReadByte(object + 72u) == 0u) {
        runtime.WriteDword(object + 28u, file);
        runtime.WriteDword(object + 12u, file + 8u);
        runtime.WriteDword(object + 16u, file + 8u);
        runtime.WriteDword(object + 32u, file);
        runtime.WriteDword(object + 44u, file + 4u);
        runtime.WriteDword(object + 48u, file + 4u);
    }
    runtime.WriteDword(object + 80u, file);
    runtime.WriteDword(object + 64u, runtime.GlobalLocaleValue());
    runtime.WriteDword(object + 60u, runtime.GlobalLocaleValue());
    runtime.WriteDword(object + 56u, 0u);
    runtime.AcquireCodecvtFacet(object);
    return object;
}

std::uint32_t CeguiFileBufferLifecycle::Close(
    const std::uint32_t object, ICeguiFileBufferRuntime& runtime) {
    const auto file = runtime.ReadDword(object + 80u);
    if (file == 0u || runtime.CloseCrtFile(file) != 0) {
        return 0u;
    }
    runtime.WriteByte(object + 72u, 0u);
    ResetBufferPointers(object, false, runtime);
    runtime.WriteDword(object + 80u, 0u);
    runtime.WriteDword(object + 64u, runtime.GlobalLocaleValue());
    runtime.WriteDword(object + 60u, runtime.GlobalLocaleValue());
    runtime.WriteDword(object + 56u, 0u);
    return object;
}

std::uint32_t CeguiFileBufferLifecycle::InitializeDefaultLocale(
    const std::uint32_t locale_slot, ICeguiFileBufferRuntime& runtime) {
    runtime.WriteDword(locale_slot, runtime.InitializeLocale());
    IncrementGlobalLocaleReference(runtime);
    return locale_slot;
}

std::uint32_t CeguiFileBufferLifecycle::Initialize(
    const std::uint32_t object, const std::uint32_t file,
    const std::uint32_t mode, ICeguiFileBufferRuntime& runtime) {
    runtime.WriteByte(object + 72u, mode == 1u ? 1u : 0u);
    if (mode == 0u) {
        (void)ReleaseReference(runtime.ReadDword(object + 76u), runtime);
        if (object + 76u != 0u) {
            (void)InitializeDefaultLocale(object + 76u, runtime);
        }
        runtime.WriteDword(object + 68u, 0u);
    }
    ResetBufferPointers(object, false, runtime);
    if (file != 0u && runtime.ReadByte(object + 72u) == 0u) {
        runtime.WriteDword(object + 28u, file);
        runtime.WriteDword(object + 12u, file + 8u);
        runtime.WriteDword(object + 16u, file + 8u);
        runtime.WriteDword(object + 32u, file);
        runtime.WriteDword(object + 44u, file + 4u);
        runtime.WriteDword(object + 48u, file + 4u);
    }
    runtime.WriteDword(object + 80u, file);
    runtime.WriteDword(object + 64u, runtime.GlobalLocaleValue());
    const auto state = runtime.GlobalLocaleValue();
    runtime.WriteDword(object + 60u, state);
    runtime.WriteDword(object + 56u, 0u);
    return state;
}

std::uint32_t CeguiFileBufferLifecycle::InitializeStreambufPointers(
    const std::uint32_t object, ICeguiFileBufferRuntime& runtime) {
    runtime.WriteDword(object + 28u, object + 20u);
    runtime.WriteDword(object + 32u, object + 24u);
    runtime.WriteDword(object + 12u, object + 4u);
    runtime.WriteDword(object + 44u, object + 36u);
    runtime.WriteDword(object + 16u, object + 8u);
    runtime.WriteDword(object + 48u, object + 40u);
    runtime.WriteDword(runtime.ReadDword(object + 16u), 0u);
    runtime.WriteDword(runtime.ReadDword(object + 32u), 0u);
    runtime.WriteDword(runtime.ReadDword(object + 48u), 0u);
    runtime.WriteDword(runtime.ReadDword(object + 12u), 0u);
    runtime.WriteDword(runtime.ReadDword(object + 28u), 0u);
    const auto last_count = runtime.ReadDword(object + 44u);
    runtime.WriteDword(last_count, 0u);
    return last_count;
}

std::uint32_t CeguiFileBufferLifecycle::SetPatternPointers(
    const std::uint32_t object, const std::uint32_t get_begin,
    const std::uint32_t get_current, const std::uint32_t get_count,
    const std::uint32_t put_begin, const std::uint32_t put_current,
    const std::uint32_t put_count,
    ICeguiFileBufferRuntime& runtime) {
    runtime.WriteDword(object + 12u, get_begin);
    runtime.WriteDword(object + 16u, put_begin);
    runtime.WriteDword(object + 28u, get_current);
    runtime.WriteDword(object + 32u, put_current);
    runtime.WriteDword(object + 44u, get_count);
    runtime.WriteDword(object + 48u, put_count);
    return get_count;
}

std::uint32_t CeguiFileBufferLifecycle::ReleaseReference(
    const std::uint32_t object, ICeguiFileBufferRuntime& runtime) {
    if (object == 0u) {
        return 0u;
    }
    auto count = runtime.ReadDword(object + 4u);
    if (count != 0u && count != 0xffffffffu) {
        --count;
        runtime.WriteDword(object + 4u, count);
    }
    if (runtime.ReadDword(object + 4u) == 0u) {
        return runtime.DestroyReference(object, 1u);
    }
    return object;
}

std::uint32_t CeguiFileBufferLifecycle::DestroyFileBuffer(
    const std::uint32_t object, ICeguiFileBufferRuntime& runtime) {
    runtime.WriteDword(object, kFilebufVtable);
    if (runtime.ReadByte(object + 72u) != 0u) {
        const auto file = runtime.ReadDword(object + 80u);
        if (file != 0u && runtime.CloseCrtFile(file) == 0) {
            runtime.WriteByte(object + 72u, 0u);
            runtime.InitializeWideStreambuf(object);
            runtime.WriteDword(object + 80u, 0u);
            runtime.WriteDword(object + 64u,
                               runtime.GlobalLocaleValue());
            runtime.WriteDword(object + 60u,
                               runtime.GlobalLocaleValue());
            runtime.WriteDword(object + 56u, 0u);
        }
    }
    const auto error_buffer = runtime.ReadDword(object + 68u);
    if (error_buffer != 0u) {
        runtime.SetErrorFlag(error_buffer, 1u);
        runtime.Free(error_buffer);
    }
    static_cast<void>(ReleaseReference(
        runtime.ReadDword(object + 76u), runtime));
    runtime.WriteDword(object, kStreambufVtable);
    return ReleaseReference(runtime.ReadDword(object + 52u), runtime);
}

std::uint32_t CeguiFileBufferLifecycle::DestroyStreambuf(
    const std::uint32_t object, ICeguiFileBufferRuntime& runtime) {
    runtime.WriteDword(object, kStreambufVtable);
    return ReleaseReference(runtime.ReadDword(object + 52u), runtime);
}

std::uint32_t CeguiFileBufferLifecycle::DestroyFileStreamSubobjects(
    const std::uint32_t istream_subobject,
    ICeguiFileBufferRuntime& runtime) {
    const auto filebuf = istream_subobject - 84u;
    const auto base_vtable = runtime.ReadDword(filebuf - 8u);
    const auto offset = runtime.ReadDword(base_vtable + 4u);
    runtime.WriteDword(filebuf - 8u + offset,
                       kIstreamSubobjectVtable);
    runtime.WriteDword(filebuf, kFilebufVtable);
    if (runtime.ReadByte(filebuf + 72u) != 0u) {
        const auto file = runtime.ReadDword(filebuf + 80u);
        if (file != 0u && runtime.CloseCrtFile(file) == 0) {
            runtime.InitializeFileBuffer(filebuf, 0u, 2u);
        }
    }

    const auto auxiliary = runtime.ReadDword(filebuf + 68u);
    if (auxiliary != 0u) {
        runtime.SetErrorFlag(auxiliary, 1u);
        runtime.Free(auxiliary);
    }
    (void)ReleaseReference(runtime.ReadDword(filebuf + 76u), runtime);
    runtime.WriteDword(filebuf, kStreambufVtable);
    const auto facet = runtime.ReadDword(filebuf + 52u);
    if (facet != 0u) {
        const auto released = runtime.DecrementLocaleFacet(facet);
        if (released != 0u) {
            (void)runtime.DestroyReference(released, 1u);
        }
    }

    const auto resulting_vtable = runtime.ReadDword(filebuf - 8u);
    const auto final_offset = runtime.ReadDword(resulting_vtable + 4u);
    runtime.WriteDword(filebuf - 8u + final_offset,
                       kIstreamVtable);
    return resulting_vtable;
}

std::uint32_t CeguiFileBufferLifecycle::DestroyStreambufWithFlag(
    const std::uint32_t object, const std::uint8_t flags,
    ICeguiFileBufferRuntime& runtime) {
    (void)DestroyStreambuf(object, runtime);
    if ((flags & 1u) != 0u) {
        runtime.Free(object);
    }
    return object;
}

std::uint32_t CeguiFileBufferLifecycle::DestroyFileBufferWithFlag(
    const std::uint32_t object, const std::uint8_t flags,
    ICeguiFileBufferRuntime& runtime) {
    (void)DestroyFileBuffer(object, runtime);
    if ((flags & 1u) != 0u) {
        runtime.Free(object);
    }
    return object;
}

std::int32_t CeguiFileBufferLifecycle::ReturnNegativeOne(
    std::uint32_t /*ignored*/) noexcept { return -1; }
std::int32_t CeguiFileBufferLifecycle::Underflow() noexcept { return -1; }

}  // namespace pal4
