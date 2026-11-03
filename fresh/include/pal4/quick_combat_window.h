#pragma once

#include <cstdint>

namespace pal4 {

class IQuickCombatWindowRuntime {
public:
    virtual ~IQuickCombatWindowRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void LoadLayout(std::uint32_t window,
                            std::uint32_t layout) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterFactory(
        std::uint32_t type, std::uint32_t creator) = 0;
    virtual void InsertShowEvent(std::uint32_t tree, std::uint32_t key,
                                 std::uint32_t handler) = 0;
    virtual void DeleteHandler(std::uint32_t handler,
                               std::uint32_t flags) = 0;
};

class QuickCombatWindow final {
public:
    static constexpr std::uint32_t kVtable = 0x8456acu;
    // IDA 0x556120.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t window, std::uint32_t layout,
        IQuickCombatWindowRuntime& runtime);
};

class QuickCombatFactory final {
public:
    static constexpr std::uint32_t kTypePointer = 0x8b76bcu;
    static constexpr std::uint32_t kTypeCache = 0x8f95c4u;
    static constexpr std::uint32_t kCreateCallback = 0x555fe0u;
    static constexpr std::uint32_t kShowEventKey = 0x8b6904u;
    static constexpr std::uint32_t kShowEventSentinel = 0x8f95c0u;

    // IDA 0x555fc0-0x556080.
    [[nodiscard]] static std::uint32_t GetTypeThunk(
        IQuickCombatWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetType(
        IQuickCombatWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t layout, IQuickCombatWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactoryThunk(
        IQuickCombatWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactory(
        IQuickCombatWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterShowEventThunk(
        IQuickCombatWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterShowEvent(
        IQuickCombatWindowRuntime& runtime);
};

}  // namespace pal4
