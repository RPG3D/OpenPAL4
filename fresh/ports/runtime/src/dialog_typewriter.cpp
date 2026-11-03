#include "pal4/portable/dialog_typewriter.h"
#include <cmath>
#include <stdexcept>
#include <utility>

namespace pal4::portable {
void DialogTypewriter::Configure(std::span<const DialogRevealNode> nodes,std::uint32_t begin,std::uint32_t end){
    if(nodes.empty() || nodes.size()>131072 || begin>end)throw std::invalid_argument("invalid dialogue typing page");
    auto previous=begin;std::uint32_t line=0;
    for(const auto& node:nodes){
        if(node.begin!=previous || node.end<node.begin || node.end>end || node.line<line || node.line-line>1 || (&node==nodes.data() && node.line!=0) || (node.line_break && node.end-node.begin>1))
            throw std::invalid_argument("invalid dialogue typing node");
        previous=node.end;line=node.line;
    }
    if(previous!=end)throw std::invalid_argument("incomplete dialogue typing page");
    std::vector<DialogRevealNode> next(nodes.begin(),nodes.end());nodes_=std::move(next);
    end_=end;shown_=begin;position_=0;cursor_={};elapsed_=0;
}
bool DialogTypewriter::Advance(float seconds,float interval){
    if(!std::isfinite(seconds) || seconds<0 || !std::isfinite(interval) || interval<=0)throw std::invalid_argument("invalid dialogue typing time");
    if(nodes_.empty() || cursor_.finished)return false;
    elapsed_+=seconds;if(elapsed_<=interval)return false;
    elapsed_=0; // 原控件每次更新最多处理一个字符或一个节点/行转换。
    if(position_<nodes_.size() && nodes_[position_].line==cursor_.line){
        const auto& node=nodes_[position_];
        if(!node.line_break && cursor_.character<node.end-node.begin){++cursor_.character;shown_=node.begin+cursor_.character;}
        else{shown_=node.end;++position_;++cursor_.node;cursor_.character=0;}
    }else{
        ++cursor_.line;cursor_.node=cursor_.character=0;
        cursor_.finished=position_==nodes_.size();
    }
    return true;
}
void DialogTypewriter::FinishLine(){
    if(nodes_.empty() || cursor_.finished)return;
    while(position_<nodes_.size() && nodes_[position_].line==cursor_.line){shown_=nodes_[position_].end;++position_;}
    ++cursor_.line;cursor_.node=cursor_.character=0;
    cursor_.finished=position_==nodes_.size();
}
void DialogTypewriter::Finish(){
    if(nodes_.empty())return;
    shown_=end_;position_=nodes_.size();cursor_={nodes_.back().line+1,0,0,true};elapsed_=0;
}
}
