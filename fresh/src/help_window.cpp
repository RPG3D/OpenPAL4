#include "pal4/help_window.h"
#include "pal4/preview_text_codec.h"

#include <new>

namespace pal4 {

namespace {

void InstallCallback(const std::uint32_t window,
                     const HelpWindowEvent event,
                     const std::uint32_t function,
                     const std::uint32_t object,
                     IHelpWindowRuntime& runtime) {
    const auto callback = runtime.Allocate(0x18u);
    if (callback != 0u) {
        runtime.WriteWord(callback + 8u, function);
        runtime.WriteWord(callback + 0xcu, 0u);
        runtime.WriteWord(callback + 0x10u, object);
        runtime.WriteWord(callback, HelpWindow::kCallbackVtable);
    }
    const auto connection = runtime.Subscribe(window, event, callback);
    if (connection != 0u) runtime.ReleaseConnection(connection);
}

void ConfigureList(const std::uint32_t list,
                   IHelpWindowRuntime& runtime) {
    while (runtime.ColumnCount(list) != 0u)
        runtime.RemoveColumn(list, 0u);
    runtime.SetSortDirection(list, 0u);
    runtime.SetUserColumnSizing(list, false);
    runtime.SetUserColumnDragging(list, false);
    runtime.SetUserSortControl(list, false);
    runtime.AddColumn(list, U"", 0u, 1.0f);
    runtime.WriteByte(list + 0x4d7u, 0u);
}

}  // namespace

std::uint32_t HelpWindow::Construct(
    const std::uint32_t object, const std::uint32_t layout_name,
    IHelpWindowRuntime& runtime) {
    runtime.LoadLayout(object, layout_name);
    try {
        runtime.WriteWord(object + 0x28u, 0x8435a4u);
        runtime.WriteWord(object + 0x40u, 0u);
        runtime.WriteWord(object + 0x44u, 0x843fb0u);
        runtime.WriteWord(object + 0x4cu, 0x843f98u);
        runtime.WriteWord(object, kVtable);
        runtime.WriteWord(object + 0x28u, kSecondaryVtable);
        InitializeControls(object, runtime);
        runtime.SetAlwaysOnTop(runtime.ReadWord(object + 0x24u), true);
        runtime.ClassifyTimeValue(runtime.TickCount());
        runtime.WriteWord(object + 0x48u, object);
        runtime.WriteWord(object + 0x50u, object);
    } catch (...) {
        (void)runtime.DestroyBase(object);
        throw;
    }
    return object;
}

void HelpWindow::SetVisible(
    const std::uint32_t object, const bool visible,
    const std::uint32_t parent, IHelpWindowRuntime& runtime) {
    runtime.SetParent(object, visible, parent);
    if (!visible) {
        runtime.RemoveCallback(runtime.FrameManager() + 0x134u,
                               object == 0u ? 0u : object + 0x28u);
    }
    runtime.SetAlwaysOnTop(runtime.ReadWord(object + 0x34u), true);
    runtime.SetVisible(runtime.ReadWord(object + 0x3cu), false);
    runtime.ResetList(runtime.ReadWord(object + 0x38u));
    runtime.SetText(runtime.ReadWord(object + 0x2cu), U"");
    runtime.SetModal(runtime.ReadWord(object + 0x24u), true);
    if (visible)
        runtime.InsertCallback(runtime.FrameManager() + 0x134u,
                               object + 0x28u);
}

std::uint8_t HelpWindow::HandleShow(
    const std::uint32_t event, IHelpWindowRuntime& runtime) {
    const auto show = runtime.ReadWord(event) != 0u;
    runtime.HandleFrameEvent(kNamePointer, {show ? 1u : 0u, 0u});
    if (show) {
        const auto frame = runtime.FindFrame(runtime.FrameManager(),
                                             kNamePointer);
        PopulateTopicList(frame, runtime.ReadWord(event + 4u), runtime);
    }
    return 1u;
}

std::uint8_t HelpWindow::OnShow(
    const std::uint32_t object, IHelpWindowRuntime& runtime) {
    (void)RefreshContentList(object, runtime);
    runtime.SetAlwaysOnTop(runtime.ReadWord(object + 0x34u), false);
    runtime.SetVisible(runtime.ReadWord(object + 0x3cu), true);
    runtime.HandleMouseEvent(runtime.ReadWord(object + 0x3cu));
    runtime.InsertCallback(runtime.FrameManager() + 0x134u, object + 0x44u);
    return 1u;
}

std::uint8_t HelpWindow::RefreshContentList(
    const std::uint32_t object, IHelpWindowRuntime& runtime) {
    (void)PopulateContentList(object, runtime);
    return 1u;
}

std::uint8_t HelpWindow::OnListChanged(
    const std::uint32_t object, IHelpWindowRuntime& runtime) {
    (void)OnItemClick(object, runtime);
    return 1u;
}

std::uint8_t HelpWindow::OnItemClick(
    const std::uint32_t object, IHelpWindowRuntime& runtime) {
    const auto selected = SelectedContentData(object, runtime);
    if (selected != 0u) SetDescription(object, selected, runtime);
    const auto content_list = runtime.ReadWord(object + 0x38u);
    const auto hovered = runtime.HoverItem(content_list);
    if (hovered == 0u) return 0u;

    const auto seen_base = runtime.GlobalData() + 0x54u;
    const auto id = runtime.ReadWord(selected);
    if (id < 0x100u) {
        const auto address = seen_base + (id >> 5u) * 4u;
        const auto bit = 1u << (id & 31u);
        const auto seen = runtime.ReadWord(address);
        if ((seen & bit) == 0u) {
            runtime.WriteWord(address, seen | bit);
            runtime.SaveSettings(runtime.GlobalData());
            runtime.SetListItemImage(hovered, 0u);
            runtime.RequestRedraw(runtime.ReadWord(object + 0x38u));
            if (!HasUnreadHelp(runtime.ReadWord(selected + 4u), runtime)) {
                const auto topic_list = runtime.ReadWord(object + 0x34u);
                const auto row = static_cast<std::int32_t>(
                    runtime.ReadWord(topic_list + 0x4c0u));
                const auto scrollbar = runtime.ReadWord(topic_list + 0x4b4u);
                const auto position = runtime.ReadFloat(scrollbar + 0x4c0u);
                PopulateTopicList(object, 0u, runtime);
                runtime.SetHoverRow(runtime.ReadWord(object + 0x34u), row);
                const auto current_list = runtime.ReadWord(object + 0x34u);
                runtime.SetScrollPosition(
                    runtime.ReadWord(current_list + 0x4b4u), position);
            }
        }
    }
    return 1u;
}

std::uint8_t HelpWindow::OnClose(
    const std::uint32_t object, IHelpWindowRuntime& runtime) {
    const auto content = runtime.ReadWord(object + 0x3cu);
    const auto callback = runtime.IsWindowVisible(content, false)
        ? object + 0x44u : object + 0x28u;
    runtime.InvokeCallback(callback);
    return 1u;
}

void HelpWindow::InitializeControls(
    const std::uint32_t object, IHelpWindowRuntime& runtime) {
    runtime.WriteWord(object + 0x2cu,
                      runtime.GetWindow("HelpWindow/EditInfo"));
    const auto close_button = runtime.GetWindow("HelpWindow/BtnClose");
    runtime.WriteWord(object + 0x30u, close_button);
    InstallCallback(close_button, HelpWindowEvent::Clicked,
                    0x4b6890u, object, runtime);

    runtime.WriteWord(object + 0x3cu,
                      runtime.GetWindow("HelpWindow/ImgDisusedMask"));
    const auto topic_list = runtime.GetWindow("HelpWindow/List1");
    runtime.WriteWord(object + 0x34u, topic_list);
    InstallCallback(topic_list, HelpWindowEvent::HoverRowChanged,
                    0x4b7ff0u, object, runtime);
    InstallCallback(topic_list, HelpWindowEvent::MouseClick,
                    0x4a7820u, object, runtime);

    const auto content_list = runtime.GetWindow("HelpWindow/List2");
    runtime.WriteWord(object + 0x38u, content_list);
    InstallCallback(content_list, HelpWindowEvent::HoverRowChanged,
                    0x4a7880u, object, runtime);

    ConfigureList(topic_list, runtime);
    ConfigureList(content_list, runtime);
    InstallCallback(runtime.ReadWord(object + 0x24u),
                    HelpWindowEvent::KeyDown, 0x4a79a0u, object, runtime);
}

void HelpWindow::PopulateTopicList(
    const std::uint32_t object, const std::uint32_t selected_topic,
    IHelpWindowRuntime& runtime) {
    runtime.WriteWord(object + 0x40u, selected_topic);
    const auto list = runtime.ReadWord(object + 0x34u);
    runtime.ResetList(list);
    const auto entries = runtime.TopicEntries();
    std::uint32_t selected_row = 0u;
    for (const auto& entry : entries) {
        if (HelpDataCount(entry.id, runtime) == 0u) continue;
        runtime.AddRow(list, 0u);
        const auto item_storage = runtime.Allocate(0x214u);
        if (item_storage == 0u) throw std::bad_alloc();
        std::uint32_t item;
        try {
            const auto utf8 = runtime.ConvertToUtf8(entry.title);
            const auto text = PreviewTextCodec::DecodeUtf8(utf8);
            item = runtime.ConstructListItem(item_storage, list, text,
                                             HasUnreadHelp(entry.id, runtime));
        } catch (...) {
            runtime.FreeItem(item_storage);
            throw;
        }
        runtime.WriteWord(item + 0x138u, entry.id);
        const auto row = runtime.RowCount(list) - 1u;
        runtime.SetItem(list, item, 0u, row);
        if (entry.id == selected_topic) selected_row = row;
    }
    if (runtime.RowCount(list) > selected_row) {
        runtime.SetHoverRow(list, static_cast<std::int32_t>(selected_row));
        runtime.ScrollRowToVisible(list, selected_row);
    }
}

std::uint32_t HelpWindow::PopulateContentList(
    const std::uint32_t object, IHelpWindowRuntime& runtime) {
    const auto list = runtime.ReadWord(object + 0x38u);
    runtime.ResetList(list);
    const auto global = runtime.GlobalData();
    const auto selected_topic = SelectedTopicId(object, runtime);
    const auto entries = runtime.ContentEntries();
    std::vector<const HelpContentEntry*> matches;
    for (const auto& entry : entries) {
        if (entry.topic_id == selected_topic) matches.push_back(&entry);
    }
    for (const auto* entry : matches) {
        const auto progress = static_cast<std::int32_t>(
            runtime.ReadWord(0x90dec0u));
        if (entry->unlock_progress > progress || entry->id >= 0x100u)
            continue;
        runtime.AddRow(list, 0u);
        const auto storage = runtime.Allocate(0x214u);
        if (storage == 0u) throw std::bad_alloc();
        std::uint32_t item;
        try {
            const auto utf8 = runtime.ConvertToUtf8(entry->title);
            const auto text = PreviewTextCodec::DecodeUtf8(utf8);
            const auto word = runtime.ReadWord(
                global + 0x54u + (entry->id >> 5u) * 4u);
            const auto unread =
                (word & (1u << (entry->id & 31u))) == 0u;
            item = runtime.ConstructListItem(storage, list, text, unread);
        } catch (...) {
            runtime.FreeItem(storage);
            throw;
        }
        runtime.WriteWord(item + 0x138u, entry->address);
        runtime.SetItem(list, item, 0u, runtime.RowCount(list) - 1u);
    }
    if (runtime.RowCount(list) != 0u) runtime.SetHoverRow(list, 0);
    return 0u;
}

void HelpWindow::SetDescription(
    const std::uint32_t object, const std::uint32_t content,
    IHelpWindowRuntime& runtime) {
    if (content == 0u) return;
    const auto text_pointer = runtime.ReadWord(content + 0x1cu);
    const auto source = text_pointer == 0u
        ? std::string{} : runtime.ReadText(text_pointer);
    const auto utf8 = runtime.ConvertToUtf8(source);
    runtime.SetText(runtime.ReadWord(object + 0x2cu),
                    PreviewTextCodec::DecodeUtf8(utf8));
}

std::uint32_t HelpWindow::SelectedTopicId(
    const std::uint32_t object, IHelpWindowRuntime& runtime) {
    const auto item = runtime.HoverItem(runtime.ReadWord(object + 0x34u));
    return item == 0u ? 0xffffffffu : runtime.ReadWord(item + 0x138u);
}

std::uint32_t HelpWindow::SelectedContentData(
    const std::uint32_t object, IHelpWindowRuntime& runtime) {
    const auto item = runtime.HoverItem(runtime.ReadWord(object + 0x38u));
    return item == 0u ? 0u : runtime.ReadWord(item + 0x138u);
}

std::uint32_t HelpWindow::HelpDataCount(
    const std::uint32_t topic, IHelpWindowRuntime& runtime) {
    const auto progress = static_cast<std::int32_t>(
        runtime.ReadWord(0x90dec0u));
    std::uint32_t count = 0u;
    for (const auto& entry : runtime.ContentEntries()) {
        if (entry.topic_id == topic && entry.unlock_progress <= progress)
            ++count;
    }
    return count;
}

bool HelpWindow::HasUnreadHelp(
    const std::uint32_t topic, IHelpWindowRuntime& runtime) {
    const auto global = runtime.GlobalData();
    const auto progress = static_cast<std::int32_t>(
        runtime.ReadWord(0x90dec0u));
    for (const auto& entry : runtime.ContentEntries()) {
        if (entry.topic_id != topic || entry.unlock_progress > progress ||
            entry.id >= 0x100u)
            continue;
        const auto seen = runtime.ReadWord(
            global + 0x54u + (entry.id >> 5u) * 4u);
        if ((seen & (1u << (entry.id & 31u))) == 0u) return true;
    }
    return false;
}

std::uint8_t HelpWindow::OnKeyDown(
    const std::uint32_t callback, const std::uint32_t event,
    IHelpWindowRuntime& runtime) {
    const auto key = runtime.ReadWord(event + 0x10u);
    if (key != 1u && key != 200u && key != 208u && key != 28u)
        return 0u;

    const auto object = runtime.ReadWord(callback + 4u);
    if (key == 1u) {
        runtime.RemoveCallback(runtime.FrameManager() + 0x134u, callback);
        runtime.SetAlwaysOnTop(runtime.ReadWord(object + 0x34u), true);
        runtime.SetVisible(runtime.ReadWord(object + 0x3cu), false);
        runtime.ResetList(runtime.ReadWord(object + 0x38u));
        runtime.SetText(runtime.ReadWord(object + 0x2cu), U"");
        runtime.InsertCallback(runtime.FrameManager() + 0x134u,
                               object == 0u ? 0u : object + 0x28u);
    } else if (key == 200u || key == 208u) {
        runtime.MoveHoverItem(runtime.ReadWord(object + 0x38u),
                              key == 200u);
    } else {
        (void)OnItemClick(object, runtime);
    }
    return 1u;
}

std::int32_t HelpWindow::HelpDataSize(
    const std::uint32_t collection, IHelpWindowRuntime& runtime) {
    const auto start = runtime.ReadWord(collection + 4u);
    if (start == 0u) return 0;
    const auto end = runtime.ReadWord(collection + 8u);
    return static_cast<std::int32_t>(end - start) / 44;
}

std::uint8_t HelpWindow::ProcessEvents(
    const std::uint32_t handler, IHelpWindowRuntime& runtime) {
    return runtime.ProcessUiEvents(runtime.FrameManager() + 0x104u,
                                   handler + 8u, handler);
}

}  // namespace pal4
