#include "pal4/win32_input_runtime.h"
#include <windows.h>
#include <cstdlib>
#include <iostream>

namespace {
void Check(bool value,const char* message) {
    if(!value){std::cerr<<message<<'\n';std::exit(1);}
}
}
int main() {
    pal4::InputManager input;
    (void)input.InitializeState();
    (void)input.InitializeKeyboardMappings();
    input.WriteDword(4u,1u);input.WriteDword(8u,2u);
    input.WriteDword(0x1870u,1u);input.WriteDword(0x186cu,1u);
    pal4::Win32InputRuntime runtime(false,true);
    runtime.ObserveComparisonMessage(WM_KEYDOWN,'W',0x00110001u);
    (void)input.UpdateInputDeviceState(runtime);
    Check(input.GetKeyState('w')==2u,"scripted scan enters existing game's press transition");
    (void)input.UpdateInputDeviceState(runtime);
    Check(input.GetKeyState('w')==3u,"held scan uses existing game's hold transition");
    runtime.ObserveComparisonMessage(WM_KEYUP,'W',0xc0110001u);
    (void)input.UpdateInputDeviceState(runtime);
    Check(input.GetKeyState('w')==1u,"scripted release uses existing release transition");
    (void)input.UpdateInputDeviceState(runtime);
    Check(input.GetKeyState('w')==0u,"release drains without a stuck key");
    runtime.ObserveComparisonMessage(WM_KEYDOWN,VK_UP,0x01480001u);
    Check(runtime.GetDeviceState(1u,256u,input,0x176u)==0 && input.ReadByte(0x176u+0xc8u)==0x80u,
        "extended Windows scan maps to DirectInput high-bit scan");
    runtime.ObserveComparisonMessage(WM_KEYDOWN,'A',0u);
    Check(runtime.GetDeviceState(1u,256u,input,0x176u)==0 && input.ReadByte(0x176u+0x1eu)==0x80u,
        "scan-less messages use the Windows mapping API");
    pal4::Win32InputRuntime quiet(true);
    quiet.ObserveComparisonMessage(WM_KEYDOWN,'W',0x00110001u);
    (void)quiet.GetDeviceState(1u,256u,input,0x176u);
    Check(input.ReadByte(0x176u+0x11u)==0u,"ordinary quiet tests do not accept comparison input");
    {
        pal4::InputManager sampled;
        (void)sampled.InitializeState();(void)sampled.InitializeKeyboardMappings();
        sampled.WriteDword(4u,1u);sampled.WriteDword(8u,2u);
        sampled.WriteDword(0x1870u,1u);sampled.WriteDword(0x186cu,1u);
        pal4::Win32InputRuntime budget(false,true);
        budget.ObserveComparisonMessage(pal4::ComparisonKeyboardBudget::kMessage,9u,3u); // W+D
        (void)budget.GetDeviceState(2u,20u,sampled,0x166u);
        Check(budget.ComparisonPendingSamples()==3u,"mouse reads do not consume keyboard samples");
        for(unsigned i=0;i<3u;++i){
            (void)sampled.UpdateInputDeviceState(budget);
            Check(sampled.GetKeyState('w')==(i?3u:2u) && sampled.GetKeyState('d')==(i?3u:2u),
                  "bounded chord enters original press/hold transitions for exactly three samples");
        }
        Check(budget.ComparisonPendingSamples()==0u,"keyboard sample budget completes");
        (void)sampled.UpdateInputDeviceState(budget);
        Check(sampled.GetKeyState('w')==1u && sampled.GetKeyState('d')==1u,
              "exhausted budget produces original release transition");
        (void)sampled.UpdateInputDeviceState(budget);
        Check(sampled.GetKeyState('w')==0u && sampled.GetKeyState('d')==0u,"bounded chord drains to neutral");
        budget.ObserveComparisonMessage(pal4::ComparisonKeyboardBudget::kMessage,1u,4u);
        budget.ObserveComparisonMessage(pal4::ComparisonKeyboardBudget::kMessage,16u,2u);
        Check(budget.ComparisonPendingSamples()==4u,"invalid mask leaves valid budget unchanged");
        budget.ObserveComparisonMessage(pal4::ComparisonKeyboardBudget::kMessage,1u,601u);
        Check(budget.ComparisonPendingSamples()==4u,"oversized budget rejected");
        budget.ObserveComparisonMessage(pal4::ComparisonKeyboardBudget::kMessage,0u,0u);
        Check(budget.ComparisonPendingSamples()==0u,"explicit cancellation releases budget");
        budget.ObserveComparisonMessage(pal4::ComparisonKeyboardBudget::kMessage,1u,4u);
        budget.ObserveComparisonMessage(WM_KEYDOWN,'S',0x001f0001u);
        (void)budget.GetDeviceState(1u,256u,sampled,0x176u);
        Check(budget.ComparisonPendingSamples()==0u && sampled.ReadByte(0x176u+0x11u)==0u &&
              sampled.ReadByte(0x176u+0x1fu)==0x80u,"manual key cancels bounded input without stale chord");
        budget.ObserveComparisonMessage(WM_KEYUP,'S',0xc01f0001u);
        budget.ObserveComparisonMessage(pal4::ComparisonKeyboardBudget::kMessage,1u,4u);
        budget.ObserveComparisonScene("M01","3",true);
        (void)sampled.UpdateInputDeviceState(budget);
        Check(sampled.GetKeyState('w')==2u,"scene-bound budget starts normally");
        budget.ObserveComparisonScene("M01","3",true);
        (void)sampled.UpdateInputDeviceState(budget);
        Check(sampled.GetKeyState('w')==3u && budget.ComparisonPendingSamples()==2u,
              "same scene preserves budget and hold transition");
        budget.ObserveComparisonScene("Q01","Q01",true);
        (void)sampled.UpdateInputDeviceState(budget);
        Check(sampled.GetKeyState('w')==1u && budget.ComparisonPendingSamples()==0u &&
              budget.ComparisonBoundaryReason()==2u,"map change releases before new scene sample");
        (void)sampled.UpdateInputDeviceState(budget);
        Check(sampled.GetKeyState('w')==0u,"boundary release drains normally");
        budget.ObserveComparisonMessage(pal4::ComparisonKeyboardBudget::kMessage,1u,4u);
        Check(budget.ComparisonBoundaryReason()==0u,"new budget clears old boundary");
        budget.ObserveComparisonScene("Q01","Q01",true);
        budget.ObserveComparisonMessage(pal4::ComparisonKeyboardBudget::kMessage,16u,2u);
        budget.ObserveComparisonScene("Q01","other",true);
        Check(budget.ComparisonBoundaryReason()==2u && budget.ComparisonPendingSamples()==0u,
              "invalid configuration preserves binding and area change cancels");
        budget.ObserveComparisonMessage(pal4::ComparisonKeyboardBudget::kMessage,1u,4u);
        budget.ObserveComparisonScene("Q01","Q01",false);
        Check(budget.ComparisonBoundaryReason()==1u && budget.ComparisonPendingSamples()==0u,
              "leaving Scene cancels budget");
        budget.ObserveComparisonMessage(WM_KEYDOWN,'S',0x001f0001u);
        budget.ObserveComparisonScene("","",false);
        (void)sampled.UpdateInputDeviceState(budget);
        Check(sampled.GetKeyState('s')==2u,"scene notifications preserve manual keys");
        quiet.ObserveComparisonMessage(pal4::ComparisonKeyboardBudget::kMessage,1u,4u);
        Check(quiet.ComparisonPendingSamples()==0u,"ordinary mode ignores bounded comparison commands");
    }
    std::cout<<"Comparison input: actual game transitions2/3/1/0, extended scans, fallback, quiet isolation pass\n";
}
