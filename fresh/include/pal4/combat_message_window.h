#pragma once

#include "pal4/combat_message_subscriptions.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

class ICombatMessageWindowRuntime : public ICombatMessageSubscriptionRuntime {
public:
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    virtual void LoadLayout(std::uint32_t message, std::string_view layout,
                            std::string_view prefix,
                            std::string_view override_name) = 0;
    [[nodiscard]] virtual std::uint32_t DestructLayout(
        std::uint32_t message) = 0;
    virtual void SetParent(std::uint32_t message, bool visible,
                           std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::string ReadCString(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        const std::string& source) = 0;
    [[nodiscard]] virtual std::uint32_t GetWindowByName(
        std::uint32_t message, const std::string& utf8_name) = 0;
    virtual void SetNormalTextColour(std::uint32_t window,
                                     float red, float green,
                                     float blue, float alpha) = 0;
    virtual void SetText(std::uint32_t window,
                         std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t CleanupTemporaryString() = 0;
    virtual void CleanupMessageString(std::uint32_t record) = 0;
    virtual void CopyMessageString(std::uint32_t destination,
                                   std::uint32_t source) = 0;
    [[nodiscard]] virtual std::u32string DecodeUtf8(
        const std::string& utf8) = 0;
    [[nodiscard]] virtual std::u32string ReadEditText(
        std::uint32_t window) = 0;
    virtual void SetUnicodeText(std::uint32_t window,
                                const std::u32string& text) = 0;
    virtual void SetCaretIndex(std::uint32_t window,
                               std::uint32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindFrame(
        std::uint32_t manager, std::uint32_t type) = 0;
    virtual void RegisterFrame(std::uint32_t manager,
                               std::uint32_t type) = 0;
    virtual void SetFrameVisible(std::uint32_t frame, bool visible,
                                 std::uint32_t parent) = 0;
};

class CombatMessageStringRecord final {
public:
    // IDA 0x54f1e0 and 0x54f230.
    [[nodiscard]] static std::uint32_t CopyConstruct(
        std::uint32_t destination, std::uint32_t source,
        ICombatMessageWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t record, std::uint32_t flags,
        ICombatMessageWindowRuntime& runtime);
};

class CombatMessageWindow final {
public:
    static constexpr std::uint32_t kObjectSize = 0x44u;
    static constexpr std::uint32_t kVtable = 0x845548u;

    // IDA 0x54e8a0-0x54ea20.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t message, std::uint32_t ignored_argument,
        ICombatMessageWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t message, ICombatMessageWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetVisible(
        std::uint32_t message, bool visible, std::uint32_t parent,
        ICombatMessageWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t message, ICombatMessageWindowRuntime& runtime);
    static void AddMessage(std::uint32_t message, std::uint32_t source_record,
                           ICombatMessageWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t InsertMessageRecord(
        std::uint32_t message, std::uint32_t source_record,
        ICombatMessageWindowRuntime& runtime);
    [[nodiscard]] static bool Show(std::uint32_t event,
                                   ICombatMessageWindowRuntime& runtime);
    static void HandleEvent(std::uint32_t message, std::uint32_t event,
                            ICombatMessageWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t ClearMessages(
        std::uint32_t message, ICombatMessageWindowRuntime& runtime);
};

}  // namespace pal4
