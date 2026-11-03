#pragma once

#include <cstdint>

namespace pal4 {

class IShowSettleSelectRuntime {
public:
    virtual ~IShowSettleSelectRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void InsertOrReplace(std::uint32_t tree,
                                 std::uint32_t key,
                                 std::uint32_t handler) = 0;
    virtual void ReleaseHandler(std::uint32_t handler,
                                std::uint32_t flags) = 0;
};

class UiFrameManagerShowSettleSelect final {
public:
    static constexpr std::uint32_t kHandlerStorage = 0x8e796cu;
    static constexpr std::uint32_t kKey = 0x8a80c8u;
    static constexpr std::uint32_t kHandlerVtable = 0x8435dcu;
    static constexpr std::uint32_t kSetVisible = 0x47dab0u;

    [[nodiscard]] static std::uint32_t GetHandler(
        IShowSettleSelectRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterHandler(
        IShowSettleSelectRuntime& runtime);
};

}  // namespace pal4
