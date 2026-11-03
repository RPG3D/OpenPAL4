#include "pal4/portable/script_immediate.h"
#include "pal4/script_builtin_handlers.h"
#include <stdexcept>
namespace pal4::portable {
struct ScriptImmediateState::Impl final:pal4::IScriptImmediateModeRuntime {
    std::uint32_t current{},saved{};
    std::uint32_t ScriptInstance() override{return 0x1000;}
    std::uint32_t SetDebuggerMode(std::uint32_t debugger,std::uint32_t mode) override {
        if(debugger!=0x1054 || mode>1)throw std::logic_error("invalid immediate mode request");
        // IDA7DF440: this owner exists only for a prepared/active stack frame.
        saved=current;current=mode;return 0x2000;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if(address==0x1060)return 0x2000;
        if(address==0x210c)return current;
        if(address==0x2110)return saved;
        throw std::logic_error("invalid immediate mode read");
    }
    void WriteWord(std::uint32_t address,std::uint32_t value) override {
        if(address==0x210c)current=value;
        else if(address==0x2110)saved=value;
        else throw std::logic_error("invalid immediate mode write");
    }
};
ScriptImmediateState::ScriptImmediateState():impl_(std::make_unique<Impl>()){}
ScriptImmediateState::~ScriptImmediateState()=default;
void ScriptImmediateState::Begin(){(void)pal4::ScriptImmediateMode::Begin(*impl_);}
void ScriptImmediateState::End(){(void)pal4::ScriptImmediateMode::End(*impl_);}
std::uint32_t ScriptImmediateState::Current() const{return impl_->current;}
std::uint32_t ScriptImmediateState::Saved() const{return impl_->saved;}
}
