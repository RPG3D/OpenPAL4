#pragma once

#include <cstdint>
#include <map>

namespace pal4 {

struct CeguiLocaleSnapshot final {
    std::uint32_t locale = 0u;
    std::uint32_t held_facet = 0u;
};

class ICeguiIstreamSentryRuntime {
public:
    virtual ~ICeguiIstreamSentryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void ClearIos(std::uint32_t ios,
                          std::uint32_t flags,
                          bool rethrow) = 0;
    [[nodiscard]] virtual std::int32_t SyncStreambuf(
        std::uint32_t streambuf) = 0;
    [[nodiscard]] virtual std::int32_t PeekVirtual(
        std::uint32_t streambuf) = 0;
    [[nodiscard]] virtual std::int32_t ConsumeVirtual(
        std::uint32_t streambuf) = 0;
    [[nodiscard]] virtual CeguiLocaleSnapshot AcquireLocale(
        std::uint32_t ios) = 0;
    [[nodiscard]] virtual std::uint32_t FindClassificationFacet(
        std::uint32_t locale) = 0;
    [[nodiscard]] virtual bool IsClassicLocale(
        std::uint32_t locale) = 0;
    [[nodiscard]] virtual std::uint32_t ClassicClassificationFacet() = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseLocaleFacet(
        std::uint32_t facet) = 0;
    virtual void DestroyFacet(std::uint32_t facet,
                              std::uint8_t flags) = 0;
    [[noreturn]] virtual void ThrowMissingFacet() = 0;
};

class CeguiIstreamSentryRuntimeStub
    : public ICeguiIstreamSentryRuntime {
public:
    [[nodiscard]] std::uint32_t ReadDword(
        std::uint32_t address) override;
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override;
    [[nodiscard]] std::uint8_t ReadByte(
        std::uint32_t address) override;
    void ClearIos(std::uint32_t ios,
                  std::uint32_t flags, bool rethrow) override;
    [[nodiscard]] std::int32_t SyncStreambuf(
        std::uint32_t streambuf) override;
    [[nodiscard]] std::int32_t PeekVirtual(
        std::uint32_t streambuf) override;
    [[nodiscard]] std::int32_t ConsumeVirtual(
        std::uint32_t streambuf) override;
    [[nodiscard]] CeguiLocaleSnapshot AcquireLocale(
        std::uint32_t ios) override;
    [[nodiscard]] std::uint32_t FindClassificationFacet(
        std::uint32_t locale) override;
    [[nodiscard]] bool IsClassicLocale(
        std::uint32_t locale) override;
    [[nodiscard]] std::uint32_t ClassicClassificationFacet() override;
    [[nodiscard]] std::uint32_t ReleaseLocaleFacet(
        std::uint32_t facet) override;
    void DestroyFacet(std::uint32_t facet,
                      std::uint8_t flags) override;
    [[noreturn]] void ThrowMissingFacet() override;

protected:
    std::map<std::uint32_t, std::uint32_t> words_;
    std::map<std::uint32_t, std::uint8_t> bytes_;
};

class CeguiIstreamSentry final {
public:
    // 0x417ce0.
    [[nodiscard]] static bool Process(
        std::uint32_t stream, bool no_skip_whitespace,
        ICeguiIstreamSentryRuntime& runtime);
    // 0x417f50.
    [[nodiscard]] static std::int32_t PeekStreamBuffer(
        std::uint32_t streambuf,
        ICeguiIstreamSentryRuntime& runtime);

private:
    [[nodiscard]] static std::int32_t ConsumeStreamBuffer(
        std::uint32_t streambuf,
        ICeguiIstreamSentryRuntime& runtime);
    static void ReleaseLocale(
        CeguiLocaleSnapshot snapshot,
        ICeguiIstreamSentryRuntime& runtime);
};

}  // namespace pal4
