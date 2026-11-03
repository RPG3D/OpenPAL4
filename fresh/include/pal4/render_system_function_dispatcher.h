#pragma once

#include <cstdint>

namespace pal4 {

class IRenderSystemFunctionRuntime {
public:
    virtual ~IRenderSystemFunctionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetRenderSystem() = 0;
    [[nodiscard]] virtual std::uint32_t ReadSlot(
        std::uint32_t system, std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::uint32_t Invoke(
        std::uint32_t function, std::uint32_t argument) = 0;
};

class RenderSystemFunctionDispatcher final {
public:
    explicit RenderSystemFunctionDispatcher(
        IRenderSystemFunctionRuntime& runtime)
        : runtime_(runtime) {}

    [[nodiscard]] std::uint32_t DispatchKeyboard(
        std::uint32_t code, std::uint32_t argument);
    [[nodiscard]] std::uint32_t DispatchMouse(
        std::uint32_t code, std::uint32_t argument);

    [[nodiscard]] std::uint32_t Call48(std::uint32_t argument);
    [[nodiscard]] std::uint32_t Call52(std::uint32_t argument);
    [[nodiscard]] std::uint32_t Call56(std::uint32_t argument);
    [[nodiscard]] std::uint32_t Call60(std::uint32_t argument);
    [[nodiscard]] std::uint32_t Call64(std::uint32_t argument);
    [[nodiscard]] std::uint32_t Call68(std::uint32_t argument);
    [[nodiscard]] std::uint32_t Call72(std::uint32_t argument);
    [[nodiscard]] std::uint32_t Call76(std::uint32_t argument);
    [[nodiscard]] std::uint32_t Call80(std::uint32_t argument);
    [[nodiscard]] std::uint32_t Call84(std::uint32_t argument);
    [[nodiscard]] std::uint32_t Call88(std::uint32_t argument);
    [[nodiscard]] std::uint32_t Call92(std::uint32_t argument);

private:
    [[nodiscard]] std::uint32_t CallSlot(std::uint32_t offset,
                                         std::uint32_t argument);
    IRenderSystemFunctionRuntime& runtime_;
};

}  // namespace pal4
