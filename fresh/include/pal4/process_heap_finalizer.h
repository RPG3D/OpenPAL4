#pragma once

#include <cstdint>

namespace pal4 {

class IProcessHeapFinalizerRuntime {
public:
    virtual ~IProcessHeapFinalizerRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadNext(std::uint32_t node) const = 0;
    [[nodiscard]] virtual std::uint32_t GetProcessHeap() = 0;
    [[nodiscard]] virtual std::uint32_t FreeFromHeap(
        std::uint32_t heap, std::uint32_t node) = 0;
};

class ProcessHeapFinalizer final {
public:
    explicit ProcessHeapFinalizer(std::uint32_t& head) : head_(head) {}

    [[nodiscard]] std::uint32_t Finalize(
        IProcessHeapFinalizerRuntime& runtime);

private:
    std::uint32_t& head_;
};

}  // namespace pal4
