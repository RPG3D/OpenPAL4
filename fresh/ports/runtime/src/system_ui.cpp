#include "pal4/portable/system_ui.h"
#include "pal4/script_system_ui.h"
#include "pal4/game_system_data.h"
#include <ostream>
#include <stdexcept>
#include <utility>
#include <vector>
namespace pal4::portable {
void GameDataEntries::InitializeSceneUiDefaults() {
    using Defaults=pal4::GameSystemData;
    for(std::uint32_t offset=0;offset<Defaults::kInitialUiPermissionEntryCount;++offset)
        Set(Defaults::kFirstInitialUiPermissionEntry+offset,Defaults::kInitialUiPermission);
    Set(Defaults::kMinimapSkillPermissionEntry,Defaults::kInitialMinimapSkillPermission);
}
void GameDataEntries::Set(std::uint32_t index,std::uint32_t value){entries_.at(index)=value;}
std::optional<std::uint32_t> GameDataEntries::Get(std::uint32_t index) const{return entries_.at(index);}
void GameDataEntries::WriteReport(std::ostream& output) const {
    output<<'[';bool first=true;
    for(unsigned i=0;i<entries_.size();++i) {
        if(!entries_[i])continue;
        if(!first)output<<',';
        first=false;
        output<<"{\"entry\":"<<i<<",\"value\":"<<*entries_[i]<<'}';
    }
    output<<']';
}
struct SystemUiPermissions::Impl final:pal4::IScriptSystemUiRuntime {
    GameDataEntries& data;std::vector<std::pair<unsigned,unsigned>> writes;unsigned calls{};
    explicit Impl(GameDataEntries& d):data(d){}
    std::uint32_t Game() override{return 1;}
    void SetGameDataEntry(std::uint32_t game,std::uint32_t entry,std::uint8_t value) override {
        if(game!=1)throw std::logic_error("invalid system UI game handle");
        data.Set(entry,value);writes.emplace_back(entry,value);
    }
    void InitializeStateAndUi(std::uint32_t,std::uint32_t,std::uint32_t) override{throw std::logic_error("system UI transitions are not bound");}
    std::uint32_t UiFrameManager() override{throw std::logic_error("system UI frame manager is not bound");}
    void ProcessEvent(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override{throw std::logic_error("system UI events are not bound");}
    void SetFrameFlag(std::uint32_t,std::uint8_t) override{throw std::logic_error("system UI portraits are not bound");}
};
SystemUiPermissions::SystemUiPermissions(GameDataEntries& d):impl_(std::make_unique<Impl>(d)){}
SystemUiPermissions::~SystemUiPermissions()=default;
void SystemUiPermissions::Grant(std::int32_t index,bool enabled) {
    ++impl_->calls;pal4::ScriptSystemUi::Grant(static_cast<std::uint32_t>(index),enabled,*impl_);
}
void SystemUiPermissions::GrantMagic(bool first,bool second) {
    ++impl_->calls;pal4::ScriptSystemUi::GrantMagic(first,second,*impl_);
}
void SystemUiPermissions::WriteReport(std::ostream& output) const {
    output<<"{\"calls\":"<<impl_->calls<<",\"writes\":[";bool first=true;
    for(const auto& [entry,value]:impl_->writes) {
        if(!first)output<<',';
        first=false;output<<"{\"entry\":"<<entry<<",\"value\":"<<value<<'}';
    }
    output<<"]}";
}
}
