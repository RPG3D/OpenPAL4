#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
namespace pal4::portable {
// begin/end为可见Unicode码点的半开范围；color为ARGB。
struct DialogTextRun {std::uint32_t begin{},end{},color{};};
// XML 文本节点边界；即使相邻颜色相同也不合并，原控件按节点度量和推进。
struct DialogTextNode {std::uint32_t begin{},end{};};
struct DialogText {std::string text;std::vector<DialogTextRun> runs;std::uint32_t characters{};std::vector<DialogTextNode> nodes;};
// TinyXML负责语法；颜色语义以用户原OIRAMLOOK公共API探针为据。
DialogText ParseDialogText(std::string_view source,std::uint32_t normal_color,bool hint_font_fallback=false);
}
