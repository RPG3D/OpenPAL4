#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace pal4::portable {
// 排版后的文本/换行节点，范围为全段 Unicode 码点索引；行号相对当前页。
// 显式 LF 消耗一个码点；原 XmlAutoBreakProcessor 的换行节点不消耗正文码点。
struct DialogRevealNode {std::uint32_t begin{},end{},line{};bool line_break{};};
struct DialogPageLayout {std::uint32_t end{};std::vector<DialogRevealNode> nodes;};
struct DialogTypingCursor {std::uint32_t line{},node{},character{};bool finished{};};
// 只负责已排版节点的逐字状态，不依赖字体、CEGUI、CPK 或平台时钟。
class DialogTypewriter {
public:
    void Configure(std::span<const DialogRevealNode> nodes,std::uint32_t begin,std::uint32_t end);
    bool Advance(float seconds,float interval);
    // OLSDialogEditbox changeLine: reveal the current line, retain clock remainder.
    void FinishLine();
    void Finish();
    std::uint32_t Shown()const{return shown_;}
    DialogTypingCursor Cursor()const{return cursor_;}
private:
    std::vector<DialogRevealNode> nodes_;
    std::uint32_t end_{},shown_{};
    std::size_t position_{};
    DialogTypingCursor cursor_{};
    float elapsed_{};
};
}
