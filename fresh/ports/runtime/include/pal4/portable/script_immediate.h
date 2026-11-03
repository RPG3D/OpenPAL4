#pragma once
#include <cstdint>
#include <memory>
namespace pal4::portable {
// 原活动脚本记录的+0x10C/+0x110；只保存一层previous，不是模式栈。
class ScriptImmediateState {
public:
    ScriptImmediateState();
    ~ScriptImmediateState();
    ScriptImmediateState(const ScriptImmediateState&)=delete;
    ScriptImmediateState& operator=(const ScriptImmediateState&)=delete;
    void Begin();
    void End();
    std::uint32_t Current() const;
    std::uint32_t Saved() const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
