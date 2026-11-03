#pragma once
#include "pal4/portable/dialog_assets.h"
#include "pal4/portable/dialog_text.h"
#include <array>
#include <iosfwd>
namespace pal4::portable {
using TutorialRect=std::array<float,4>;
struct TutorialPatch {UiImage image;TutorialRect rect{};std::uint32_t color{0xffffffff};};
struct TutorialInline {std::uint32_t character{};UiImage image;};
struct TutorialPresentation {
    std::vector<TutorialPatch> patches;
    bool hint_sized{};
    TutorialRect text_rect{},button_rect{};
    UiImage button;
    std::string font,text;
    float font_size{},line_space{};
    std::uint32_t color{};
    std::vector<TutorialInline> images;
    std::vector<DialogTextRun> text_runs;
};
// Original layout and imagesets are read directly from user ui.cpk by UiReader.
void ResizeHintPresentation(TutorialPresentation&,float measured_width,float measured_height);
TutorialPresentation PrepareHintPresentation(DialogAssets&,std::string_view source,float x=.5f,float y=.15f);
TutorialPresentation PrepareTutorialPresentation(DialogAssets&,std::string_view source);
std::vector<std::string> TutorialResourceMembers(const TutorialPresentation&);
void WriteTutorialPresentation(std::ostream&,const TutorialPresentation&);
}
