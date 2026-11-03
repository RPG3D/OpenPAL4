#pragma once
#include <initializer_list>
#include <string_view>

namespace pal4 {
// IDA 0x425300: preserve match priority, case, and C-string truncation.
inline unsigned FloorEffectKind(std::string_view path) {
    path=path.substr(0,path.find('\0'));
    const auto any=[&](std::initializer_list<std::string_view> codes) {
        for(auto code:codes)if(path.find(code)!=std::string_view::npos)return true;
        return false;
    };
    if(any({"WF011","WF013","WF015","WF017","WF023"}))return 1;
    if(any({"WF041","WF037"}))return 2;
    if(any({"WF021","WF039"}))return 3;
    return 0;
}
} // namespace pal4
