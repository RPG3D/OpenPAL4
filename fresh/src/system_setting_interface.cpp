#include "pal4/system_setting_interface.h"

#include <string>

namespace pal4 {
namespace {

std::uint32_t Find(const std::uint32_t frame,
                   const std::uint32_t buffer, std::string_view suffix,
                   ISystemSettingInterfaceRuntime& runtime) {
    return runtime.FindWindow(runtime.ResetName(buffer, frame, suffix));
}

std::uint32_t Bind(const std::uint32_t frame,
                   const std::uint32_t window, const std::uint32_t event_address,
                   const std::uint32_t function,
                   ISystemSettingInterfaceRuntime& runtime) {
    const auto callback = runtime.Allocate(0x18u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, function);
        runtime.WriteWord(callback + 12u, 0u);
        runtime.WriteWord(callback + 16u, frame);
        runtime.WriteWord(callback, SystemSettingInterface::kCallbackVtable);
    }
    const auto event = runtime.ReadWord(event_address);
    const auto connection = runtime.Subscribe(window, event, callback);
    if (connection != 0u) {
        runtime.ReleaseConnection(connection);
    }
    return callback;
}

void BindNumbered(const std::uint32_t frame, const std::uint32_t buffer,
                  std::string_view prefix, const std::uint32_t count,
                  const std::uint32_t window_offset,
                  const std::uint32_t record_offset,
                  const std::uint32_t function,
                  ISystemSettingInterfaceRuntime& runtime) {
    for (std::uint32_t index = 0; index < count; ++index) {
        const auto window = Find(frame, buffer,
                                 std::string(prefix) + std::to_string(index + 1u),
                                 runtime);
        runtime.WriteWord(frame + window_offset + index * 4u, window);
        (void)Bind(frame, window, SystemSettingInterface::kClickedEvent,
                   function, runtime);
        runtime.WriteWord(window + 280u, index);
        runtime.WriteWord(frame + record_offset + index * 20u, window);
    }
}

void LinkRecords(const std::uint32_t frame,
                 ISystemSettingNavigationLayoutRuntime& runtime) {
    constexpr std::uint32_t groups[][2] = {
        {240u, 3u}, {300u, 2u}, {340u, 3u}, {400u, 2u}};
    for (const auto& group : groups) {
        for (std::uint32_t index = 0; index < group[1]; ++index) {
            runtime.AppendRecordPointer(frame + 72u,
                                        frame + group[0] + index * 20u);
        }
    }
    constexpr std::uint32_t links[][2] = {
        {256, 340}, {248, 260}, {284, 260}, {372, 260},
        {264, 240}, {288, 300}, {324, 300}, {364, 340},
        {296, 380}, {368, 380}, {404, 380}, {432, 380},
        {308, 320}, {336, 420}, {412, 300}, {268, 280},
        {276, 360}, {304, 280}, {316, 400}, {352, 240},
        {348, 360}, {392, 280}, {384, 360}, {388, 400},
        {408, 420}, {424, 400}};
    for (const auto& link : links) {
        runtime.WriteWord(frame + link[0], frame + link[1]);
    }
}

}  // namespace

void SystemSettingInterface::LinkNavigationRecords(std::uint32_t frame,
    ISystemSettingNavigationLayoutRuntime& runtime) {
    LinkRecords(frame, runtime);
}

void SystemSettingInterface::ConfigureSaveList(
    const std::uint32_t list, ISystemSettingListControlsRuntime& runtime) {
    runtime.ResetList(list);
    while (runtime.ColumnCount(list) != 0u) runtime.RemoveColumn(list, 0u);
    runtime.SetSortDirection(list, 0u);
    runtime.SetUserColumnSizing(list, false);
    runtime.SetUserColumnDragging(list, false);
    runtime.SetUserSortControl(list, false);
}

