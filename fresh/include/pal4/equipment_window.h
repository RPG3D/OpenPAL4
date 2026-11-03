#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace pal4 {

struct EquipmentWindowState {
    std::array<std::uint8_t, 0x134> bytes{};

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const noexcept {
        std::uint32_t value{};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }
    void Write(std::uint32_t offset, std::uint32_t value) noexcept {
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
    }
};

class IEquipmentWindowRuntime {
public:
    virtual ~IEquipmentWindowRuntime() = default;
    virtual void LoadLayout(EquipmentWindowState& state, std::uint32_t name,
                            std::uint32_t first, std::uint32_t second) = 0;
    virtual void ConstructMember(EquipmentWindowState& state,
                                 std::uint32_t offset) = 0;
    virtual void CleanupMember(EquipmentWindowState& state,
                               std::uint32_t offset) = 0;
    virtual void ConstructNameNode(EquipmentWindowState& state) = 0;
    virtual void ResetNameNode(EquipmentWindowState& state) = 0;
    virtual void DestroyDialogBase(EquipmentWindowState& state) = 0;
    virtual void FreeBuffer(std::uint32_t pointer) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateCallback(
        std::uint32_t bytes) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    virtual void FreeCallback(std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t FrameManager() = 0;
    virtual void InsertOrReplace(std::uint32_t tree,
                                 std::string_view command,
                                 std::uint32_t& callback) = 0;
    virtual void DeleteCallback(std::uint32_t callback,
                                std::uint32_t flags) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindowByName(
        std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::string_view name) = 0;
    virtual void SetFont(std::uint32_t window,
                         std::string_view font) = 0;
    virtual void SetFormatting(std::uint32_t window,
                               std::uint32_t horizontal,
                               std::uint32_t vertical) = 0;
    [[nodiscard]] virtual std::uint32_t Subscribe(
        std::uint32_t window, std::string_view event,
        std::uint32_t callback) = 0;
    [[nodiscard]] virtual std::uint32_t ReferenceCount(
        std::uint32_t reference) = 0;
    virtual void SetReferenceCount(std::uint32_t reference,
                                   std::uint32_t count) = 0;
    virtual void DeleteReference(std::uint32_t reference,
                                 std::uint32_t flags) = 0;
    virtual void SetupMouseEventHandlers(std::uint32_t member,
                                         std::uint32_t window) = 0;
    virtual void ActivateMember(std::uint32_t member) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window, bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t ColumnCount(
        std::uint32_t list) = 0;
    virtual void RemoveColumn(std::uint32_t list,
                              std::uint32_t index) = 0;
    virtual void SetUserColumnSizingEnabled(std::uint32_t list,
                                             bool enabled) = 0;
    virtual void SetUserColumnDraggingEnabled(std::uint32_t list,
                                               bool enabled) = 0;
    virtual void SetUserSortControlEnabled(std::uint32_t list,
                                            bool enabled) = 0;
    virtual void AddColumn(std::uint32_t list, std::string_view title,
                           std::uint32_t id, float width) = 0;
    virtual void SetColumnLabelImage(std::uint32_t list,
                                     std::uint32_t column,
                                     std::string_view image,
                                     std::string_view label) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t GetImage(
        std::string_view imageset, std::string_view image) = 0;
};

class EquipmentWindowRuntimeStub : public IEquipmentWindowRuntime {
public:
    void LoadLayout(EquipmentWindowState&, std::uint32_t,
                    std::uint32_t, std::uint32_t) override {}
    void ConstructMember(EquipmentWindowState&, std::uint32_t) override {}
    void CleanupMember(EquipmentWindowState&, std::uint32_t) override {}
    void ConstructNameNode(EquipmentWindowState&) override {}
    void ResetNameNode(EquipmentWindowState&) override {}
    void DestroyDialogBase(EquipmentWindowState&) override {}
    void FreeBuffer(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t AllocateCallback(std::uint32_t) override {
        return 0u;
    }
    void WriteWord(std::uint32_t, std::uint32_t) override {}
    void FreeCallback(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameManager() override { return 0u; }
    void InsertOrReplace(std::uint32_t, std::string_view,
                         std::uint32_t&) override {}
    void DeleteCallback(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FindWindowByName(
        std::string_view) override { return 0u; }
    [[nodiscard]] std::uint32_t FindWindow(
        std::string_view) override { return 0u; }
    void SetFont(std::uint32_t, std::string_view) override {}
    void SetFormatting(std::uint32_t, std::uint32_t,
                       std::uint32_t) override {}
    [[nodiscard]] std::uint32_t Subscribe(
        std::uint32_t, std::string_view, std::uint32_t) override { return 0u; }
    [[nodiscard]] std::uint32_t ReferenceCount(
        std::uint32_t) override { return 0u; }
    void SetReferenceCount(std::uint32_t, std::uint32_t) override {}
    void DeleteReference(std::uint32_t, std::uint32_t) override {}
    void SetupMouseEventHandlers(std::uint32_t,
                                 std::uint32_t) override {}
    void ActivateMember(std::uint32_t) override {}
    void SetAlwaysOnTop(std::uint32_t, bool) override {}
    [[nodiscard]] std::uint32_t ColumnCount(
        std::uint32_t) override { return 0u; }
    void RemoveColumn(std::uint32_t, std::uint32_t) override {}
    void SetUserColumnSizingEnabled(std::uint32_t, bool) override {}
    void SetUserColumnDraggingEnabled(std::uint32_t, bool) override {}
    void SetUserSortControlEnabled(std::uint32_t, bool) override {}
    void AddColumn(std::uint32_t, std::string_view,
                   std::uint32_t, float) override {}
    void SetColumnLabelImage(std::uint32_t, std::uint32_t,
                             std::string_view, std::string_view) override {}
    void SetEnabled(std::uint32_t, bool) override {}
    [[nodiscard]] std::uint32_t GetImage(
        std::string_view, std::string_view) override { return 0u; }
};

class EquipmentWindow final {
public:
    static constexpr std::uint32_t kObjectVtable = 0x843838u;
    static constexpr std::uint32_t kPrimaryMemberVtable = 0x843850u;
    static constexpr std::uint32_t kNameNodeVtable = 0x8438acu;
    static constexpr std::uint32_t kSecondaryMemberVtable = 0x843878u;
    static constexpr std::uint32_t kCommandCallbackVtable = 0x84382cu;
    static constexpr std::uint32_t kEventCallbackVtable = 0x843824u;

    [[nodiscard]] static EquipmentWindowState& Construct(
        EquipmentWindowState& state, std::uint32_t object_address,
        std::uint32_t name, IEquipmentWindowRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        EquipmentWindowState& state, std::uint32_t object_address,
        IEquipmentWindowRuntime& runtime);
};

}  // namespace pal4
