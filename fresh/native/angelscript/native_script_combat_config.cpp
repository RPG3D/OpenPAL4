#include "pal4/native_script_combat_config.h"
#include <stdexcept>

namespace pal4 {
NativeScriptCombatConfig::NativeScriptCombatConfig() {
    // IDA579770: five words 0/0/0/0/1, empty texts, flags 1/0/0.
    config_.words[4] = 1u;
    config_.flags = {1u, 0u, 0u};
}
void NativeScriptCombatConfig::SetText(CombatConfigTextSlot slot, std::string_view text) {
    if(live_){live_->SetText(slot,text);return;}
    const auto index = static_cast<std::size_t>(slot);
    if (index >= config_.texts.size()) throw std::invalid_argument("combat text slot");
    // Public STL assignment replaces VC6 private COW ownership. A temporary
    // also permits a view into the previously stored value.
    std::string copy(text);
    config_.texts[index].value.swap(copy);
}
std::uint32_t NativeScriptCombatConfig::CombatSystem() { return live_?live_->CombatSystem():kOwner; }
std::uint32_t NativeScriptCombatConfig::ReadWord(std::uint32_t address) {
    if(live_)return live_->ReadWord(address);
    const auto offset = address - (kOwner + 432u);
    if (offset >= 20u || offset % 4u) throw std::invalid_argument("combat configuration word owner");
    return config_.words[offset / 4u];
}
CombatConfigText NativeScriptCombatConfig::ReadTextProperty(std::uint32_t address) {
    if(live_)return live_->ReadTextProperty(address);
    const auto offset = address - (kOwner + 452u);
    if (offset >= 48u || offset % 16u) throw std::invalid_argument("combat configuration text owner");
    return config_.texts[offset / 16u];
}
void NativeScriptCombatConfig::ApplyConfiguration(const CombatConfigSnapshot& config) {
    if(live_){live_->ApplyConfiguration(config);return;}
    auto copy = config;
    using std::swap;
    swap(config_, copy);
}
std::uint32_t NativeScriptCombatConfig::ReleaseText(CombatConfigText& text, std::uint32_t) noexcept {
    text.value.clear();
    return 0u; // Original private string cleanup return is discarded by the void SDK callback.
}
CombatConfigSnapshot NativeScriptCombatConfig::Snapshot()const{return live_?live_->Snapshot():config_;}
void NativeScriptCombatConfig::SetCombatProperty110(std::uint32_t value){auto copy=Snapshot();copy.words[2]=value;ApplyConfiguration(copy);}
void NativeScriptCombatConfig::Bind(ICombatConfigurationState& live){
    if(live_ || &live==this)throw std::logic_error("Combat configuration already bound or recursive");
    live.ApplyConfiguration(config_);live_=&live;
}
void NativeScriptCombatConfig::Unbind()noexcept{live_=nullptr;}

}
