#pragma once

#include <cstdint>

namespace pal4 {

class ICsbBufferRuntime {
public:
    virtual ~ICsbBufferRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CreateContext(
        std::uint32_t type, std::uint32_t access,
        const char* source) = 0;
    virtual void ReadSize(std::uint32_t context,
                          std::uint32_t& size) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t size) = 0;
    virtual void ReadBytes(std::uint32_t context,
                           std::uint32_t destination,
                           std::uint32_t size) = 0;
    virtual void CloseContext(std::uint32_t context,
                              std::uint32_t output) = 0;
};

class CsbBufferRuntimeStub final : public ICsbBufferRuntime {
public:
    [[nodiscard]] std::uint32_t CreateContext(
        std::uint32_t, std::uint32_t, const char*) override { return 0u; }
    void ReadSize(std::uint32_t, std::uint32_t&) override {}
    [[nodiscard]] std::uint32_t Allocate(
        std::uint32_t) override { return 0u; }
    void ReadBytes(std::uint32_t, std::uint32_t,
                   std::uint32_t) override {}
    void CloseContext(std::uint32_t, std::uint32_t) override {}
};

class CsbBufferReader final {
public:
    // IDA 0x600d50.
    [[nodiscard]] static bool Read(const char* source,
                                   std::uint32_t& size,
                                   std::uint32_t& data,
                                   ICsbBufferRuntime& runtime);
};

}  // namespace pal4
