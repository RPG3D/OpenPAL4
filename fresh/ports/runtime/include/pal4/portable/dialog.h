#pragma once
#include "pal4/portable/script_stack.h"
#include "pal4/portable/dialog_assets.h"
#include "pal4/portable/dialog_text.h"
#include "pal4/portable/dialog_typewriter.h"
#include <iosfwd>

namespace pal4::portable {
struct DialogView {
    std::uint64_t generation{},id{},revision{};
    bool visible{},configured{},portrait_left{};
    std::string text,display_text;
    std::vector<DialogTextRun> text_runs;
    UiImage portrait;
    std::uint32_t characters{},shown{},page{},page_start{},page_end{};
    float font_extent{};
    std::array<float,4> text_rect{},portrait_rect{};
    std::vector<DialogTextNode> text_nodes;
    std::optional<DialogTypingCursor> typing;
    std::array<float,4> click_rect{};
};
// A single game-thread owner. UI callbacks must carry both generation and id.
class DialogServices final:public IScriptContextServices {
public:
    DialogServices(IScriptContextServices& next,DialogAssets& assets,std::uint64_t generation);
    ~DialogServices();
    void Prepare();
    void OnCall(const ScriptRequest& request) override;
    ScriptResponse Invoke(const ScriptRequest& request) override;
    bool Configure(std::uint64_t generation,std::uint64_t id,std::span<const std::uint32_t> page_ends,float font_extent);
    // 由字体排版器提交节点/行/分页结果；共享服务负责逐字、确认与脚本等待。
    bool ConfigureRich(std::uint64_t generation,std::uint64_t id,std::span<const DialogPageLayout> pages,float font_extent);
    bool Confirm(std::uint64_t generation,std::uint64_t id);
    void Advance(float elapsed);
    std::optional<ScriptToken> TakeCompletion();
    std::uint64_t Revision() const;
    DialogView View() const;
    std::vector<std::string> ResourceMembers() const;
    void WriteReport(std::ostream& out) const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
} // namespace pal4::portable
