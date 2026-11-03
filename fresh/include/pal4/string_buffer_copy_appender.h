#pragma once

#include <cstdint>
#include <string_view>

namespace pal4 {

class IStringBufferCopyAppendRuntime {
public:
    virtual ~IStringBufferCopyAppendRuntime() = default;
    [[nodiscard]] virtual std::string_view CopySource(
        std::uint32_t source) = 0;
    [[nodiscard]] virtual std::uint32_t TailLength(
        std::uint32_t tail) = 0;
    [[nodiscard]] virtual bool Allocate(std::uint32_t length) = 0;
    [[nodiscard]] virtual std::string_view TailBytes(
        std::uint32_t tail) = 0;
    virtual void WriteDestination(std::uint32_t destination,
                                  std::string_view value) = 0;
    virtual void ReleaseTemporary(std::string_view value) = 0;
};

class StringBufferCopyAppendRuntimeStub final
    : public IStringBufferCopyAppendRuntime {
public:
    [[nodiscard]] std::string_view CopySource(std::uint32_t) override {
        return {};
    }
    [[nodiscard]] std::uint32_t TailLength(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] bool Allocate(std::uint32_t) override { return false; }
    [[nodiscard]] std::string_view TailBytes(std::uint32_t) override {
        return {};
    }
    void WriteDestination(std::uint32_t, std::string_view) override {}
    void ReleaseTemporary(std::string_view) override {}
};

class StringBufferCopyAppender final {
public:
    // IDA 0x51b600.
    [[nodiscard]] static std::uint32_t CopyAndAppend(
        std::uint32_t destination, std::uint32_t source,
        std::uint32_t tail, IStringBufferCopyAppendRuntime& runtime);
    // IDA 0x60bd10. For this entry the runtime measures tail with strlen.
    [[nodiscard]] static std::uint32_t CopyAndAppendCString(
        std::uint32_t destination, std::uint32_t source,
        std::uint32_t c_string, IStringBufferCopyAppendRuntime& runtime);
};

}  // namespace pal4
