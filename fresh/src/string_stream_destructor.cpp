#include "pal4/string_stream_destructor.h"

namespace pal4 {

std::uint32_t StringStreamDestructor::Destroy(
    const std::uint32_t streambuf,
    IStringStreamDestructorRuntime& runtime) {
    const auto base = streambuf - 0x58u;
    auto vtable = runtime.ReadWord(base);
    auto offset = runtime.ReadWord(vtable + 4u);
    runtime.WriteWord(base + offset, 0x84a050u);
    const auto flags = runtime.ReadWord(streambuf - 8u);

    try {
        runtime.WriteWord(streambuf - 0x4cu, 0x84814cu);
        if ((flags & 1u) != 0u) {
            runtime.Free(runtime.ReadWord(
                runtime.ReadWord(streambuf - 0x40u)));
        }
        runtime.WriteWord(streambuf - 0x10u, 0u);
        runtime.WriteWord(streambuf - 0x4cu, 0x868310u);
        runtime.WriteWord(streambuf - 8u, flags & 0xfffffffeu);

        const auto locale = runtime.ReadWord(streambuf - 0x18u);
        if (locale != 0u) {
            const auto count = runtime.ReadWord(locale + 4u);
            if (count != 0u && count != 0xffffffffu) {
                runtime.WriteWord(locale + 4u, count - 1u);
            }
            if (runtime.ReadWord(locale + 4u) == 0u) {
                const auto function = runtime.ReadWord(runtime.ReadWord(locale));
                runtime.DestroyLocale(function, locale, 1u);
            }
        }

        vtable = runtime.ReadWord(base);
        offset = runtime.ReadWord(vtable + 4u);
        runtime.WriteWord(base + offset, 0x849facu);
        const auto ostream_base = streambuf - 0x50u;
        vtable = runtime.ReadWord(ostream_base);
        offset = runtime.ReadWord(vtable + 4u);
        runtime.WriteWord(ostream_base + offset, 0x8682c8u);
        const auto result = runtime.ReadWord(base);
        offset = runtime.ReadWord(result + 4u);
        runtime.WriteWord(base + offset, 0x8682c0u);
        return result;
    } catch (...) {
        runtime.DestroyFileStream(streambuf - 0x4cu);
        throw;
    }
}

}  // namespace pal4
