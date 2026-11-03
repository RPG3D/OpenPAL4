#include "pal4/smith_window_panel_input.h"
#include <bit>
namespace pal4 {
std::uint8_t SmithWindowPanelInput::Prescription(unsigned member,unsigned key,ISmithWindowPanelInputRuntime& r){
    const auto owner=r.ReadWord(member+4u);
    if(key==1u){r.RemoveActivity(member);r.UpdateDisplay(owner);return 1u;}
    if(key==200u || key==208u){const auto list=r.ReadWord(owner+0x44u);
        if(r.HasHover(list)){
            const auto row=r.HoverRow(list);
            if(key==200u){if(std::bit_cast<int>(row)>0)r.MoveHover(list,false);}
            else if(row<r.RowCount(list)-1u)r.MoveHover(list,true);
        }
        return 1u;
    }
    if(key==28u){r.ConfirmTrade(owner);return 1u;}
    return 0u;
}
std::uint8_t SmithWindowPanelInput::Equipment(unsigned member,unsigned key,ISmithWindowPanelInputRuntime& r){
    const auto owner=r.ReadWord(member+4u);
    if(key==1u){r.RemoveActivity(member);r.HideSelection(owner);return 1u;}
    if(key==200u){
        if(!r.ReadByte(member+8u)){const auto list=r.ReadWord(owner+0x40u);
            if(r.HasHover(list)){if(r.HoverRow(list))r.MoveHover(list,false);else r.SetHighlighted(member,true);}}
        return 1u;
    }
    if(key==208u){const auto list=r.ReadWord(owner+0x40u);
        if(r.ReadByte(member+8u)){if(r.RowCount(list)){r.SetHighlighted(member,false);r.SetHoverRow(list,0u);}}
        else if(r.HasHover(list) && r.HoverRow(list)<r.RowCount(list)-1u)r.MoveHover(list,true);
        return 1u;
    }
    if(key==203u || key==205u){if(r.ReadByte(member+8u)){
        r.ClearHighlight(r.ReadWord(owner+(key==203u?0x5cu:0x58u)));
        r.MouseAtCenter(r.ReadWord(owner+(key==203u?0x58u:0x5cu)));
        }return 1u;
    }
    if(key==28u){r.TradeSelected(owner);return 1u;}
    return 0u;
}
}
