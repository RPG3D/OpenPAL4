#pragma once

#include <cstdint>

namespace pal4 {

class IDNameNodeRuntime {
public:
    virtual ~IDNameNodeRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
};

class DNameNode final {
public:
    // IDA 0x639f60: initialize the two-word name handle.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t node, IDNameNodeRuntime& runtime);

    // D3DX resource methods at 0x6972ae/0x6972bb/0x699423 are library-owned.

};

class IDNameNodeEventRuntime : public IDNameNodeRuntime {
public:
    ~IDNameNodeEventRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t StringLength(
        std::uint32_t text) = 0;
    virtual void SetErrorFlag(std::uint32_t record,
                              std::uint8_t value) = 0;
    virtual void ReallocateString(std::uint32_t record,
                                  std::uint32_t length) = 0;
    virtual void CopyBytes(std::uint32_t destination,
                           std::uint32_t source,
                           std::uint32_t length) = 0;
};

class DNameNodeEventRecord final {
public:
    static constexpr std::uint32_t kSize = 24u;

    [[nodiscard]] static std::uint32_t SetString(
        std::uint32_t record, std::uint32_t first,
        std::uint32_t second, std::uint32_t text,
        IDNameNodeEventRuntime& runtime);
};

}  // namespace pal4
