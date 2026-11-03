#pragma once

#include "pal4/mission_window_lifecycle.h"

#include <cstdint>
#include <string_view>

namespace pal4 {

class IMissionWindowPanelRuntime {
public:
    virtual ~IMissionWindowPanelRuntime() = default;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetImage(std::uint32_t window, std::string_view image_set,
                          std::string_view image_name) = 0;
};

class IMissionWindowVisibilityRuntime : public IMissionWindowPanelRuntime {
public:
    virtual ~IMissionWindowVisibilityRuntime() = default;
    virtual void SetParent(MissionWindowState& state, std::uint8_t visible,
                           std::uint32_t parent) = 0;
    virtual void ProcessUiEvent(std::uint32_t event,
                                std::uint32_t first,
                                std::uint32_t second) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window, bool value) = 0;
    virtual void SetListHeaderByte(std::uint32_t list,
                                   std::uint32_t offset,
                                   std::uint8_t value) = 0;
    virtual void SetSelected(std::uint32_t button, bool value) = 0;
    virtual void InsertActiveMember(MissionWindowState& state,
                                    std::uint32_t member_offset) = 0;
    virtual void RemoveActiveMember(MissionWindowState& state,
                                    std::uint32_t member_offset) = 0;
    virtual void PopulateMissionList(MissionWindowState& state) = 0;
    virtual void UpdateSystemDialogVisibility(std::uint32_t key) = 0;
};

class MissionWindowVisibility final {
public:
    static void SetCharacterPanel(MissionWindowState& state, bool visible,
                                  IMissionWindowPanelRuntime& runtime);
    static void SetMissionPanel(MissionWindowState& state, bool visible,
                                IMissionWindowPanelRuntime& runtime);
    static void Set(MissionWindowState& state, std::uint8_t visible,
                    std::uint32_t parent,
                    IMissionWindowVisibilityRuntime& runtime);
};

}  // namespace pal4
