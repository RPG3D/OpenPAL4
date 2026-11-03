#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct CallbackCollectionState {
    static constexpr std::size_t kFieldCount = 10;
    using Fields = std::array<std::uint32_t, kFieldCount>;

    Fields fields{};

    [[nodiscard]] std::uint32_t Read(std::size_t offset) const noexcept;
    void Write(std::size_t offset, std::uint32_t value) noexcept;
};

class ICallbackCollectionRuntime {
public:
    virtual ~ICallbackCollectionRuntime() = default;

    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ReadDword(
        std::uint32_t address) = 0;
    virtual void WriteDword(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeFactory(
        std::uint32_t table_address) = 0;
    [[nodiscard]] virtual std::uint32_t InvokeInsert(
        std::uint32_t callback_address, std::uint32_t ecx,
        std::uint32_t argument1, std::uint32_t argument2) = 0;
};

class CallbackCollectionElementInitializer final {
public:
    [[nodiscard]] static bool AllocateAndInitialize(
        CallbackCollectionState& collection, std::uint32_t self_address,
        ICallbackCollectionRuntime& runtime);
};

}  // namespace pal4
