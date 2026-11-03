#include "pal4/portable/arena_skill_state.h"
#include "pal4/arena_skill_manager.h"
#include "pal4/script_arena_skill_state.h"
#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <stdexcept>
#include <vector>

namespace pal4::portable {
struct ArenaSkillState::Impl final:pal4::IArenaSkillRuntime,
    pal4::IScriptArenaSkillStateRuntime {
    std::map<std::uint32_t,std::vector<std::uint8_t>> blocks;
    std::uint32_t next_address{0x10000},manager{};
    bool busy{};

    Impl():manager(pal4::ArenaSkillManager::Create(*this)) {
        if(!manager)throw std::bad_alloc();
    }

    std::pair<std::vector<std::uint8_t>&,std::size_t> Block(std::uint32_t address,std::size_t size) {
        auto it=blocks.upper_bound(address);
        if(it==blocks.begin())throw std::out_of_range("arena skill memory address");
        --it;
        const auto offset=static_cast<std::size_t>(address-it->first);
        if(offset>it->second.size() || size>it->second.size()-offset)
            throw std::out_of_range("arena skill memory range");
        return {it->second,offset};
    }
    void PreflightCleanup() {
        const auto check_record=[&](std::uint32_t record) {
            if(ReadWord(record+32u))throw std::logic_error("arena skill actor cleanup owner is not bound");
            if(ReadWord(record+36u) || ReadWord(record+40u))
                throw std::logic_error("arena skill effect content owner is not bound");
        };
        for(auto cursor=ReadWord(manager+4u);cursor!=ReadWord(manager+8u);cursor+=4u)
            check_record(ReadWord(cursor));
        for(auto cursor=ReadWord(manager+20u);cursor!=ReadWord(manager+24u);cursor+=4u) {
            const auto record=ReadWord(cursor);
            if(record)check_record(record);
        }
        for(auto cursor=ReadWord(manager+36u);cursor!=ReadWord(manager+40u);cursor+=12u)
            check_record(ReadWord(cursor+8u));
    }
    std::uint32_t ArenaSkillManager() override{return manager;}
    std::uint32_t CleanupPlayerData(std::uint32_t selected) override {
        if(selected!=manager)throw std::invalid_argument("arena skill manager owner mismatch");
        if(busy)throw std::logic_error("arena skill state reentered");
        PreflightCleanup();
        struct Guard{bool& value;explicit Guard(bool& v):value(v){value=true;}~Guard(){value=false;}} guard(busy);
        return pal4::ArenaSkillManager::CleanupPlayerData(manager,*this);
    }
    std::uint32_t UpdatePlayerData(std::uint32_t) override {
        throw std::logic_error("arena skill restore requires scene clump and effect owners");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        const auto [block,offset]=Block(address,4);
        return std::uint32_t(block[offset]) | (std::uint32_t(block[offset+1])<<8) |
            (std::uint32_t(block[offset+2])<<16) | (std::uint32_t(block[offset+3])<<24);
    }
    void WriteWord(std::uint32_t address,std::uint32_t value) override {
        auto [block,offset]=Block(address,4);
        for(unsigned i=0;i<4;++i)block[offset+i]=static_cast<std::uint8_t>(value>>(i*8));
    }
    void WriteByte(std::uint32_t address,std::uint8_t value) override {
        auto [block,offset]=Block(address,1);block[offset]=value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        if(!size)throw std::invalid_argument("zero-sized arena skill allocation");
        const auto aligned=(std::uint64_t(size)+15u)&~std::uint64_t(15u);
        if(aligned>std::numeric_limits<std::uint32_t>::max()-next_address)
            throw std::overflow_error("arena skill address space exhausted");
        const auto address=next_address;next_address+=static_cast<std::uint32_t>(aligned);
        blocks.emplace(address,std::vector<std::uint8_t>(size));return address;
    }
    std::uint32_t Free(std::uint32_t address) override {
        if(!address)return 0;
        if(!blocks.erase(address))throw std::logic_error("arena skill allocation is not owned");
        return 0;
    }
    std::uint32_t CreateArenaActor() override {
        throw std::logic_error("arena skill actor factory is not bound");
    }
    std::uint32_t Call(pal4::ArenaActorCall,std::initializer_list<std::uint32_t>) override {
        throw std::logic_error("arena skill actor operation is not bound");
    }
    std::uint32_t RegisterSingleton(std::string_view,std::uint32_t,std::uint32_t,std::uint32_t) override {
        throw std::logic_error("arena skill singleton registration belongs to the application");
    }
    std::uint32_t SkillCall(pal4::ArenaSkillCall operation,
        std::initializer_list<std::uint32_t> arguments) override {
        if(operation==pal4::ArenaSkillCall::NeedsContentUpdate) {
            if(arguments.size()!=1)throw std::invalid_argument("arena skill content handle address");
            const auto address=*arguments.begin();
            if(ReadWord(address) || ReadWord(address+4u))
                throw std::logic_error("arena skill effect content owner is not bound");
            return 0;
        }
        if(operation==pal4::ArenaSkillCall::ResetEffectHandle) {
            if(arguments.size()!=1)throw std::invalid_argument("arena skill reset handle address");
            const auto address=*arguments.begin();
            if(ReadWord(address) || ReadWord(address+4u))
                throw std::logic_error("arena skill effect content owner is not bound");
            return 0;
        }
        throw std::logic_error("arena skill scene operation is not bound");
    }
    void PlayActionSound(std::string_view) override {
        throw std::logic_error("arena skill action sound is not bound");
    }
    void Close() {
        if(!manager)return;
        if(busy)throw std::logic_error("cannot close an executing arena skill state");
        PreflightCleanup();
        struct Guard{bool& value;explicit Guard(bool& v):value(v){value=true;}~Guard(){value=false;}} guard(busy);
        (void)pal4::ArenaSkillManager::ClearActiveObjects(manager,*this);
        (void)pal4::ArenaSkillManager::Destroy(manager,*this);
        manager=0;
    }
};

ArenaSkillState::ArenaSkillState():impl_(std::make_unique<Impl>()){}
ArenaSkillState::~ArenaSkillState()=default;
std::uint32_t ArenaSkillState::HideSkillObject(){
    if(!impl_ || !impl_->manager)throw std::logic_error("arena skill owner is unavailable");
    return pal4::ScriptArenaSkillState::HideSkillObject(*impl_);
}
void ArenaSkillState::Close(){if(impl_)impl_->Close();}
} // namespace pal4::portable
