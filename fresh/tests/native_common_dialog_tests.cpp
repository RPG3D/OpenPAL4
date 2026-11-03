#include "support/native_cegui_frame_session.h"
#include "support/native_desktop_observation.h"
#include "pal4/native_cegui_ui_session.h"
#include <rtfsyst.h>
#include <filesystem>
#include <iostream>
#include <stdexcept>
void VerifyNativeCommonDialogLayout(pal4::NativeCeguiUiSession&,const std::filesystem::path&);
void VerifyNativeYesNoDialog(pal4::NativeCeguiUiSession&,const std::filesystem::path&);
namespace {
void Check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Filesystem{bool ready{};~Filesystem(){if(ready)RtFSManagerClose();}};
}
int main(int argc,char** argv)try{
    std::cout<<std::unitbuf;Check(argc==2,"provide original game directory");
    const pal4::test::NativeDesktopObservation desktop;
    pal4::test::NativeFrameSession frame;Check(frame.Start(800,600),"CommonDialog hidden SDK frame");
    Check(!IsWindowVisible(frame.window),"CommonDialog window remains hidden");
    Filesystem files;files.ready=RtFSManagerOpen(8)!=FALSE;Check(files.ready,"CommonDialog filesystem");
    pal4::NativeCeguiUiSession ui;Check(ui.Start(argv[1],800,600),"CommonDialog original DLL UI");
    Check(ui.LoadImageset(argv[1],"gamedata\\ui\\fonts\\fontPuzzle_0.imageset") && ui.InitializeWidgetRendering(argv[1]),"CommonDialog widget rendering");
    Check(ui.LoadImageset(argv[1],"gamedata\\ui\\imagesets\\OiramLook.imageset") && ui.LoadOriginalMainLayout() && ui.HideMainLayout(),"CommonDialog desktop/font");
    VerifyNativeCommonDialogLayout(ui,argv[1]);VerifyNativeYesNoDialog(ui,argv[1]);desktop.Check("CommonDialog native regression");return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
