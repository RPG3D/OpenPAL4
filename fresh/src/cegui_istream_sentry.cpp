#include "pal4/cegui_istream_sentry.h"

#include <typeinfo>

namespace pal4 {

std::uint32_t CeguiIstreamSentryRuntimeStub::ReadDword(
    const std::uint32_t address) { return words_.at(address); }
void CeguiIstreamSentryRuntimeStub::WriteDword(
    const std::uint32_t address, const std::uint32_t value) {
    words_[address] = value;
}
std::uint8_t CeguiIstreamSentryRuntimeStub::ReadByte(
    const std::uint32_t address) { return bytes_.at(address); }
void CeguiIstreamSentryRuntimeStub::ClearIos(
    std::uint32_t /*ios*/, std::uint32_t /*flags*/,
    bool /*rethrow*/) {}
std::int32_t CeguiIstreamSentryRuntimeStub::SyncStreambuf(
    std::uint32_t /*streambuf*/) { return -1; }
std::int32_t CeguiIstreamSentryRuntimeStub::PeekVirtual(
    std::uint32_t /*streambuf*/) { return -1; }
std::int32_t CeguiIstreamSentryRuntimeStub::ConsumeVirtual(
    std::uint32_t /*streambuf*/) { return -1; }
CeguiLocaleSnapshot CeguiIstreamSentryRuntimeStub::AcquireLocale(
    std::uint32_t /*ios*/) { return {}; }
std::uint32_t CeguiIstreamSentryRuntimeStub::FindClassificationFacet(
    std::uint32_t /*locale*/) { return 0u; }
bool CeguiIstreamSentryRuntimeStub::IsClassicLocale(
    std::uint32_t /*locale*/) { return false; }
std::uint32_t CeguiIstreamSentryRuntimeStub::ClassicClassificationFacet() {
    return 0u;
}
std::uint32_t CeguiIstreamSentryRuntimeStub::ReleaseLocaleFacet(
    std::uint32_t /*facet*/) { return 0u; }
void CeguiIstreamSentryRuntimeStub::DestroyFacet(
    std::uint32_t /*facet*/, std::uint8_t /*flags*/) {}
void CeguiIstreamSentryRuntimeStub::ThrowMissingFacet() {
    throw std::bad_cast();
}

std::int32_t CeguiIstreamSentry::PeekStreamBuffer(
    const std::uint32_t streambuf,
    ICeguiIstreamSentryRuntime& runtime) {
    const auto cursor_address = runtime.ReadDword(streambuf + 28u);
    const auto cursor = runtime.ReadDword(cursor_address);
    if (cursor != 0u) {
        const auto count_address = runtime.ReadDword(streambuf + 44u);
        const auto count = runtime.ReadDword(count_address);
        if (cursor < cursor + count) {
            return runtime.ReadByte(cursor);
        }
    }
    return runtime.PeekVirtual(streambuf);
}

std::int32_t CeguiIstreamSentry::ConsumeStreamBuffer(
    const std::uint32_t streambuf,
    ICeguiIstreamSentryRuntime& runtime) {
    const auto cursor_address = runtime.ReadDword(streambuf + 28u);
    const auto cursor = runtime.ReadDword(cursor_address);
    if (cursor != 0u) {
        const auto count_address = runtime.ReadDword(streambuf + 44u);
        const auto count = runtime.ReadDword(count_address);
        if (cursor < cursor + count) {
            runtime.WriteDword(count_address, count - 1u);
            runtime.WriteDword(cursor_address, cursor + 1u);
            return runtime.ReadByte(cursor);
        }
    }
    return runtime.ConsumeVirtual(streambuf);
}

void CeguiIstreamSentry::ReleaseLocale(
    const CeguiLocaleSnapshot snapshot,
    ICeguiIstreamSentryRuntime& runtime) {
    if (snapshot.held_facet == 0u) {
        return;
    }
    const auto released = runtime.ReleaseLocaleFacet(snapshot.held_facet);
    if (released != 0u) {
        runtime.DestroyFacet(released, 1u);
    }
}

bool CeguiIstreamSentry::Process(
    const std::uint32_t stream, const bool no_skip_whitespace,
    ICeguiIstreamSentryRuntime& runtime) {
    const auto vtable = runtime.ReadDword(stream);
    auto ios = stream + runtime.ReadDword(vtable + 4u);
    if (runtime.ReadDword(ios + 4u) == 0u) {
        const auto tied = runtime.ReadDword(ios + 44u);
        if (tied != 0u) {
            const auto tied_vtable = runtime.ReadDword(tied);
            const auto tied_ios = tied + runtime.ReadDword(tied_vtable + 4u);
            std::uint32_t flags = 0u;
            if ((runtime.ReadByte(tied_ios + 4u) & 6u) == 0u &&
                runtime.SyncStreambuf(runtime.ReadDword(tied_ios + 40u))
                    == -1) {
                flags = 4u;
            }
            if (flags != 0u) {
                auto combined = runtime.ReadDword(tied_ios + 4u) | flags;
                if (runtime.ReadDword(tied_ios + 40u) == 0u) {
                    combined |= 4u;
                }
                runtime.ClearIos(tied_ios, combined, false);
            }
        }
        if (!no_skip_whitespace &&
            (runtime.ReadDword(ios + 12u) & 1u) != 0u) {
            const auto locale = runtime.AcquireLocale(ios);
            std::uint32_t facet = 0u;
            try {
                facet = runtime.FindClassificationFacet(locale.locale);
                if (facet == 0u) {
                    if (!runtime.IsClassicLocale(locale.locale)) {
                        runtime.ThrowMissingFacet();
                    }
                    facet = runtime.ClassicClassificationFacet();
                }
            } catch (...) {
                ReleaseLocale(locale, runtime);
                throw;
            }
            ReleaseLocale(locale, runtime);
            const auto table = runtime.ReadDword(facet + 16u);
            auto streambuf = runtime.ReadDword(ios + 40u);
            auto character = PeekStreamBuffer(streambuf, runtime);
            while (character != -1 &&
                   (runtime.ReadByte(table + 2u *
                       static_cast<std::uint8_t>(character)) & 0x48u) != 0u) {
                streambuf = runtime.ReadDword(ios + 40u);
                const auto consumed = ConsumeStreamBuffer(streambuf, runtime);
                if (consumed == -1) {
                    break;
                }
                character = PeekStreamBuffer(streambuf, runtime);
            }
        }
        ios = stream + runtime.ReadDword(runtime.ReadDword(stream) + 4u);
        if (runtime.ReadDword(ios + 4u) == 0u) {
            return true;
        }
    }
    auto flags = runtime.ReadDword(ios + 4u) | 2u;
    if (runtime.ReadDword(ios + 40u) == 0u) {
        flags |= 4u;
    }
    runtime.ClearIos(ios, flags, false);
    return false;
}

}  // namespace pal4
