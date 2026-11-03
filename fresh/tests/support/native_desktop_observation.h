#pragma once
#include <windows.h>
#include <iostream>
#include <stdexcept>
#include <string>
namespace pal4::test {
// An unchanged desktop is measurable only while there is no concurrent input.
// Never count an interrupted observation as proof of cursor preservation.
class NativeDesktopObservation final {
    HWND foreground_{};
    POINT cursor_{};
    LASTINPUTINFO input_{sizeof(LASTINPUTINFO)};
    static void RejectTestForeground(HWND foreground){
        DWORD process{};
        if(foreground)GetWindowThreadProcessId(foreground,&process);
        if(process==GetCurrentProcessId())throw std::runtime_error("hidden native test acquired desktop foreground");
    }
public:
    NativeDesktopObservation():foreground_(GetForegroundWindow()){
        if(!GetCursorPos(&cursor_) || !GetLastInputInfo(&input_))
            throw std::runtime_error("native test desktop snapshot unavailable");
        RejectTestForeground(foreground_);
    }
    void Check(const char* label) const{
        POINT cursor{};LASTINPUTINFO input{sizeof(LASTINPUTINFO)};
        if(!GetCursorPos(&cursor) || !GetLastInputInfo(&input))
            throw std::runtime_error("native test desktop observation unavailable");
        const auto foreground=GetForegroundWindow();RejectTestForeground(foreground);
        if(input.dwTime!=input_.dwTime){
            std::cout<<"Desktop observation inconclusive (concurrent input): "<<label
                <<" input="<<input_.dwTime<<"->"<<input.dwTime<<'\n';
            return;
        }
        if(foreground!=foreground_ || cursor.x!=cursor_.x || cursor.y!=cursor_.y){
            DWORD before_process{},after_process{};
            if(foreground_)GetWindowThreadProcessId(foreground_,&before_process);
            if(foreground)GetWindowThreadProcessId(foreground,&after_process);
            std::cerr<<"Desktop observation mismatch: "<<label
                <<" cursor="<<cursor_.x<<','<<cursor_.y<<"->"<<cursor.x<<','<<cursor.y
                <<" foreground="<<foreground_<<"->"<<foreground
                <<" process="<<before_process<<"->"<<after_process
                <<" test_process="<<GetCurrentProcessId()<<" input="<<input.dwTime<<'\n';
            throw std::runtime_error(std::string("desktop changed without concurrent input: ")+label);
        }
        std::cout<<"Desktop observation preserved: "<<label<<'\n';
    }
};
}
