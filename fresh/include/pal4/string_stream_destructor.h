#pragma once

#include <cstdint>

namespace pal4 {

class IStringStreamDestructorRuntime {
public:
    virtual ~IStringStreamDestructorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void DestroyLocale(std::uint32_t function,
                               std::uint32_t locale,
                               std::uint32_t flags) = 0;
    virtual void DestroyFileStream(std::uint32_t address) = 0;
};

class StringStreamDestructorRuntimeStub final
    : public IStringStreamDestructorRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(
        std::uint32_t) override { return 0u; }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void Free(std::uint32_t) override {}
    void DestroyLocale(std::uint32_t, std::uint32_t,
                       std::uint32_t) override {}
    void DestroyFileStream(std::uint32_t) override {}
};

class StringStreamDestructor final {
public:
    // IDA 0x600db0; object is the embedded streambuf at base +0x58.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t streambuf,
        IStringStreamDestructorRuntime& runtime);
};

}  // namespace pal4
