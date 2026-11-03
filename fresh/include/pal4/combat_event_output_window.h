#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class ICombatEventOutputRuntime {
public:
    virtual ~ICombatEventOutputRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void LoadLayout(std::uint32_t window, std::uint32_t layout) = 0;
    [[nodiscard]] virtual std::uint32_t DestructLayout(std::uint32_t window) = 0;
    virtual void SetParent(std::uint32_t window, bool visible,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterFactory(
        std::uint32_t type, std::uint32_t creator) = 0;
    virtual void InsertShowEvent(std::uint32_t tree, std::uint32_t key,
                                 std::uint32_t handler) = 0;
    virtual void DeleteHandler(std::uint32_t handler, std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t FindControl(
        std::uint32_t window, std::string_view name) = 0;
    virtual void ConstructLevelItem(std::uint32_t item,
                                    std::string_view label) = 0;
    virtual void AddComboItem(std::uint32_t combo, std::uint32_t item) = 0;
    virtual void SelectComboItem(std::uint32_t combo, std::uint32_t index,
                                 bool selected) = 0;
    [[nodiscard]] virtual std::uint32_t SubscribeComboSelection(
        std::uint32_t combo, std::uint32_t callback) = 0;
    virtual void ReleaseSubscription(std::uint32_t subscription) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTemporaryString() = 0;
    [[nodiscard]] virtual std::uint32_t GetSelectedComboItem(
        std::uint32_t combo) = 0;
    [[nodiscard]] virtual std::string ReadCString(std::uint32_t address) = 0;
    virtual void CopyString(std::uint32_t destination,
                            std::string_view text) = 0;
    virtual void CleanupString(std::uint32_t record) = 0;
    virtual void SetText(std::uint32_t control,
                         const std::u32string& text) = 0;
    [[nodiscard]] virtual std::uint32_t EventRouter() = 0;
    [[nodiscard]] virtual std::uint32_t RegisterRouterEvent(
        std::uint32_t router, std::uint32_t mode, std::uint32_t key,
        std::uint32_t owner, std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t RemoveRouterEvent(
        std::uint32_t router, std::uint32_t key,
        std::uint32_t callback) = 0;
};

class CombatEventOutputWindow final {
public:
    static constexpr std::uint32_t kVtable = 0x84568cu;
    static constexpr std::uint32_t kComboCallbackVtable = 0x8456a4u;
    static constexpr std::uint32_t kComboCallback = 0x555d90u;
    static constexpr std::uint32_t kHandledCallback = 0x555940u;
    static constexpr std::uint32_t kUnhandledCallback = 0x555a10u;
    static constexpr std::uint32_t kHandledKeyPointer = 0x8b8514u;
    static constexpr std::uint32_t kUnhandledKeyPointer = 0x8b8518u;

    [[nodiscard]] static std::uint32_t Construct(std::uint32_t window,
        std::uint32_t layout, ICombatEventOutputRuntime& runtime);
    [[nodiscard]] static std::uint32_t DeletingDestructor(
        std::uint32_t window, std::uint8_t flags,
        ICombatEventOutputRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t window, ICombatEventOutputRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetVisible(
        std::uint32_t window, bool visible, std::uint32_t parent,
        ICombatEventOutputRuntime& runtime);
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t window, ICombatEventOutputRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterEvents(
        std::uint32_t window, ICombatEventOutputRuntime& runtime);
    [[nodiscard]] static std::uint32_t ClearEvents(
        std::uint32_t window, ICombatEventOutputRuntime& runtime);
    static void HandleEventHandled(std::uint32_t window, std::uint32_t event,
                                   ICombatEventOutputRuntime& runtime);
    static void HandleEventUnhandled(std::uint32_t window, std::uint32_t event,
                                     ICombatEventOutputRuntime& runtime);
    static void RebuildText(std::uint32_t window,
                            ICombatEventOutputRuntime& runtime);
    [[nodiscard]] static std::uint8_t ClearEntries(
        std::uint32_t window, ICombatEventOutputRuntime& runtime);
};

class CombatEventOutputFactory final {
public:
    static constexpr std::uint32_t kTypePointer = 0x8b691cu;
    static constexpr std::uint32_t kTypeCache = 0x8f95bcu;
    static constexpr std::uint32_t kCreateCallback = 0x555120u;
    static constexpr std::uint32_t kShowEventKey = 0x8b68f0u;
    static constexpr std::uint32_t kShowEventSentinel = 0x8f95b8u;

    [[nodiscard]] static std::uint32_t GetTypeThunk(
        ICombatEventOutputRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetType(
        ICombatEventOutputRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t layout, ICombatEventOutputRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactoryThunk(
        ICombatEventOutputRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterFactory(
        ICombatEventOutputRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterShowEventThunk(
        ICombatEventOutputRuntime& runtime);
    [[nodiscard]] static std::uint32_t RegisterShowEvent(
        ICombatEventOutputRuntime& runtime);
};

}  // namespace pal4