std::uint32_t SystemSettingInterface::Initialize(
    const std::uint32_t frame, ISystemSettingInterfaceRuntime& runtime) {
    const auto buffer = runtime.BeginName(frame);
    try {
        for (std::uint32_t index = 0; index < 3u; ++index) {
            runtime.WriteWord(frame + 112u + index * 4u,
                Find(frame, buffer, "SettingWindow" + std::to_string(index), runtime));
        }
        for (std::uint32_t index = 0; index < 3u; ++index) {
            runtime.WriteWord(frame + 124u + index * 4u,
                Find(frame, buffer, "SaveWindow" + std::to_string(index), runtime));
        }
        for (std::uint32_t index = 0; index < 10u; ++index) {
            const auto window = Find(frame, buffer,
                "BtnData" + std::to_string(index), runtime);
            runtime.WriteWord(frame + 136u + index * 4u, window);
            runtime.WriteWord(window + 280u, index);
            (void)Bind(frame, window, kClickedEvent, 0x481980u, runtime);
            runtime.SetText(window, runtime.ToUtf8(runtime.TriggerText(205u + index)));
        }
        constexpr struct MainButton {
            std::string_view suffix;
            std::uint32_t callback;
        } buttons[] = {{"BtnSaveData", 0x481010u}, {"BtnLoadData", 0x480de0u},
                       {"BtnSystemSetting", 0x4812e0u}, {"BtnExit", 0x481510u}};
        for (std::uint32_t index = 0; index < 4u; ++index) {
            const auto window = Find(frame, buffer, buttons[index].suffix, runtime);
            runtime.WriteWord(frame + 184u + index * 4u, window);
            (void)Bind(frame, window, kClickedEvent, buttons[index].callback, runtime);
        }
        const auto mask = Find(frame, buffer, "ImgDataMask", runtime);
        runtime.WriteWord(frame + 176u, mask);
        runtime.SetAlwaysOnTop(mask, true);
        runtime.SetEnabled(mask, false);
        const auto label = Find(frame, buffer, "StaticDataMask", runtime);
        runtime.WriteWord(frame + 108u, label);
        runtime.SetHorizontalFormatting(label, 2u);
        runtime.CopyText(label, runtime.ReadWord(frame + 136u));

        const auto list = Find(frame, buffer, "List", runtime);
        runtime.WriteWord(frame + 104u, list);
        ConfigureSaveList(list, runtime);
        (void)Bind(frame, list, kHoverEvent, 0x481e10u, runtime);
        (void)Bind(frame, list, kMouseClickEvent, 0x4819a0u, runtime);

        BindNumbered(frame, buffer, "BtnSeq", 3u, 200u, 240u, 0x481b80u, runtime);
        BindNumbered(frame, buffer, "BtnClip", 2u, 212u, 300u, 0x481bd0u, runtime);
        BindNumbered(frame, buffer, "BtnAuto", 3u, 220u, 340u, 0x481c20u, runtime);
        runtime.WriteWord(runtime.ReadWord(frame + 220u) + 280u, 1u);
        runtime.WriteWord(runtime.ReadWord(frame + 224u) + 280u, 0u);
        runtime.WriteWord(runtime.ReadWord(frame + 228u) + 280u, 2u);
        BindNumbered(frame, buffer, "BtnIntro", 2u, 232u, 400u, 0x481c70u, runtime);

        for (std::uint32_t index = 0; index < 2u; ++index) {
            const auto slider = Find(frame, buffer,
                "ScroolAudio" + std::to_string(index + 1u), runtime);
            runtime.WriteWord(frame + 496u + index * 4u, slider);
            const auto child = runtime.ReadWord(slider + 1212u);
            (void)Bind(frame, child, kMouseButtonUpEvent, 0x481df0u, runtime);
        }
        runtime.WriteWord(frame + 180u,
                          Find(frame, buffer, "cucunyouxi", runtime));
        for (std::uint32_t index = 0; index < 14u; ++index) {
            runtime.WriteWord(frame + 440u + index * 4u,
                Find(frame, buffer,
                     "StaticInfo" + std::to_string(index + 1u), runtime));
        }
        for (std::uint32_t index = 0; index < 2u; ++index) {
            const auto slider = runtime.ReadWord(frame + 496u + index * 4u);
            runtime.SetSliderMaximum(slider, 1.0f);
        }
        (void)Bind(frame, runtime.ReadWord(frame + 496u), kValueChangedEvent,
                   0x481cc0u, runtime);
        (void)Bind(frame, runtime.ReadWord(frame + 500u), kValueChangedEvent,
                   0x481d20u, runtime);
        for (std::uint32_t index = 0; index < 4u; ++index) {
            runtime.SetupMouseHandlers(frame + 40u,
                                       runtime.ReadWord(frame + 184u + index * 4u));
        }
        runtime.WriteWord(frame + 100u, frame);
        runtime.WriteWord(frame + 68u, frame);
        runtime.WriteWord(frame + 88u, runtime.ReadWord(frame + 496u));
        LinkRecords(frame, runtime);
    } catch (...) {
        (void)runtime.EndName(buffer);
        throw;
    }
    return runtime.EndName(buffer);
}

}  // namespace pal4
