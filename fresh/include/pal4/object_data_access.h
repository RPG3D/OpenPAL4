#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class IObjectMethodRuntime {
public:
    virtual ~IObjectMethodRuntime() = default;
    [[nodiscard]] virtual std::int32_t CallVirtual(
        std::uint32_t receiver, std::uint32_t slot_offset,
        std::uint32_t context) = 0;
};

class ObjectMethodRuntimeStub : public IObjectMethodRuntime {
public:
    std::int32_t CallVirtual(std::uint32_t, std::uint32_t,
                             std::uint32_t) override;
};

class ObjectMethodForwarder final {
public:
    // IDA 0x440300 and 0x440310.
    [[nodiscard]] static std::int32_t CallSlot10(
        std::uint32_t context, std::uint32_t receiver,
        IObjectMethodRuntime& runtime);
    [[nodiscard]] static std::int32_t CallSlot28(
        std::uint32_t context, std::uint32_t receiver,
        IObjectMethodRuntime& runtime);
};

struct IndexedDataRecord {
    std::uint32_t vtable{};
    std::array<std::uint32_t, 8> values{};

    // IDA 0x440320.
    void ResetVtable();
    // IDA 0x440330.
    [[nodiscard]] std::uint32_t GetElement(std::uint32_t index) const;
};
static_assert(sizeof(IndexedDataRecord) == 0x24u);

}  // namespace pal4
