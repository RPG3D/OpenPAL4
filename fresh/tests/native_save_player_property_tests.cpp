#include "pal4/native_save_player_property_runtime.h"
#include "pal4/save_ui_state.h"
#include "pal4/native_save_ui_exit_runtime.h"
#include <cstring>

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void TestSavePlayerProperties() {
    pal4::PlayerManagerObjectState players{};
    const auto manager = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&players));
    const auto component = manager + pal4::PlayerManagerObject::kElementBytes +
                           1620u;
    const auto word = [&](std::uint32_t index) -> std::uint32_t& {
        return players.Word((pal4::PlayerManagerObject::kElementBytes +
                             1620u + index * 4u) / 4u);
    };
    word(86u) = 80u;
    word(88u) = 40u;
    word(86u + 4u) = 31u;
    word(136u + 4u) = 2u;
    word(136u + 7u) = 2u;
    word(136u + 9u) = 3u;

    pal4::NativeSavePlayerPropertyRuntime runtime(players);
    Check(runtime.GetMaximum(component, 0u) == 80u &&
          runtime.GetMaximum(component, 2u) == 40u,
          "Save getter reads player component maxima");
    Check(runtime.GetMaximum(component, 4u) == 31u &&
          runtime.GetCurrent(component, 4u) == 2u &&
          runtime.GetCurrent(component, 10u) == 10u,
          "archive level uses current property slot, independent of maximum stat");
    runtime.Set(component, 7u, runtime.GetMaximum(component, 0u));
    runtime.Set(component, 9u, runtime.GetMaximum(component, 2u));
    Check(word(136u + 7u) == 80u && word(136u + 9u) == 40u &&
          word(162u) == 0u,
          "Save copies maxima into current values and refreshes health");

    runtime.Set(component, 7u, 100u);
    runtime.Set(component, 9u, 100u);
    Check(word(136u + 7u) == 80u && word(136u + 9u) == 40u,
          "Save setters preserve original bounds");
    runtime.Set(component, 7u, 0u);
    Check(word(162u) == 3u,
          "zero health updates the component status");

    bool rejected = false;
    try {
        (void)runtime.GetMaximum(component + 4u, 0u);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    Check(rejected, "foreign component address is rejected");
}

class SaveEffects final : public pal4::IScriptEffectRuntime {
public:
    explicit SaveEffects(pal4::PlayerManagerObjectState& players) : players_(players) {}
    std::uint32_t EffectManager() override { return 0x9000u; }
    pal4::ScriptEffectHandle CreateAtAddress(std::uint32_t manager,std::uint32_t name,
        std::uint32_t position,std::uint32_t repeats,std::uint32_t flag) override {
        const auto base=reinterpret_cast<std::uint32_t>(&players_);
        Check(manager==0x9000u && std::strcmp(reinterpret_cast<const char*>(name),"H_XS027")==0 &&
              position==base+2u*2836u+172u && repeats==1u && flag==0u,
              "production Save exit effect uses selected player, original name/repeat/flag");
        Check(players_.Word((1620u+143u*4u)/4u)==60u &&
              players_.Word((2u*2836u+1620u+145u*4u)/4u)==32u,
              "party recovery precedes actual effect creation");
        ++created;return {123u,0x9000u};
    }
    std::uint32_t ResetHandle(pal4::ScriptEffectHandle& handle) override {
        Check(handle.first==123u && handle.second==0x9000u,"Save exit resets the returned temporary handle");
        handle={};++resets;return 0u;
    }
    std::uint32_t HandleScriptCallback(std::uint32_t,std::int32_t) override { throw std::logic_error("unexpected CG callback"); }
    std::uint32_t CleanupScriptCallback(std::uint32_t) override { throw std::logic_error("unexpected CG cleanup"); }
    std::uint32_t ReadWord(std::uint32_t) override { throw std::logic_error("unexpected script field"); }
    std::uint32_t EmptyStringAddress() override { throw std::logic_error("unexpected script name"); }
    std::uint32_t PlayerManager() override { throw std::logic_error("unexpected effect player manager"); }
    std::uint32_t PlayerByIndex(std::uint32_t,std::int32_t) override { throw std::logic_error("unexpected effect player lookup"); }
    std::uint32_t NpcManager() override { throw std::logic_error("unexpected NPC manager"); }
    std::uint32_t FindNpc(std::uint32_t,std::uint32_t) override { throw std::logic_error("unexpected NPC lookup"); }
    pal4::ScriptEffectHandle CreateAtCoordinates(std::uint32_t,std::uint32_t,const std::array<float,3>&,
        std::uint32_t,std::uint32_t) override { throw std::logic_error("Save exit retains position-address seam"); }
    std::uint32_t created{},resets{};
private:
    pal4::PlayerManagerObjectState& players_;
};

void TestSaveFinishOnNativePlayers() {
    pal4::PlayerManagerObjectState players{};
    players.Word(2824u / 4u) = 1u;
    players.Word((pal4::PlayerManagerObject::kElementBytes + 2824u) / 4u) =
        0u;
    players.Word((2u * pal4::PlayerManagerObject::kElementBytes + 2824u) /
                 4u) = 1u;
    players.Word(11344u / 4u) = 2u;
    players.Word((2836u+1620u+143u*4u)/4u)=777u;
    players.Word((2836u+1620u+145u*4u)/4u)=888u;
    for (const auto index : {0u, 2u}) {
        const auto base =
            (index * pal4::PlayerManagerObject::kElementBytes + 1620u) / 4u;
        players.Word(base + 86u) = 60u + index;
        players.Word(base + 88u) = 30u + index;
    }
    SaveEffects effects(players);
    pal4::NativeSaveUiExitRuntime runtime(players,effects);
    Check(pal4::SaveUiState::Finish(0u, runtime) == 1u &&
              effects.created == 1u && effects.resets == 1u && runtime.EffectWasCreated(),
          "Save Finish invokes the production effect adapter exactly once");
    Check(players.Word((2836u+1620u+143u*4u)/4u)==777u &&
              players.Word((2836u+1620u+145u*4u)/4u)==888u,
          "inactive player current properties remain untouched");
    bool rejected=false;try{runtime.ReadWord(reinterpret_cast<std::uint32_t>(&players)+sizeof(players));}
    catch(const std::invalid_argument&){rejected=true;}
    Check(rejected,"Save exit rejects foreign fields");
    for (const auto index : {0u, 2u}) {
        const auto base =
            (index * pal4::PlayerManagerObject::kElementBytes + 1620u) / 4u;
        Check(players.Word(base + 143u) == 60u + index &&
                  players.Word(base + 145u) == 30u + index &&
                  players.Word(base + 162u) == 0u,
              "Save Finish restores active native player properties");
    }
}

}  // namespace

int main() {
    try {
        TestSavePlayerProperties();
        TestSaveFinishOnNativePlayers();
        std::cout << "native Save player properties passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
