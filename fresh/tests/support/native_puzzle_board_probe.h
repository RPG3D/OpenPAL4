#pragma once

#include "pal4/native_puzzle_board.h"
#include "pal4/native_cegui_ui_session.h"
#include "pal4/native_cegui_widget_images.h"
#include <iostream>
#include <stdexcept>

namespace pal4::test {
inline void VerifyNativePuzzleBoard(NativeCeguiUiSession& ui,
    const NativeGameDatabase& database,const std::filesystem::path& root) {
    auto check=[](bool value,const char* why){if(!value)throw std::runtime_error(why);};
    check(ui.LoadImageset(root,"gamedata\\ui\\imagesets\\youshang_0.imageset"),
        "Puzzle requires original youshang imageset");
    const auto module=GetModuleHandleA("CEGUIBase.dll");
    NativeCeguiWidgetImages images(module);
    NativePuzzleBoard board(module,database);
    board.Initialize(0);
    const auto first=board.Elements();
    check(first.size()==7u && board.OwnedStrings()==7u && board.Counter()==60 &&
        !board.UsesMoveBudget() && !board.TitleUtf8().empty() && !board.Complete(),
        "Original first puzzle has seven elements/60-second budget and is unsolved");
    check(first.front().property==1u && first.front().origin==std::array<int,2>{0,0} &&
        first.front().extent==std::array<int,2>{1,1} &&
        first.front().images==std::array<std::uint32_t,3>{
            images.Image("youshang0","fangxiao"),images.Image("youshang0","fangxiao2"),images.Image("youshang0","fangxiao3")},
        "Original images occupy normal/selected/foreground slots in that order");
    board.Confirm();
    check(board.Move(JigsawDirection::Right)==1u && board.Elements().front().origin==first.front().origin,
        "Actual occupied first-row neighbor rejects moving first piece");
    board.ResetSelection();
    const bool queued=ui.OverlayQueueEnabled();
    ui.SetOverlayQueueEnabled(true);
    try {
        for(int type=0;type<18;++type) {
            board.Initialize(type);
            const auto rows=board.Elements();
            check(!rows.empty() && board.OwnedStrings()==rows.size(),
                "Repeated original puzzle initialization retains only live element strings");
            for(const auto& row:rows)check(row.images[0] && row.images[1] && row.images[2],
                "Original puzzle image handles are all resolved through CEGUI");
            board.Render({160.f,80.f},.01f);
            check(ui.QueuedOverlayCount()!=0u,"Original Image/Font draw calls queue actual puzzle graphics");
            ui.ClearQueuedOverlays();
        }
        ui.SetOverlayQueueEnabled(queued);
    } catch(...) {ui.ClearQueuedOverlays();ui.SetOverlayQueueEnabled(queued);throw;}
    board.Initialize(0);
    check(board.Elements().size()==7u && board.OwnedStrings()==7u && !board.Complete(),
        "Return from all18 layouts restores unsolved first board and retires old strings");
    std::cout<<"Native puzzle board: all18 shared layouts, original GameDB/images/Puzzle font, DLL String ownership, draw queue, occupied move and reinitialization pass\n";
}
} // namespace pal4::test
