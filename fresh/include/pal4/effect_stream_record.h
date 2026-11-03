#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct EffectStreamRecord final {
    std::array<std::uint32_t, 15u> words{};
};
static_assert(sizeof(EffectStreamRecord) == 60u);

class IEffectStreamRecordRuntime {
public:
    virtual ~IEffectStreamRecordRuntime() = default;
    virtual void ReadData(std::uint32_t context,
                          std::uint32_t word_offset,
                          std::uint32_t bytes,
                          EffectStreamRecord& record) = 0;
    virtual void ReadString(std::uint32_t context,
                            std::uint32_t word_offset,
                            EffectStreamRecord& record) = 0;
};

class EffectStreamRecordReader final {
public:
    // IDA 0x64bc70: deserialize seven fields in native order.
    static void Read(EffectStreamRecord& record, std::uint32_t context,
                     IEffectStreamRecordRuntime& runtime);
};

}  // namespace pal4
