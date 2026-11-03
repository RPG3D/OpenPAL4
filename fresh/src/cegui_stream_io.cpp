#include "pal4/cegui_stream_io.h"

namespace pal4 {

std::uint32_t CeguiStreamIoRuntimeStub::ReadDword(
    const std::uint32_t address) { return words_.at(address); }
void CeguiStreamIoRuntimeStub::WriteDword(
    const std::uint32_t address, const std::uint32_t value) {
    words_[address] = value;
}
std::uint32_t CeguiStreamIoRuntimeStub::DestroyIosBase(
    std::uint32_t /*ios_address*/) { return 0u; }
void CeguiStreamIoRuntimeStub::ClearIos(
    std::uint32_t /*ios_address*/, std::uint32_t /*flags*/,
    bool /*rethrow*/) {}
bool CeguiStreamIoRuntimeStub::ProcessCharacterInput(
    std::uint32_t /*stream_address*/, std::uint32_t /*mode*/) {
    return false;
}
std::uint32_t CeguiStreamIoRuntimeStub::ReadStreamBuffer(
    std::uint32_t /*streambuf_address*/, std::uint32_t /*buffer*/,
    std::uint32_t /*bytes*/) { return 0u; }
void CeguiStreamIoRuntimeStub::DestroyCeguiIOStream(
    std::uint32_t /*stream_address*/) {}
void CeguiStreamIoRuntimeStub::Free(std::uint32_t /*address*/) {}
void CeguiStreamIoRuntimeStub::InitializeIosBase(
    std::uint32_t /*ios_address*/) {}
void CeguiStreamIoRuntimeStub::AddStandardStream(
    std::uint32_t /*ios_address*/) {}
void CeguiStreamIoRuntimeStub::WriteByte(
    const std::uint32_t address, const std::uint8_t value) {
    bytes_[address] = value;
}

std::uint32_t CeguiStreamIO::DestroyIos(
    const std::uint32_t ios_address, ICeguiStreamIoRuntime& runtime) {
    runtime.WriteDword(ios_address, kIosVtable);
    return runtime.DestroyIosBase(ios_address);
}

void CeguiStreamIO::SetState(
    const std::uint32_t ios_address, const std::uint32_t flags,
    const bool rethrow, ICeguiStreamIoRuntime& runtime) {
    if (flags == 0u) {
        return;
    }
    auto combined = runtime.ReadDword(ios_address + 4u) | flags;
    if (runtime.ReadDword(ios_address + 40u) == 0u) {
        combined |= 4u;
    }
    runtime.ClearIos(ios_address, combined, rethrow);
}

void CeguiStreamIO::SetStateWithoutPreviousFlags(
    const std::uint32_t ios_address, std::uint32_t flags,
    const bool rethrow, ICeguiStreamIoRuntime& runtime) {
    if (runtime.ReadDword(ios_address + 40u) == 0u) {
        flags |= 4u;
    }
    runtime.ClearIos(ios_address, flags, rethrow);
}

std::uint32_t CeguiStreamIO::DestroyIstreamSubobject(
    const std::uint32_t subobject_address,
    ICeguiStreamIoRuntime& runtime) {
    const auto vtable = runtime.ReadDword(subobject_address - 8u);
    const auto offset = runtime.ReadDword(vtable + 4u);
    runtime.WriteDword(subobject_address + offset - 8u,
                       kIstreamVtable);
    return vtable;
}

std::uint32_t CeguiStreamIO::Read(
    const std::uint32_t stream_address, const std::uint32_t buffer,
    const std::uint32_t bytes, ICeguiStreamIoRuntime& runtime) {
    runtime.WriteDword(stream_address + 4u, 0u);
    if (!runtime.ProcessCharacterInput(stream_address, 1u)) {
        return stream_address;
    }
    const auto vtable = runtime.ReadDword(stream_address);
    const auto offset = runtime.ReadDword(vtable + 4u);
    const auto streambuf = runtime.ReadDword(stream_address + offset + 40u);
    const auto copied = runtime.ReadStreamBuffer(streambuf, buffer, bytes);
    runtime.WriteDword(stream_address + 4u, copied);
    if (copied != bytes) {
        SetState(stream_address + offset, 3u, false, runtime);
    }
    return stream_address;
}

std::uint32_t CeguiStreamIO::DestroyCeguiIstream(
    const std::uint32_t stream_address, const std::uint8_t flags,
    ICeguiStreamIoRuntime& runtime) {
    runtime.DestroyCeguiIOStream(stream_address);
    if ((flags & 1u) != 0u) {
        runtime.Free(stream_address);
    }
    return stream_address;
}

std::uint32_t CeguiStreamIO::DestroyCeguiIstreamWrapper(
    const std::uint32_t stream_address, const std::uint8_t flags,
    ICeguiStreamIoRuntime& runtime) {
    (void)DestroyIstreamSubobject(stream_address, runtime);
    runtime.DestroyCeguiIOStream(stream_address);
    const auto object = stream_address - 8u;
    if ((flags & 1u) != 0u) {
        runtime.Free(object);
    }
    return object;
}

std::uint32_t CeguiStreamIO::ConstructIstream(
    const std::uint32_t stream_address,
    const std::uint32_t streambuf,
    const bool add_standard, const bool initialize_base,
    ICeguiStreamIoRuntime& runtime) {
    if (initialize_base) {
        runtime.WriteDword(stream_address, kCeguiIstreamBaseVtable);
        runtime.WriteDword(stream_address + 44u, 0u);
        runtime.WriteDword(stream_address + 8u, kIosVtable);
    }
    try {
        runtime.WriteDword(stream_address + 4u, 0u);
        const auto vtable = runtime.ReadDword(stream_address);
        const auto offset = runtime.ReadDword(vtable + 4u);
        runtime.WriteDword(stream_address + offset, kIstreamVtable);
        const auto ios_address = stream_address + offset;
        runtime.WriteDword(ios_address + 40u, streambuf);
        runtime.WriteDword(ios_address + 44u, 0u);
        runtime.WriteByte(ios_address + 48u, 0x20u);
        runtime.InitializeIosBase(ios_address);
        if (runtime.ReadDword(ios_address + 40u) == 0u) {
            runtime.ClearIos(ios_address,
                             runtime.ReadDword(ios_address + 4u) | 4u,
                             false);
        }
        if (add_standard) {
            runtime.AddStandardStream(ios_address);
        }
        return stream_address;
    } catch (...) {
        if (initialize_base) {
            runtime.DestroyCeguiIOStream(stream_address + 8u);
        }
        throw;
    }
}

}  // namespace pal4
