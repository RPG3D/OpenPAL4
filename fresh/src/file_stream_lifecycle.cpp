#include "pal4/file_stream_lifecycle.h"

#include <stdexcept>

namespace pal4 {
namespace {

constexpr std::uint32_t kFileStreamVtable = 0x849f98u;
constexpr std::uint32_t kIostreamVtable = 0x849facu;
constexpr std::uint32_t kIostreamVirtualBase = 0x849fb8u;
constexpr std::uint32_t kIostreamOutputBase = 0x849fb0u;
constexpr std::uint32_t kFilebufVtable = 0x8682d8u;
constexpr std::uint32_t kStreambufVtable = 0x868310u;
constexpr std::uint32_t kOstreamVtable = 0x8682c8u;
constexpr std::uint32_t kIstreamVtable = 0x8682c0u;
constexpr std::uint32_t kIosVtable = 0x8682d0u;

std::uint32_t VirtualOffset(const std::uint32_t vtable,
                            IFileStreamLifecycleRuntime& runtime) {
    return runtime.ReadWord(vtable + 4u);
}

}  // namespace

std::uint32_t FileStreamLifecycle::DestroyIostreamBase(
    const std::uint32_t filebuf, IFileStreamLifecycleRuntime& runtime) {
    const auto base = filebuf - 12u;
    auto vtable = runtime.ReadWord(base);
    runtime.WriteWord(base + VirtualOffset(vtable, runtime),
                      kIostreamVtable);
    const auto ostream = filebuf - 4u;
    vtable = runtime.ReadWord(ostream);
    runtime.WriteWord(ostream + VirtualOffset(vtable, runtime),
                      kOstreamVtable);
    vtable = runtime.ReadWord(base);
    runtime.WriteWord(base + VirtualOffset(vtable, runtime),
                      kIstreamVtable);
    return vtable;
}

std::uint32_t FileStreamLifecycle::Destroy(
    const std::uint32_t ios, IFileStreamLifecycleRuntime& runtime) {
    const auto base = ios - 96u;
    const auto filebuf = ios - 84u;
    runtime.WriteWord(base + VirtualOffset(runtime.ReadWord(base), runtime),
                      kFileStreamVtable);
    runtime.WriteWord(filebuf, kFilebufVtable);
    const auto handle = runtime.ReadWord(filebuf + 80u);
    if (runtime.ReadByte(filebuf + 72u) != 0u && handle != 0u &&
        runtime.CloseFile(handle) == 0) {
        runtime.InitializeFileBuffer(filebuf, 0u, 2u);
    }

    const auto text = runtime.ReadWord(filebuf + 68u);
    if (text != 0u) {
        runtime.ResetText(text, 1u);
        runtime.Free(text);
    }
    const auto locale_record = runtime.ReadWord(filebuf + 76u);
    if (locale_record != 0u) {
        auto references = runtime.ReadWord(locale_record + 4u);
        if (references != 0u && references != 0xffffffffu) {
            --references;
            runtime.WriteWord(locale_record + 4u, references);
        }
        if (runtime.ReadWord(locale_record + 4u) == 0u)
            runtime.DeleteLocaleRecord(locale_record, 1u);
    }

    runtime.WriteWord(filebuf, kStreambufVtable);
    const auto facet = runtime.ReadWord(filebuf + 52u);
    if (facet != 0u) {
        const auto released = runtime.DecrementFacet(facet);
        if (released != 0u) runtime.DeleteFacet(released, 1u);
    }
    return DestroyIostreamBase(filebuf, runtime);
}

std::uint32_t FileStreamLifecycle::InitializeIostream(
    const std::uint32_t base, const std::uint32_t streambuf,
    const bool initialize_virtual_base, IFileStreamLifecycleRuntime& runtime) {
    if (initialize_virtual_base) {
        runtime.WriteWord(base, kIostreamVirtualBase);
        runtime.WriteWord(base + 8u, kIostreamOutputBase);
        runtime.WriteWord(base + 48u, 0u);
        runtime.WriteWord(base + 12u, kIosVtable);
    }
    auto vtable = runtime.ReadWord(base);
    runtime.WriteWord(base + 4u, 0u);
    runtime.WriteWord(base + VirtualOffset(vtable, runtime),
                      kIstreamVtable);

    vtable = runtime.ReadWord(base);
    const auto ios = base + VirtualOffset(vtable, runtime);
    runtime.WriteWord(ios + 40u, streambuf);
    runtime.WriteWord(ios + 44u, 0u);
    runtime.WriteByte(ios + 48u, 0x20u);
    runtime.InitializeIosBase(ios);
    if (runtime.ReadWord(ios + 40u) == 0u) {
        const auto state = runtime.ReadWord(ios + 4u);
        runtime.ClearIosBase(ios, state | 4u, false);
    }
    const auto output_vtable = runtime.ReadWord(base + 8u);
    runtime.WriteWord(base + VirtualOffset(output_vtable, runtime) + 8u,
                      kOstreamVtable);
    vtable = runtime.ReadWord(base);
    runtime.WriteWord(base + VirtualOffset(vtable, runtime),
                      kIostreamVtable);
    return base;
}

std::uint32_t FileStreamLifecycle::FreeMembers(
    const std::uint32_t object, IFileStreamReleaseRuntime& runtime) {
    if (object == 0u) return 0u;
    (void)runtime.Free(runtime.ReadWord(object + 8u));
    (void)runtime.Free(runtime.ReadWord(object + 4u));
    (void)runtime.Free(runtime.ReadWord(object));
    return runtime.Free(object);
}

}  // namespace pal4
