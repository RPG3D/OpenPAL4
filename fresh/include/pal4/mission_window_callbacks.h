#pragma once

#include "pal4/mission_window_lifecycle.h"

#include <cstdint>

namespace pal4 {

class IMissionWindowDetailsCallbackRuntime {
public:
    virtual ~IMissionWindowDetailsCallbackRuntime() = default;
    virtual void UpdateDetailsPanel(MissionWindowState& state) = 0;
};

class IMissionWindowListCallbackRuntime : public IMissionWindowDetailsCallbackRuntime {
public:
    virtual void InsertActiveMember(MissionWindowState& state,
                                    std::uint32_t offset) = 0;
    virtual void SetVisible(std::uint32_t window, bool visible) = 0;
    virtual void SetAlwaysOnTop(std::uint32_t window, bool value) = 0;
    virtual void HandleMouseEvent(std::uint32_t window) = 0;
};

class IMissionWindowCallbacksRuntime : public IMissionWindowListCallbackRuntime {
public:
    virtual void PopulateMissionList(MissionWindowState& state) = 0;
    virtual void PopulateQuestList(MissionWindowState& state) = 0;
};

class MissionWindowCallbacks final {
public:
    [[nodiscard]] static std::uint8_t MissionClicked(
        MissionWindowState& state, std::uint32_t event,
        IMissionWindowCallbacksRuntime& runtime);
    [[nodiscard]] static std::uint8_t OpenMission(
        MissionWindowState& state, IMissionWindowCallbacksRuntime& runtime);
    [[nodiscard]] static std::uint8_t QuestClicked(
        MissionWindowState& state, std::uint32_t event,
        IMissionWindowCallbacksRuntime& runtime);
    [[nodiscard]] static std::uint8_t OpenQuest(
        MissionWindowState& state, IMissionWindowCallbacksRuntime& runtime);
    [[nodiscard]] static std::uint8_t HoverChanged(
        MissionWindowState& state, std::uint32_t event,
        IMissionWindowDetailsCallbackRuntime& runtime);
    [[nodiscard]] static std::uint8_t ListMouseClick(
        MissionWindowState& state, std::uint32_t event,
        IMissionWindowListCallbackRuntime& runtime);
};

}  // namespace pal4
