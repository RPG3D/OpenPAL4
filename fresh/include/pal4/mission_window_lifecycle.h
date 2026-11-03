#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

namespace pal4 {

struct MissionWindowState {
    std::array<std::uint8_t, 0x8c> bytes{};

    [[nodiscard]] std::uint32_t Read(std::uint32_t offset) const noexcept {
        std::uint32_t value{};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }
    void Write(std::uint32_t offset, std::uint32_t value) noexcept {
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
    }
};

enum class MissionWindowEvent {
    Clicked,
    HoverRowChanged,
    MouseClick,
};

class IMissionWindowLifecycleRuntime {
public:
    virtual ~IMissionWindowLifecycleRuntime() = default;
    virtual void LoadLayout(MissionWindowState& state, std::uint32_t name,
                            std::uint32_t first_flag,
                            std::uint32_t second_flag) = 0;
    virtual void InitializeMember(MissionWindowState& state) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectAddress(
        const MissionWindowState& state) = 0;
    [[nodiscard]] virtual std::uint32_t FindWindow(
        std::string_view path, bool by_name) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window, bool value) = 0;
    virtual void Subscribe(std::uint32_t window, MissionWindowEvent event,
                           std::uint32_t handler, std::uint32_t owner,
                           std::uint32_t callback_bytes,
                           std::uint32_t callback_vtable) = 0;
    virtual void SetUserColumnSizing(std::uint32_t list, bool value) = 0;
    virtual void SetUserColumnDragging(std::uint32_t list, bool value) = 0;
    virtual void SetUserSortControl(std::uint32_t list, bool value) = 0;
    virtual void SetListHeaderByte(std::uint32_t list,
                                   std::uint32_t offset,
                                   std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ColumnCount(std::uint32_t list) = 0;
    virtual void RemoveColumn(std::uint32_t list, std::uint32_t index) = 0;
    virtual void ResetList(std::uint32_t list) = 0;
    [[nodiscard]] virtual std::string TriggerTitle(
        std::uint32_t trigger_id) = 0;
    [[nodiscard]] virtual std::string ConvertToUtf8(
        std::string_view title) = 0;
    virtual void AddColumn(std::uint32_t list, std::string_view title,
                           std::uint32_t id, float width) = 0;
    virtual void SetEnabled(std::uint32_t window, bool value) = 0;
    virtual void SetupMouseHandlers(MissionWindowState& state,
                                    std::uint32_t button) = 0;
    virtual void DestroyMember(MissionWindowState& state) = 0;
    virtual void DestroyDialogBase(MissionWindowState& state) = 0;
};

class MissionWindowLifecycle final {
public:
    static constexpr std::uint32_t kInitialMemberVtable = 0x843628u;
    static constexpr std::uint32_t kObjectVtable = 0x8439ccu;
    static constexpr std::uint32_t kMemberVtable = 0x8439a4u;
    static constexpr std::uint32_t kCallbackVtable = 0x84399cu;

    [[nodiscard]] static MissionWindowState& Construct(
        MissionWindowState& state, std::uint32_t name,
        IMissionWindowLifecycleRuntime& runtime);
};

}  // namespace pal4
