#pragma once

#include <cstdint>

namespace pal4 {

class IFileStreamLifecycleRuntime {
public:
    virtual ~IFileStreamLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::int32_t CloseFile(std::uint32_t handle) = 0;
    virtual void InitializeFileBuffer(std::uint32_t buffer,
                                      std::uint32_t source,
                                      std::uint32_t mode) = 0;
    virtual void ResetText(std::uint32_t text, std::uint8_t flag) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void DeleteLocaleRecord(std::uint32_t record,
                                    std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t DecrementFacet(
        std::uint32_t facet) = 0;
    virtual void DeleteFacet(std::uint32_t facet,
                             std::uint32_t flags) = 0;
    virtual void InitializeIosBase(std::uint32_t ios) = 0;
    virtual void ClearIosBase(std::uint32_t ios,
                              std::uint32_t state, bool throw_on_error) = 0;
};

class IFileStreamReleaseRuntime {
public:
    virtual ~IFileStreamReleaseRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
};

class FileStreamLifecycle final {
public:
    // IDA 0x5f86d0. ios points 96 bytes into the complete file-stream object.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t ios, IFileStreamLifecycleRuntime& runtime);
    // IDA 0x5f87f0. filebuf points 12 bytes beyond the iostream base.
    [[nodiscard]] static std::uint32_t DestroyIostreamBase(
        std::uint32_t filebuf, IFileStreamLifecycleRuntime& runtime);
    // IDA 0x5f8890. Initializes the iostream portion around a streambuf.
    [[nodiscard]] static std::uint32_t InitializeIostream(
        std::uint32_t base, std::uint32_t streambuf,
        bool initialize_virtual_base, IFileStreamLifecycleRuntime& runtime);
    // IDA 0x5f89b0: release three member pointers before the object itself.
    [[nodiscard]] static std::uint32_t FreeMembers(
        std::uint32_t object, IFileStreamReleaseRuntime& runtime);
};

}  // namespace pal4
