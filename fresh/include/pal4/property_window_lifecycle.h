#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace pal4 {

struct PropertyWindowState {
    std::array<std::uint8_t, 0xb8> bytes{};

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const noexcept {
        std::uint32_t value{};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }
    void Write(std::uint32_t offset, std::uint32_t value) noexcept {
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
    }
};

enum class PropertyWindowEvent { MouseClick, HoverRowChanged, Clicked };

enum class PropertyWindowInitStage {
    Layout,
    Member,
    NameNode,
    UiObject,
};

class IPropertyWindowLifecycleRuntime {
public:
    virtual ~IPropertyWindowLifecycleRuntime() = default;
    virtual void LoadLayout(PropertyWindowState& state,
                            std::uint32_t name) = 0;
    virtual void InitializeMember(PropertyWindowState& state) = 0;
    virtual void InitializeNameNode(PropertyWindowState& state) = 0;
    virtual void InitializeUiObject(PropertyWindowState& state) = 0;
    virtual void Cleanup(PropertyWindowState& state,
                         PropertyWindowInitStage stage) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectAddress(
        const PropertyWindowState& state) = 0;
    virtual void RegisterCommand(std::string_view command,
                                 std::uint32_t handler,
                                 std::uint32_t owner,
                                 std::uint32_t callback_vtable) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::string_view path) = 0;
    virtual void Subscribe(std::uint32_t window,
                           PropertyWindowEvent event,
                           std::uint32_t handler,
                           std::uint32_t owner,
                           std::uint32_t callback_vtable) = 0;
    virtual void SetButtonTag(std::uint32_t button,
                              std::uint32_t tag) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetEnabled(std::uint32_t window, bool enabled) = 0;
    [[nodiscard]] virtual std::uint32_t ColumnCount(
        std::uint32_t list) = 0;
    virtual void RemoveColumn(std::uint32_t list,
                              std::uint32_t column) = 0;
    virtual void AddColumn(std::uint32_t list, std::string_view title,
                           std::uint32_t id, float width) = 0;
    virtual void SetColumnLabelImage(std::uint32_t list,
                                     std::uint32_t id,
                                     std::string_view imageset,
                                     std::string_view image) = 0;
    virtual void ConfigureList(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::uint32_t Image(
        std::string_view imageset, std::string_view image) = 0;
    virtual void SetupMouseHandlers(PropertyWindowState& state,
                                    std::uint32_t button) = 0;
    virtual void ResetNameNode(PropertyWindowState& state) = 0;
    virtual void FreeMemberBuffer(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyDialogBase(
        PropertyWindowState& state) = 0;
};

class PropertyWindowLifecycle final {
public:
    static constexpr std::uint32_t kObjectVtable = 0x843a2cu;
    static constexpr std::uint32_t kInitialMemberVtable = 0x843628u;
    static constexpr std::uint32_t kMemberVtable = 0x843a44u;
    static constexpr std::uint32_t kCommandVtable = 0x843a20u;
    static constexpr std::uint32_t kEventVtable = 0x843a18u;

    [[nodiscard]] static PropertyWindowState& Construct(
        PropertyWindowState& state, std::uint32_t name,
        IPropertyWindowLifecycleRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        PropertyWindowState& state,
        IPropertyWindowLifecycleRuntime& runtime);
};

}  // namespace pal4
