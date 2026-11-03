#pragma once

#include <cstdint>

namespace pal4 {

struct CheckedIteratorState {
    std::uint32_t proxy{};
};

class ICheckedIteratorRuntime {
public:
    virtual ~ICheckedIteratorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadContainer(
        std::uint32_t proxy) = 0;
};

class CheckedIteratorAccess final {
public:
    // 0x5eac70: _Iterator_base12::_Getcont.
    [[nodiscard]] static std::uint32_t Container(
        const CheckedIteratorState& iterator,
        ICheckedIteratorRuntime& runtime);
};

}  // namespace pal4
