#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pal4::portable {
struct UiSequenceFrame {float duration{};std::string imageset,image;};
struct UiSequenceDefinition {std::string name;std::uint32_t loop_count{};std::vector<UiSequenceFrame> frames;};
UiSequenceDefinition ParseUiSequence(std::string_view xml);
// 原资源管理器共享播放状态；多个控件应引用同一实例，不能各自复制时钟。
class UiSequencePlayback {
public:
    explicit UiSequencePlayback(UiSequenceDefinition definition);
    std::size_t Advance(float seconds);
    const UiSequenceDefinition& Definition()const{return definition_;}
    std::size_t FrameIndex()const{return frame_;}
    float Elapsed()const{return elapsed_;}
    bool Finished()const{return definition_.loop_count && loops_>=definition_.loop_count;}
private:
    UiSequenceDefinition definition_;
    std::size_t frame_{};
    std::uint32_t loops_{};
    float duration_{},elapsed_{};
};
}
