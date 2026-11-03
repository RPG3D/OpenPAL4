#pragma once
#include <functional>
namespace pal4 {
class NativeCombatEndingWindow;class NativeEventHandlerArray;
namespace test {
void VerifyNativeCombatEndingInput(NativeCombatEndingWindow&,NativeEventHandlerArray&,const std::function<void()>& restart);
}
}
