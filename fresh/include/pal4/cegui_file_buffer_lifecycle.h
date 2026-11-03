#pragma once

#include <cstdint>
#include <map>

namespace pal4 {

class ICeguiFileBufferRuntime {
public:
    virtual ~ICeguiFileBufferRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address,
                            std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address,
                           std::uint8_t value) = 0;
    [[nodiscard]] virtual std::int32_t CloseCrtFile(
        std::uint32_t file) = 0;
    virtual void InitializeWideStreambuf(std::uint32_t object) = 0;
    [[nodiscard]] virtual std::uint32_t GlobalLocaleValue() = 0;
    virtual void SetErrorFlag(std::uint32_t object,
                              std::uint8_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyReference(
        std::uint32_t object, std::uint8_t flags) = 0;
    virtual void InitializeFileBuffer(
        std::uint32_t filebuf, std::uint32_t first,
        std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t DecrementLocaleFacet(
        std::uint32_t facet) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeLocale() = 0;
    [[nodiscard]] virtual std::uint32_t GlobalLocaleObject() = 0;
    virtual void ConfigureFilePatterns(std::uint32_t filebuf,
                                       std::uint32_t file) = 0;
    [[nodiscard]] virtual std::uint32_t OpenCrtFile(
        std::uint32_t name, std::uint32_t mode) = 0;
    virtual void AcquireCodecvtFacet(std::uint32_t filebuf) = 0;
};

class CeguiFileBufferRuntimeStub : public ICeguiFileBufferRuntime {
public:
    [[nodiscard]] std::uint32_t ReadDword(
        std::uint32_t address) override;
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override;
    [[nodiscard]] std::uint8_t ReadByte(
        std::uint32_t address) override;
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override;
    [[nodiscard]] std::int32_t CloseCrtFile(
        std::uint32_t file) override;
    void InitializeWideStreambuf(std::uint32_t object) override;
    [[nodiscard]] std::uint32_t GlobalLocaleValue() override;
    void SetErrorFlag(std::uint32_t object,
                      std::uint8_t value) override;
    void Free(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t DestroyReference(
        std::uint32_t object, std::uint8_t flags) override;
    void InitializeFileBuffer(std::uint32_t filebuf,
                              std::uint32_t first,
                              std::uint32_t mode) override;
    [[nodiscard]] std::uint32_t DecrementLocaleFacet(
        std::uint32_t facet) override;
    [[nodiscard]] std::uint32_t InitializeLocale() override;
    [[nodiscard]] std::uint32_t GlobalLocaleObject() override;
    void ConfigureFilePatterns(std::uint32_t filebuf,
                               std::uint32_t file) override;
    [[nodiscard]] std::uint32_t OpenCrtFile(
        std::uint32_t name, std::uint32_t mode) override;
    void AcquireCodecvtFacet(std::uint32_t filebuf) override;

private:
    std::map<std::uint32_t, std::uint32_t> words_;
    std::map<std::uint32_t, std::uint8_t> bytes_;
};

class CeguiFileBufferLifecycle final {
public:
    static constexpr std::uint32_t kFilebufVtable = 0x8682d8u;
    static constexpr std::uint32_t kStreambufVtable = 0x868310u;
    static constexpr std::uint32_t kIstreamSubobjectVtable = 0x842160u;
    static constexpr std::uint32_t kIstreamVtable = 0x8682c0u;

    // 0x4182b0, 0x418440, 0x418460.
    [[nodiscard]] static std::uint32_t ConstructFileBuffer(
        std::uint32_t object, std::uint32_t file,
        ICeguiFileBufferRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetGetArea(
        std::uint32_t object, std::uint32_t begin,
        std::uint32_t current, std::uint32_t end,
        ICeguiFileBufferRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetPutArea(
        std::uint32_t object, std::uint32_t begin,
        std::uint32_t end, ICeguiFileBufferRuntime& runtime);
    // 0x418480, 0x418550.
    [[nodiscard]] static std::uint32_t Open(
        std::uint32_t object, std::uint32_t name,
        std::uint32_t mode, ICeguiFileBufferRuntime& runtime);
    [[nodiscard]] static std::uint32_t Close(
        std::uint32_t object, ICeguiFileBufferRuntime& runtime);
    // 0x418670, 0x418750.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t object, std::uint32_t file,
        std::uint32_t mode, ICeguiFileBufferRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeDefaultLocale(
        std::uint32_t locale_slot, ICeguiFileBufferRuntime& runtime);
    // 0x418980, 0x4189e0.
    [[nodiscard]] static std::uint32_t InitializeStreambufPointers(
        std::uint32_t object, ICeguiFileBufferRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetPatternPointers(
        std::uint32_t object, std::uint32_t get_begin,
        std::uint32_t get_current, std::uint32_t get_count,
        std::uint32_t put_begin, std::uint32_t put_current,
        std::uint32_t put_count, ICeguiFileBufferRuntime& runtime);

    // 0x416ca0, 0x416da0.
    [[nodiscard]] static std::uint32_t DestroyFileBuffer(
        std::uint32_t object, ICeguiFileBufferRuntime& runtime);
    [[nodiscard]] static std::uint32_t DestroyStreambuf(
        std::uint32_t object, ICeguiFileBufferRuntime& runtime);
    // 0x4177e0: `this` points to the istream subobject at filebuf + 84.
    [[nodiscard]] static std::uint32_t DestroyFileStreamSubobjects(
        std::uint32_t istream_subobject,
        ICeguiFileBufferRuntime& runtime);
    // 0x417930.
    [[nodiscard]] static std::uint32_t DestroyStreambufWithFlag(
        std::uint32_t object, std::uint8_t flags,
        ICeguiFileBufferRuntime& runtime);
    // 0x417990.
    [[nodiscard]] static std::uint32_t DestroyFileBufferWithFlag(
        std::uint32_t object, std::uint8_t flags,
        ICeguiFileBufferRuntime& runtime);

    // 0x416de0 and 0x416df0.
    [[nodiscard]] static std::int32_t ReturnNegativeOne(
        std::uint32_t ignored) noexcept;
    [[nodiscard]] static std::int32_t Underflow() noexcept;

private:
    [[nodiscard]] static std::uint32_t ReleaseReference(
        std::uint32_t object, ICeguiFileBufferRuntime& runtime);
    static void IncrementGlobalLocaleReference(
        ICeguiFileBufferRuntime& runtime);
    static void ResetBufferPointers(
        std::uint32_t object, bool mark_open,
        ICeguiFileBufferRuntime& runtime);
};

}  // namespace pal4
