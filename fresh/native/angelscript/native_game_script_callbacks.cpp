#include "pal4/script_quest_dialog.h"
#include "pal4/script_cache_context.h"
#include "pal4/script_inn_dialog.h"
#include "pal4/script_dialogs.h"
#include "pal4/script_common_dialog_selection.h"
#include "pal4/native_script_select_dialog_runtime.h"
#include "pal4/script_poetry_wait.h"
#include "pal4/script_world_map_wait.h"
#include "pal4/world_map_buttons.h"
#include "pal4/script_trade_npc_behavior.h"
#include "pal4/script_npc_hand_effect.h"
#include "pal4/script_player_gob_attachment.h"
#include "pal4/script_player_blend.h"
#include "pal4/script_monster_blend.h"
#include "pal4/native_game_script_callbacks.h"
#include "pal4/script_puzzle_game.h"
#include "pal4/script_ui_state_callbacks.h"
#include "pal4/script_action_hold_release.h"
#include "pal4/script_timed_state.h"
#include "pal4/script_weather.h"
#include "pal4/script_pet_visibility.h"
#include "pal4/script_inventory_change.h"
#include "pal4/script_money_transaction.h"
#include "pal4/script_inventory_query.h"
#include "pal4/script_hint_display.h"
#include "pal4/script_minimap_level.h"
#include "pal4/script_prescription_addition.h"
#include "pal4/script_party_membership.h"
#include "pal4/script_player_status.h"
#include "pal4/script_forbidden_skill.h"
#include "pal4/script_player_garb.h"
#include "pal4/script_player_favor.h"
#include "pal4/script_combat_config.h"
#include "pal4/script_combat_start.h"
#include "pal4/script_party_recovery.h"
#include "pal4/script_player_rest.h"
#include "pal4/script_party_magic_query.h"
#include "pal4/script_pal_test_result.h"
#include "pal4/script_arena_skill_state.h"

#include "pal4/background_music_player.h"
#include "pal4/script_bgm_callbacks.h"
#include <cstring>
#include "pal4/script_camera_callbacks.h"
#include "pal4/script_gameplay_callbacks.h"
#include "pal4/script_camera_sequences.h"
#include "pal4/script_player_actions.h"
#include "pal4/script_player_material.h"
#include "pal4/script_npc_material.h"
#include "pal4/script_shadow_control.h"
#include "pal4/script_current_player_visibility.h"
#include "pal4/script_player_action.h"
#include "pal4/script_player_equipment.h"
#include "pal4/script_player_position.h"
#include "pal4/script_player_rotation.h"
#include "pal4/native_script_entity_transform_runtime.h"
#include "pal4/script_player_turn.h"
#include "pal4/script_player_walk.h"
#include "pal4/script_object_visibility.h"
#include "pal4/script_object_touch.h"
#include "pal4/script_object_blend.h"
#include "pal4/script_object_action.h"
#include "pal4/script_object_timed_move.h"
#include "pal4/script_object_timed_scale.h"
#include "pal4/script_current_player_timed_move.h"
#include "pal4/script_visibility_query.h"
#include "pal4/script_gob_reset.h"
#include "pal4/script_gob_position.h"
#include "pal4/script_event_volume_visibility.h"
#include "pal4/script_object_research.h"
#include "pal4/script_effect_callbacks.h"
#include "pal4/script_system_ui.h"
#include "pal4/script_entity_callbacks.h"
#include "pal4/script_monster_visibility.h"
#include "pal4/script_builtin_forwarders.h"
#include "pal4/game_arena_load_handler.h"
#include "pal4/game_arena_ready_handler.h"
#include "pal4/script_builtin_handlers.h"
#include "pal4/script_flash_effect.h"
#include "pal4/comparison_gob_script_trace.h"
#include "pal4/script_global_callbacks.h"
#include "pal4/script_movie_player.h"
#include "pal4/script_night_time_query.h"
#include "pal4/script_music_transport.h"
#include "pal4/script_sound_2d.h"
#include "pal4/script_player_skill_acquisition.h"
#include "pal4/script_command_queue.h"
#include "pal4/native_script_random_talk_pool.h"
#include "pal4/script_random_player_position.h"
#include "pal4/script_world_misc.h"
#include "pal4/native_crt_random.h"
#include "pal4/script_wait_handler.h"
#include "pal4/script_talk_handler.h"
#include "pal4/script_tutorial_wait.h"
#include "pal4/script_trade_wait.h"
#include "pal4/script_scenario_portrait.h"
#include "pal4/script_ui_countdown.h"
#include "pal4/script_ui_timer_save_data.h"
#include "scriptstring.h"

#include <bit>
#include <array>
#include <stdexcept>
#include <string>
#include <string_view>

namespace pal4 {
namespace {

std::uint32_t AddressOf(const void* object) {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(object));
}

// Project SDK strings onto the shared legacy query without assuming their ABI.
// Tokens identify this adapter's owned strings, never process memory addresses.
class BgmAreaQuery final : public IScriptBgmAreaRuntime {
public:
    BgmAreaQuery(std::string_view name, std::string_view area)
        : name_(name), area_(area) {}
    std::uint32_t ArenaManager() override { return 0x10u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        if (address == 0xf0u) return 1u;
        if (address == 4u) return 2u;
        throw std::out_of_range("PAL4 BGM area query token");
    }
    std::uint32_t EmptyStringAddress() override { return 2u; }
    std::int32_t CompareIgnoreCase(std::uint32_t lhs, std::uint32_t rhs) override {
        if (lhs != 2u || rhs != 1u)
            throw std::out_of_range("PAL4 BGM area comparison token");
        return _stricmp(name_.c_str(), area_.c_str());
    }
private:
    std::string name_, area_;
};

// Adapts the shared query to the existing scene visibility service. Payload0
// and text1 are local tokens, never addresses or legacy string allocations.
class ObjectVisibilityQuery final : public IScriptVisibilityQueryRuntime {
public:
    ObjectVisibilityQuery(IScriptObjectVisibilityRuntime& objects, std::string_view name)
        : objects_(objects), name_(name) {}
    std::uint32_t ReadWord(std::uint32_t address) override {
        return address == 4u ? 1u : objects_.ReadWord(address);
    }
    std::uint32_t EmptyStringAddress() override { return 1u; }
    std::uint32_t ObjectManager() override { return objects_.ObjectManager(); }
    std::uint32_t FindObject(std::uint32_t manager, std::uint32_t text) override {
        if (text != 1u) throw std::invalid_argument("object visibility query text");
        return objects_.FindByName(manager, name_);
    }
    std::uint32_t MonsterManager() override {
        throw std::logic_error("object visibility query has no monster manager");
    }
    std::uint32_t FindMonster(std::uint32_t, std::uint32_t) override {
        throw std::logic_error("object visibility query has no monster lookup");
    }
private:
    IScriptObjectVisibilityRuntime& objects_;
    std::string_view name_;
};

// Payload0/text1 are call-local tokens, not a fabricated VC6 string object.
class GobPositionArguments final : public IScriptGobPositionRuntime {
public:
    GobPositionArguments(IScriptObjectTimedMoveRuntime& objects,std::string_view name)
        : objects_(objects),name_(name) {}
    std::uint32_t ReadWord(std::uint32_t address) override {
        if(address!=4u)throw std::invalid_argument("GOB position argument token");
        return name_.empty()?0u:1u;
    }
    std::uint32_t EmptyStringAddress() override { return 1u; }
    std::uint32_t ObjectManager() override { return objects_.ObjectManager(); }
    std::uint32_t FindObject(std::uint32_t manager,std::uint32_t text) override {
        if(text!=1u)throw std::invalid_argument("GOB position text token");
        return objects_.FindByName(manager,name_);
    }
    std::uint32_t SetPosition(std::uint32_t object,const std::array<float,3>& position,
        std::uint32_t flags) override {
        objects_.SetPosition(object,position,flags);
        // The native facet and script signature are void. Do not expose an
        // invented legacy setter register return or repeat the transform call.
        return 0u;
    }
private:
    IScriptObjectTimedMoveRuntime& objects_;
    std::string_view name_;
};

// Call-local tokens project the three modern script strings onto the shared
// algorithm's ReadWord boundary. No VC6 string allocation/layout is fabricated.
class NpcHandArguments final : public IScriptNpcHandEffectRuntime {
public:
    NpcHandArguments(IScriptNpcHandEffectRuntime& target,
        std::string_view first, std::string_view second = {}, std::string_view third = {})
        : target_(target), text_{std::string(first), std::string(second), std::string(third)} {}
    std::uint32_t ReadWord(std::uint32_t address) override {
        for (unsigned i=0; i<3; ++i) {
            if (address == 16u*(i+1u)+4u) return AddressOf(text_[i].c_str());
            if (address == 16u*(i+1u)+8u) return static_cast<std::uint32_t>(text_[i].size());
        }
        return target_.ReadWord(address);
    }
    std::uint32_t EmptyStringAddress() override { return target_.EmptyStringAddress(); }
    bool NameDiffersFromEmptyError(std::uint32_t token, std::uint32_t length) override {
        if (token != 16u || length != text_[0].size())
            throw std::invalid_argument("NPC hand argument token/length");
        return !text_[0].empty();
    }
    std::uint32_t NpcManager() override { return target_.NpcManager(); }
    std::uint32_t FindNpc(std::uint32_t manager, std::uint32_t name) override { return target_.FindNpc(manager,name); }
    std::uint32_t SetupLeftHandEffect(std::uint32_t npc, std::uint32_t name) override { return target_.SetupLeftHandEffect(npc,name); }
    std::uint32_t SetupRightHandEffect(std::uint32_t npc, std::uint32_t name) override { return target_.SetupRightHandEffect(npc,name); }
    std::uint32_t UpdateWeaponEffects(std::uint32_t npc) override { return target_.UpdateWeaponEffects(npc); }
    std::uint32_t ObjectManager() override { return target_.ObjectManager(); }
    std::uint32_t FindObject(std::uint32_t manager, std::uint32_t name) override { return target_.FindObject(manager,name); }
    std::uint32_t ArenaManager() override { return target_.ArenaManager(); }
    void RemoveAnimatedObject(std::uint32_t arena, std::uint32_t object) override { target_.RemoveAnimatedObject(arena,object); }
    std::uint32_t RenderUnit(std::uint32_t npc, std::uint32_t clump, const std::array<float,3>& scale,
        std::uint32_t name, bool left) override { return target_.RenderUnit(npc,clump,scale,name,left); }
    std::uint32_t RenderObject(std::uint32_t npc, std::uint32_t clump, std::uint32_t scale,
        std::uint32_t name, bool left) override { return target_.RenderObject(npc,clump,scale,name,left); }
    std::uint32_t CleanupWeapons(std::uint32_t npc) override { return target_.CleanupWeapons(npc); }
private:
    IScriptNpcHandEffectRuntime& target_;
    std::array<std::string,3> text_;
};

class PlayerHandArguments final : public IScriptPlayerGobAttachmentRuntime {
public:
    PlayerHandArguments(IScriptPlayerGobAttachmentRuntime& target,
        std::string_view first, std::string_view second)
        : target_(target), text_{std::string(first), std::string(second)} {}
    std::uint32_t ReadWord(std::uint32_t address) override {
        for (unsigned i=0; i<text_.size(); ++i) {
            if (address == 16u*(i+1u)+4u) return AddressOf(text_[i].c_str());
            if (address == 16u*(i+1u)+8u) return static_cast<std::uint32_t>(text_[i].size());
        }
        return target_.ReadWord(address);
    }
    bool NameDiffersFromEmptyError(std::uint32_t token, std::uint32_t length) override {
        if (token != 16u || length != text_[0].size())
            throw std::invalid_argument("player hand argument token/length");
        return !text_[0].empty();
    }
    std::uint32_t EmptyStringAddress() override { return target_.EmptyStringAddress(); }
    std::uint32_t PlayerManager() override { return target_.PlayerManager(); }
    std::uint32_t PlayerByIndex(std::uint32_t manager, std::int32_t index) override {
        return target_.PlayerByIndex(manager, index);
    }
    std::uint32_t ObjectManager() override { return target_.ObjectManager(); }
    std::uint32_t FindObject(std::uint32_t manager, std::uint32_t name) override { return target_.FindObject(manager,name); }
    std::uint32_t ArenaManager() override { return target_.ArenaManager(); }
    void RemoveAnimatedObject(std::uint32_t arena, std::uint32_t object) override { target_.RemoveAnimatedObject(arena,object); }
    std::uint32_t RenderUnit(std::uint32_t player, std::uint32_t clump, const std::array<float,3>& scale,
        std::uint32_t name, bool left) override { return target_.RenderUnit(player,clump,scale,name,left); }
    std::uint32_t RenderObject(std::uint32_t player, std::uint32_t clump, std::uint32_t scale,
        std::uint32_t name, bool left) override { return target_.RenderObject(player,clump,scale,name,left); }
    std::uint32_t CleanupWeapons(std::uint32_t player) override { return target_.CleanupWeapons(player); }
    std::uint32_t SetupLeftHandEffect(std::uint32_t player, std::uint32_t name) override { return target_.SetupLeftHandEffect(player,name); }
    std::uint32_t SetupRightHandEffect(std::uint32_t player, std::uint32_t name) override { return target_.SetupRightHandEffect(player,name); }
    std::uint32_t UpdateWeaponEffects(std::uint32_t player) override { return target_.UpdateWeaponEffects(player); }
private:
    IScriptPlayerGobAttachmentRuntime& target_;
    std::array<std::string,2> text_;
};

}  // namespace

void NativeGameScriptCallbacks::ResetImmediateMode() noexcept {
    immediate_record_ = {};
    immediate_record_.function_index = 1;
    immediate_instance_[24] = AddressOf(&immediate_record_);
}

std::uint32_t NativeGameScriptCallbacks::ScriptInstance() {
    return AddressOf(immediate_instance_.data());
}

std::uint32_t NativeGameScriptCallbacks::SetDebuggerMode(
    const std::uint32_t debugger, const std::uint32_t mode) {
    if (debugger != ScriptInstance() + 0x54u || mode > 1u)
        throw std::invalid_argument("PAL4 immediate script mode request");
    if (immediate_record_.function_index != 0) {
        immediate_record_.word_272 = immediate_record_.word_268;
        immediate_record_.word_268 = mode;
    }
    return AddressOf(&immediate_record_);
}

std::uint32_t NativeGameScriptCallbacks::ReadWord(
    const std::uint32_t address) {
    if (address == ScriptInstance() + 0x60u)
        return immediate_instance_[24];
    if (address == AddressOf(&immediate_record_) + 0x10cu)
        return immediate_record_.word_268;
    if (address == AddressOf(&immediate_record_) + 0x110u)
        return immediate_record_.word_272;
    throw std::out_of_range("PAL4 immediate script word read");
}

void NativeGameScriptCallbacks::WriteWord(
    const std::uint32_t address, const std::uint32_t value) {
    if (address == AddressOf(&immediate_record_) + 0x10cu)
        immediate_record_.word_268 = value;
    else if (address == AddressOf(&immediate_record_) + 0x110u)
        immediate_record_.word_272 = value;
    else
        throw std::out_of_range("PAL4 immediate script word write");
}

namespace {

template <typename T>
T& Require(T* service, int system_id) {
    if (service == nullptr)
        throw std::runtime_error("PAL4 game script service " +
                                 std::to_string(system_id) + " is unavailable");
    return *service;
}

std::string_view StringArgument(asIScriptGeneric& arguments, asUINT index) {
    const auto* value = static_cast<const asCScriptString*>(
        arguments.GetArgObject(index));
    if (value == nullptr)
        throw std::runtime_error("PAL4 game script string argument is null");
    return value->buffer;
}

}  // namespace

void NativeGameScriptCallbacks::Invoke(int system_id, asIScriptGeneric& arguments) {
    auto& trace=GetComparisonGobScriptTrace();
    if(!trace.Enabled() || (system_id!=-176 && system_id!=-302 && system_id!=-303 &&
                          system_id!=-298 && system_id!=-308)) {
        InvokeImplementation(system_id,arguments);return;
    }
    // AngelScript2.2 GetArgDWord returns any nonobject one-word slot verbatim,
    // including float slots (as_generic.cpp); no float conversion is needed.
    const auto name=StringArgument(arguments,0);
    std::array<std::uint32_t,5> words{};std::uint32_t count{};
    if(system_id==-176){
        count=3;for(unsigned i=0;i<count;++i)words[i]=arguments.GetArgDWord(i+1u);
    }else if(system_id==-302 || system_id==-303){
        count=2;words[0]=arguments.GetArgDWord(1);words[1]=arguments.GetArgDWord(2)&0xffu;
    }else if(system_id==-308){
        count=5;for(unsigned i=0;i<4;++i)words[i]=arguments.GetArgDWord(i+1u);
        words[4]=arguments.GetArgDWord(5)&0xffu;
    }else{
        const auto clip=StringArgument(arguments,1);count=clip.size()>8u?6u:5u;
        for(std::size_t i=0;i<clip.size() && i<8u;++i)
            words[i/4u]|=static_cast<std::uint32_t>(static_cast<unsigned char>(clip[i]))<<((i%4u)*8u);
        words[2]=static_cast<std::uint32_t>(clip.size());
        words[3]=arguments.GetArgDWord(2);words[4]=arguments.GetArgDWord(3)&0xffu;
    }
    trace.Record(system_id,false,name,words,count);
    InvokeImplementation(system_id,arguments);
    trace.Record(system_id,true,name,words,count);
}

void NativeGameScriptCallbacks::InvokeImplementation(int system_id, asIScriptGeneric& arguments) {
    switch (system_id) {
    case -150:
        ScriptPlayerGarb::ApplySecondToAll(Require(services_.player_garb,system_id));
        return;
    case -153:
        ScriptPlayerGarb::ApplyFirst(
            std::bit_cast<std::int32_t>(arguments.GetArgDWord(0)),
            Require(services_.player_garb,system_id));
        return;
    case -177:
        (void)ScriptCacheContext::ClearExceptCurrent(Require(services_.cache_context, system_id));
        return;
    case -73:
        (void)ScriptPlayerActions::DetachCollision(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            Require(services_.player_actions, system_id));
        return;
    case -186:
        (void)Require(services_.player_material, system_id).SetLingshaLegs(
            StringArgument(arguments, 0));
        return;
    case -187:
        (void)Require(services_.player_material, system_id).ApplyLingshaLegs();
        return;
    case -72:
        (void)ScriptPlayerActions::AttachCollision(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            Require(services_.player_actions, system_id));
        return;
    case -182:
        (void)Require(services_.player_material, system_id).SetFace(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            StringArgument(arguments, 1));
        return;
    case -183:
        (void)Require(services_.player_material, system_id).ApplyFace(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)));
        return;
    case -196:
        ScriptSystemUi::SetArenaSkillEnabled(
            static_cast<std::uint8_t>(arguments.GetArgDWord(0)),
            Require(services_.system_ui, system_id));
        return;
    case -241:
        Require(services_.random_talk, system_id).Push(StringArgument(arguments,0));
        return;
    case -242:
        (void)Require(services_.random_talk, system_id).Show(Require(services_.talk,system_id));
        return;
    case -243:
        Require(services_.random_talk, system_id).Clear();
        return;
    case -92:
        (void)ScriptCommandDispatcher::Push(arguments.GetArgDWord(0),
            arguments.GetArgDWord(1), Require(services_.commands, system_id));
        return;
    case -216: {
        const std::string callback(StringArgument(arguments,1));
        const std::array<std::uint32_t,2> record{0u,AddressOf(callback.c_str())};
        (void)ScriptTimedState::Start(std::bit_cast<std::uint32_t>(arguments.GetArgFloat(0)),
            AddressOf(record.data()),Require(services_.timer,system_id));
        return;
    }
    case -217:
        (void)ScriptTimedState::Terminate(Require(services_.timer,system_id));return;
    case -136:
        (void)ScriptEffectCallbacks::PlayCallback(std::bit_cast<std::int32_t>(arguments.GetArgDWord(0)),Require(services_.effects,system_id));return;
    case -137:
        (void)ScriptEffectCallbacks::StopCallback(Require(services_.effects,system_id));return;
    case -138:
    case -139:
    case -140:
    case -141: {
        auto& effects = Require(services_.effects, system_id);
        const std::string name(StringArgument(arguments, 0));
        const std::array<std::uint32_t, 2> payload{
            0u, AddressOf(name.c_str())};
        const auto name_payload = AddressOf(payload.data());
        const auto repeats = arguments.GetArgDWord(1);
        if (system_id == -138)
            (void)ScriptEffectCallbacks::Create(
                name_payload, repeats, arguments.GetArgFloat(2),
                arguments.GetArgFloat(3), arguments.GetArgFloat(4), effects);
        else if (system_id == -139)
            (void)ScriptEffectCallbacks::CreateOnPlayer(
                name_payload, repeats,
                std::bit_cast<std::int32_t>(arguments.GetArgDWord(2)), effects);
        else if (system_id == -140)
            (void)ScriptEffectCallbacks::CreateOnCurrentPlayer(
                name_payload, repeats, effects);
        else {
            const std::string npc(StringArgument(arguments, 2));
            const std::array<std::uint32_t, 2> npc_payload{
                0u, AddressOf(npc.c_str())};
            (void)ScriptEffectCallbacks::CreateOnNpc(
                name_payload, repeats, AddressOf(npc_payload.data()), effects);
        }
        return;
    }
    case -80:
        (void)MonsterPursuitController::StopAll(Require(services_.monster_pursuit,system_id));
        return;
    case -55:
        (void)ScriptImmediateMode::Begin(*this);
        return;
    case -56:
        (void)ScriptImmediateMode::End(*this);
        return;
    case -210: {
        const std::string name(StringArgument(arguments, 0));
        (void)ScriptGobReset::ByName(name.c_str(), Require(services_.gob_reset, system_id));
        return;
    }
    case -154: {
        ObjectVisibilityQuery query(Require(services_.object_visibility, system_id),
                                    StringArgument(arguments, 0));
        (void)arguments.SetReturnDWord(ScriptVisibilityQuery::Object(0u, query));
        return;
    }
    case -102: {
        const std::string name(StringArgument(arguments, 0));
        (void)ScriptObjectTouch::Run(name.c_str(), Require(services_.object_touch, system_id));
        return;
    }
    case -97: {
        const std::string name(StringArgument(arguments, 0));
        (void)ScriptObjectVisibility::SetVisible(
            name.c_str(), static_cast<std::uint8_t>(arguments.GetArgDWord(1)),
            Require(services_.object_visibility, system_id));
        return;
    }
    case -89: {
        class Random final : public IScriptRandomIntegerRuntime {
        public:
            std::int32_t Rand() override { return NativeCrtRandom(__FILE__); }
        } host;
        auto& runtime = services_.random_integer ? *services_.random_integer : host;
        (void)arguments.SetReturnDWord(static_cast<std::uint32_t>(
            ScriptWorldMisc::RandomInRange(
                static_cast<std::int32_t>(arguments.GetArgDWord(0)),
                static_cast<std::int32_t>(arguments.GetArgDWord(1)),runtime)));
        return;
    }
    case -147:
        (void)ScriptRandomPlayerPosition::MoveIndexed(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            arguments.GetArgFloat(1), arguments.GetArgFloat(2),
            Require(services_.random_player_position, system_id));
        return;
    case -148:
        (void)ScriptRandomPlayerPosition::MoveCurrent(
            arguments.GetArgFloat(0), arguments.GetArgFloat(1),
            Require(services_.random_player_position, system_id));
        return;
    case -149: {
        const std::string name(StringArgument(arguments, 0));
        const std::array<std::uint32_t, 2> payload{
            0u, AddressOf(name.c_str())};
        (void)ScriptEventVolumeVisibility::Set(
            AddressOf(payload.data()),
            static_cast<std::uint8_t>(arguments.GetArgDWord(1)),
            Require(services_.event_volume_visibility, system_id));
        return;
    }
    case -122: {
        const std::string name(StringArgument(arguments, 0));
        const std::array<std::uint32_t, 2> payload{
            0u, AddressOf(name.c_str())};
        (void)ScriptObjectResearch::Set(
            AddressOf(payload.data()), arguments.GetArgDWord(1),
            Require(services_.object_research, system_id));
        return;
    }
    case -121:
    case -146: {
        const std::string name(StringArgument(arguments, 0));
        const std::array<std::uint32_t, 2> payload{0u, AddressOf(name.c_str())};
        auto& runtime = Require(services_.monster_visibility, system_id);
        const auto flag = static_cast<std::uint8_t>(arguments.GetArgDWord(1));
        if (system_id == -121)
            (void)ScriptMonsterVisibility::SetHide(AddressOf(payload.data()), flag, runtime);
        else
            (void)ScriptMonsterVisibility::SetVisible(AddressOf(payload.data()), flag, runtime);
        return;
    }
    case -76: {
        const std::string name(StringArgument(arguments, 0));
        (void)ScriptNpcVisibility::SetVisible(
            name.c_str(), arguments.GetArgDWord(1) != 0u,
            Require(services_.npc_visibility, system_id));
        return;
    }
    case -120:
        (void)ScriptCameraDistanceOptimization::Set(
            arguments.GetArgDWord(0),
            Require(services_.camera_distance, system_id));
        return;
    case -221:
        ScriptMinimapLevel::Set(arguments.GetArgDWord(0), Require(services_.minimap_level, system_id));
        return;
    case -222:
        (void)ScriptPetVisibility::SetEnabled(
            static_cast<std::uint8_t>(arguments.GetArgDWord(0)),
            Require(services_.pet_visibility, system_id));
        return;
    case -86:
        Require(services_.weather, system_id).OpenWeather(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)));
        return;
    case -87:
        Require(services_.weather, system_id).CloseWeather();
        return;
    case -81:
        (void)ScriptCameraModeHandler::SetMode(
            arguments.GetArgDWord(0), arguments.GetArgDWord(1),
            Require(services_.camera_mode, system_id));
        return;
    case -257:
        (void)Require(services_.player_walk, system_id).WalkTo(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            {arguments.GetArgFloat(1), arguments.GetArgFloat(2), arguments.GetArgFloat(3)},
            static_cast<std::uint8_t>(arguments.GetArgDWord(4)));
        return;
    case -258:
        (void)Require(services_.player_walk, system_id).RunTo(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            {arguments.GetArgFloat(1), arguments.GetArgFloat(2),
             arguments.GetArgFloat(3)},
            static_cast<std::uint8_t>(arguments.GetArgDWord(4)));
        return;
    case -259:
        (void)Require(services_.player_walk, system_id).WalkCurrent(
            {arguments.GetArgFloat(0), arguments.GetArgFloat(1), arguments.GetArgFloat(2)},
            static_cast<std::uint8_t>(arguments.GetArgDWord(3)));
        return;
    case -260:
        (void)Require(services_.player_walk, system_id).WalkBack(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            {arguments.GetArgFloat(1), arguments.GetArgFloat(2), arguments.GetArgFloat(3)},
            static_cast<std::uint8_t>(arguments.GetArgDWord(4)));
        return;
    case -302:
    case -303: {
        ScriptObjectBlend blend(Require(services_.object_blend, system_id));
        const auto name = StringArgument(arguments, 0);
        const auto duration = arguments.GetArgFloat(1);
        const auto wait = static_cast<std::uint8_t>(arguments.GetArgDWord(2));
        if (system_id == -302) (void)blend.BlendOut(name, duration, wait);
        else (void)blend.BlendIn(name, duration, wait);
        return;
    }
    case -313: {
        ScriptCurrentPlayerTimedMove move(Require(services_.current_player_move, system_id));
        (void)move.Create(arguments.GetArgFloat(0),
            {arguments.GetArgFloat(1), arguments.GetArgFloat(2), arguments.GetArgFloat(3)},
            static_cast<std::uint8_t>(arguments.GetArgDWord(4)));
        return;
    }
    case -311: {
        ScriptObjectTimedScale scale(Require(services_.object_scale, system_id));
        (void)scale.Create(StringArgument(arguments,0),arguments.GetArgFloat(1),
            arguments.GetArgFloat(2),static_cast<std::uint8_t>(arguments.GetArgDWord(3)));
        return;
    }
    case -176: {
        GobPositionArguments position(Require(services_.object_move,system_id),StringArgument(arguments,0));
        (void)ScriptGobPosition::Set(0u,arguments.GetArgFloat(1),arguments.GetArgFloat(2),
            arguments.GetArgFloat(3),position);
        return;
    }
    case -308: {
        ScriptObjectTimedMove move(Require(services_.object_move, system_id));
        (void)move.Create(StringArgument(arguments, 0), arguments.GetArgFloat(1),
            {arguments.GetArgFloat(2), arguments.GetArgFloat(3), arguments.GetArgFloat(4)},
            static_cast<std::uint8_t>(arguments.GetArgDWord(5)));
        return;
    }
    case -267:
        (void)Require(services_.player_walk, system_id).EndMoveIndexed(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)));
        return;
    case -268:
        (void)Require(services_.player_walk, system_id).EndMoveCurrent();
        return;
    case -269:
    case -270:
    case -271: {
        auto& walk = Require(services_.player_walk, system_id);
        const auto name = StringArgument(arguments, 0);
        const PlayerWalkPoint target{arguments.GetArgFloat(1),
                                     arguments.GetArgFloat(2),
                                     arguments.GetArgFloat(3)};
        const auto wait = static_cast<std::uint8_t>(arguments.GetArgDWord(4));
        if (system_id == -269) (void)walk.WalkNpc(name, target, wait);
        else if (system_id == -270) (void)walk.RunNpc(name, target, wait);
        else (void)walk.BackNpc(name, target, wait);
        return;
    }
    case -285:
        (void)Require(services_.player_walk, system_id).EndMoveNpc(
            StringArgument(arguments, 0));
        return;
    case -272: {
        ScriptPlayerAction action(
            Require(services_.player_action, system_id));
        (void)action.StartNpc(
            StringArgument(arguments, 0), StringArgument(arguments, 1),
            static_cast<std::int32_t>(arguments.GetArgDWord(2)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(3)));
        return;
    }
    case -214:
        (void)ScriptShadowControl::Enable(static_cast<std::uint8_t>(arguments.GetArgDWord(0)),
            Require(services_.shadow, system_id));
        return;
    case -188:
    case -189: {
        const std::string name(StringArgument(arguments, 0));
        const std::array<std::uint32_t, 2> npc{0u, AddressOf(name.c_str())};
        auto& material = Require(services_.npc_material, system_id);
        if (system_id == -189) (void)ScriptNpcMaterial::Apply(AddressOf(npc.data()), material);
        else {
            const std::string texture(StringArgument(arguments, 1));
            const std::array<std::uint32_t, 2> face{0u, AddressOf(texture.c_str())};
            (void)ScriptNpcMaterial::Set(AddressOf(npc.data()), AddressOf(face.data()), material);
        }
        return;
    }
    case -134:
        (void)ScriptSound2d::StopCurrent(Require(services_.sound_2d, system_id));
        return;
    case -135:
        (void)ScriptSound2d::StopId(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            Require(services_.sound_2d, system_id));
        return;
    case -204:
        (void)ScriptForbiddenSkill::Set(
            std::bit_cast<std::int32_t>(arguments.GetArgDWord(0)),
            std::bit_cast<std::int32_t>(arguments.GetArgDWord(1)),
            arguments.GetArgDWord(2), Require(services_.forbidden_skill, system_id));
        return;
    case -145:
        (void)ScriptPlayerSkillAcquisition::Add(
            std::bit_cast<std::int32_t>(arguments.GetArgDWord(0)),
            arguments.GetArgDWord(1),
            static_cast<std::uint8_t>(arguments.GetArgDWord(2)),
            Require(services_.player_skill_acquisition, system_id));
        return;
    case -273: {
        ScriptPlayerAction action(
            Require(services_.player_action, system_id));
        (void)action.EndNpc(StringArgument(arguments, 0));
        return;
    }
    case -291:
        (void)Require(services_.flash, system_id).FlashOutBlack(
            arguments.GetArgFloat(0),
            static_cast<std::uint8_t>(arguments.GetArgDWord(1)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(2)));
        return;
    case -292:
        (void)Require(services_.flash, system_id).FlashInBlack(
            arguments.GetArgFloat(0),
            static_cast<std::uint8_t>(arguments.GetArgDWord(1)));
        return;
    case -293:
        (void)Require(services_.flash, system_id).FlashOutWhite(
            arguments.GetArgFloat(0),static_cast<std::uint8_t>(arguments.GetArgDWord(1)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(2)));
        return;
    case -294:
        (void)Require(services_.flash, system_id).FlashInWhite(
            arguments.GetArgFloat(0),static_cast<std::uint8_t>(arguments.GetArgDWord(1)));
        return;
    case -237:
        (void)ScriptMusicTransport::Pause(Require(services_.music, system_id));
        return;
    case -238:
        (void)ScriptMusicTransport::Resume(Require(services_.music, system_id));
        return;
    case -127: {
        auto& player = Require(services_.background_music_player, system_id);
        auto& runtime = Require(services_.background_music_runtime, system_id);
        (void)player.PlayScriptTrack(
            std::string(StringArgument(arguments, 0)), arguments.GetArgDWord(1),
            arguments.GetArgFloat(2),
            std::bit_cast<std::uint32_t>(arguments.GetArgFloat(3)), runtime);
        return;
    }
    case -128: {
        auto& player = Require(services_.background_music_player, system_id);
        auto& runtime = Require(services_.background_music_runtime, system_id);
        (void)player.ApplyScriptStopMode(arguments.GetArgDWord(0),
            arguments.GetArgFloat(1), runtime);
        return;
    }
    case -125: {
        BgmAreaQuery query(StringArgument(arguments, 0),
            Require(services_.background_music_area, system_id).SceneName());
        (void)arguments.SetReturnDWord(ScriptBgmCallbacks::IsInArea(0u, query) ? 1u : 0u);
        return;
    }
    case -124: {
        auto& player = Require(services_.background_music_player, system_id);
        auto& runtime = Require(services_.background_music_runtime, system_id);
        (void)player.AssignSavedTrack(
            std::string(StringArgument(arguments, 0)), runtime);
        return;
    }
    case -199:
        (void)ScriptPlayerRest::RestoreParty(Require(services_.player_rest, system_id));
        return;
    case -200:
        (void)arguments.SetReturnDWord(
            ScriptNightTimeQuery::IsNightTime(
                Require(services_.night_time, system_id)) ? 1u : 0u);
        return;
    case -63:
        (void)arguments.SetReturnFloat(static_cast<float>(ScriptCameraHandler::Pitch(
            Require(services_.camera, system_id))));
        return;
    case -103:
        ScriptCameraCallbacks::SetCollision(
            static_cast<std::uint8_t>(arguments.GetArgDWord(0)),
            Require(services_.camera, system_id));
        return;
    case -104:
        (void)ScriptCameraCallbacks::SeekToPlayer(
            Require(services_.camera, system_id));
        return;
    case -105:
        (void)ScriptCameraCallbacks::AutoSeek(
            arguments.GetArgDWord(0), Require(services_.camera, system_id));
        return;
    case -197:
        (void)ScriptInnDialog::Show(arguments.GetArgDWord(0),Require(services_.inn,system_id));return;
    case -198:
        (void)arguments.SetReturnDWord(static_cast<std::uint32_t>(ScriptInnDialog::Result(Require(services_.inn,system_id))));return;
    case -144: {
        const std::string text(StringArgument(arguments, 0));
        const std::array<std::uint32_t,2> payload{0u, AddressOf(text.c_str())};
        ScriptHintDisplay::Show(AddressOf(payload.data()), arguments.GetArgFloat(1),
            arguments.GetArgFloat(2), Require(services_.hint_display, system_id));
        return;
    }
    case -156:
        (void)arguments.SetReturnDWord(ScriptInventoryQuery::HasAtLeast(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            static_cast<std::int32_t>(arguments.GetArgDWord(1)),
            Require(services_.inventory_query, system_id)) ? 1u : 0u);
        return;
    case -190:
        (void)arguments.SetReturnDWord(static_cast<std::uint32_t>(ScriptInventoryQuery::Count(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            Require(services_.inventory_query, system_id))));
        return;
    case -205:
        (void)arguments.SetReturnDWord(static_cast<std::uint32_t>(ScriptInventoryQuery::Money(Require(services_.inventory_query,system_id))));return;
    case -207: {
        const std::string text(StringArgument(arguments,0));
        (void)ScriptQuestDialog::Show(text.c_str(),Require(services_.quest,system_id));return;
    }
    case -208:
        (void)arguments.SetReturnDWord(static_cast<std::uint32_t>(ScriptQuestDialog::Result(Require(services_.quest,system_id))));return;
    case -131:
        (void)arguments.SetReturnDWord(ScriptPuzzleResult::Get(
            Require(services_.puzzle_result, system_id)));
        return;
    case -300:
    case -301: {
        ScriptPuzzleGame puzzle(Require(services_.puzzle, system_id));
        const auto type = static_cast<std::int32_t>(arguments.GetArgDWord(0));
        if (system_id == -300) (void)puzzle.StartPuzzle(type);
        else (void)puzzle.StartJigsaw(type);
        return;
    }
    case -298:
        (void)ScriptObjectAction(Require(services_.object_action,system_id)).Create(
            StringArgument(arguments,0),StringArgument(arguments,1),
            static_cast<std::int32_t>(arguments.GetArgDWord(2)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(3)));
        return;
    case -299: {
        ScriptTradeWait trade(Require(services_.trade,system_id));
        (void)trade.Start(StringArgument(arguments,0),StringArgument(arguments,1));
        return;
    }
    case -229:
        (void)arguments.SetReturnDWord(ScriptCommonDialogSelection::IsOne(Require(services_.common_selection,system_id)));return;
    case -162:
        Require(services_.select_dialog,system_id).AddItem(StringArgument(arguments,0));return;
    case -163:
        (void)arguments.SetReturnDWord(static_cast<std::uint32_t>(Require(services_.select_dialog,system_id).LastSelection()));return;
    case -206:
        Require(services_.select_dialog,system_id).SetDefaultSelection(static_cast<std::int32_t>(arguments.GetArgDWord(0)));return;
    case -307:
        (void)ScriptDialogs(Require(services_.select_dialog,system_id)).ShowSelect(StringArgument(arguments,0));return;
    case -306:
    case -317: {
        ScriptDialogs dialogs(Require(services_.dialogs,system_id));
        const auto first=static_cast<std::int32_t>(arguments.GetArgDWord(1));
        const auto second=static_cast<std::int32_t>(arguments.GetArgDWord(2));
        if(system_id==-317)(void)dialogs.ShowCommonInSelectMode(StringArgument(arguments,0),first,second);
        else (void)dialogs.ShowCommon(StringArgument(arguments,0),first,second);
        return;
    }
    case -309: {
        ScriptTutorialWait tutorial(Require(services_.tutorial, system_id));
        (void)tutorial.Show(arguments.GetArgDWord(0));
        return;
    }
    case -287: {
        ScriptCameraSequences sequences(
            Require(services_.camera_sequences, system_id));
        (void)arguments.SetReturnDWord(
            sequences.Prepare(StringArgument(arguments, 0)) ? 1u : 0u);
        return;
    }
    case -288: {
        ScriptCameraSequences sequences(
            Require(services_.camera_sequences, system_id));
        (void)arguments.SetReturnDWord(
            sequences.RunSingle(
                StringArgument(arguments, 0),
                static_cast<std::uint8_t>(arguments.GetArgDWord(1))) ? 1u : 0u);
        return;
    }
    case -289: {
        ScriptCameraSequences sequences(
            Require(services_.camera_sequences, system_id));
        (void)arguments.SetReturnDWord(sequences.RunCircle(
            static_cast<std::uint8_t>(arguments.GetArgDWord(0))) ? 1u : 0u);
        return;
    }
    case -290: {
        ScriptCameraSequences sequences(
            Require(services_.camera_sequences, system_id));
        (void)sequences.CreateWaitHandler();
        return;
    }
    case -133: {
        const auto source = static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(arguments.GetArgObject(0)));
        (void)arguments.SetReturnDWord(static_cast<std::uint32_t>(
            ScriptSound2d::Play(
                source, static_cast<std::int32_t>(arguments.GetArgDWord(1)),
                Require(services_.sound_2d, system_id))));
        return;
    }
    case -132:
        (void)ScriptAlwaysJump::Set(arguments.GetArgDWord(0),Require(services_.always_jump,system_id));return;
    case -239: {
        ScriptWaitHandler wait(Require(services_.wait, system_id));
        (void)wait.Create(arguments.GetArgDouble(0));
        return;
    }
    case -240: {
        ScriptTalkHandler talk(Require(services_.talk, system_id));
        (void)talk.Invoke(StringArgument(arguments, 0),
                          StringArgument(arguments, 1));
        return;
    }
    case -244: {
        ScriptTalkHandler talk(Require(services_.talk, system_id));
        (void)talk.Wait();
        return;
    }
    case -203:
        Require(services_.ui_countdown,system_id).Start(arguments.GetArgDWord(0),StringArgument(arguments,1));
        return;
    case -232:
        Require(services_.ui_countdown,system_id).Clear();return;
    case -233:
        (void)Require(services_.ui_countdown,system_id).Pause();return;
    case -234:
        Require(services_.ui_countdown,system_id).Resume();return;
    case -236:
        (void)arguments.SetReturnDWord(ScriptUiTimerSaveData::Get(
            Require(services_.ui_timer_save_data,system_id)));return;
    case -123: {
        const std::string name(StringArgument(arguments, 0));
        const std::array<std::uint32_t, 2> payload{
            0u, static_cast<std::uint32_t>(
                    reinterpret_cast<std::uintptr_t>(name.c_str()))};
        (void)ScriptScenarioPortrait::Set(
            static_cast<std::uint32_t>(
                reinterpret_cast<std::uintptr_t>(payload.data())),
            static_cast<std::int32_t>(arguments.GetArgDWord(1)),
            Require(services_.portrait, system_id));
        return;
    }
    case -119: {
        const std::string name(StringArgument(arguments,0));
        const std::array<std::uint32_t,2> payload{0u,AddressOf(name.c_str())};
        (void)ScriptActionHoldRelease::Npc(AddressOf(payload.data()),Require(services_.action_hold,system_id));return;
    }
    case -118:
        (void)ScriptActionHoldRelease::Player(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            Require(services_.action_hold, system_id));
        return;
    case -113:
        (void)ScriptCurrentPlayerVisibility::Set(
            static_cast<std::uint8_t>(arguments.GetArgDWord(0)),
            Require(services_.current_player_visibility, system_id));
        return;
    case -71:
        (void)ScriptPlayerActions::SetVisible(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            arguments.GetArgDWord(1) != 0u,
            Require(services_.player_actions, system_id));
        return;
    case -70:
        (void)ScriptPlayerActions::SetLeader(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            Require(services_.player_actions, system_id));
        return;
    case -245: {
        ScriptPlayerAction action(
            Require(services_.player_action, system_id));
        (void)action.StartIndexed(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            StringArgument(arguments, 1),
            static_cast<std::int32_t>(arguments.GetArgDWord(2)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(3)));
        return;
    }
    case -246: {
        ScriptPlayerAction action(
            Require(services_.player_action, system_id));
        (void)action.EndIndexed(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)));
        return;
    }
    case -247: {
        ScriptPlayerAction action(Require(services_.player_action, system_id));
        (void)action.StartCurrent(StringArgument(arguments, 0),
            static_cast<std::int32_t>(arguments.GetArgDWord(1)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(2)));
        return;
    }
    case -248: {
        ScriptPlayerAction action(Require(services_.player_action, system_id));
        (void)action.EndCurrent();
        return;
    }
    case -114:
        (void)ScriptPartyRecovery::RestoreHp(Require(services_.party_recovery,system_id));
        return;
    case -115:
        (void)ScriptPartyRecovery::RestoreMp(Require(services_.party_recovery,system_id));
        return;
    case -96: {
        const std::string world(StringArgument(arguments, 0));
        (void)ScriptCombatStarter::Start(world.c_str(), Require(services_.combat_start, system_id));
        return;
    }
    case -219:
        (void)ScriptPlayerFavor::Add(
            std::bit_cast<std::int32_t>(arguments.GetArgDWord(0)),
            std::bit_cast<std::int32_t>(arguments.GetArgDWord(1)),
            std::bit_cast<std::int32_t>(arguments.GetArgDWord(2)),
            Require(services_.player_favor,system_id));
        return;
    case -94:
    case -95:
    case -218:
        Require(services_.combat_text, system_id).SetText(
            system_id == -94 ? CombatConfigTextSlot::bgm :
            system_id == -95 ? CombatConfigTextSlot::camera : CombatConfigTextSlot::ground_camera,
            StringArgument(arguments, 0));
        return;
    case -93:
        (void)ScriptCombatConfig::Configure(arguments.GetArgDWord(0), arguments.GetArgDWord(1),
            arguments.GetArgDWord(2), arguments.GetArgDWord(3), Require(services_.combat_config, system_id));
        return;
    case -108:
        (void)ScriptPlayerStatus::SetLevel(static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            static_cast<std::int32_t>(arguments.GetArgDWord(1)),
            Require(services_.player_status, system_id));
        return;
    case -107:
        arguments.SetReturnDWord(static_cast<asDWORD>(ScriptPlayerStatus::GetLeaderState(
            Require(services_.player_status, system_id))));
        return;
    case -261:
    case -262: {
        ScriptPlayerBlend blend(Require(services_.player_blend, system_id));
        const auto index = static_cast<std::int32_t>(arguments.GetArgDWord(0));
        const auto duration = arguments.GetArgFloat(1);
        const auto mode = static_cast<std::uint8_t>(arguments.GetArgDWord(2));
        if (system_id == -261) (void)blend.BlendOut(index, duration, mode);
        else (void)blend.BlendIn(index, duration, mode);
        return;
    }
    case -100:
        (void)ScriptPartyMembership::Add(arguments.GetArgDWord(0),
            static_cast<std::uint8_t>(arguments.GetArgDWord(1)),
            Require(services_.party_membership, system_id));
        return;
    case -101:
        (void)ScriptPartyMembership::Remove(arguments.GetArgDWord(0),
            static_cast<std::uint8_t>(arguments.GetArgDWord(1)),
            Require(services_.party_membership, system_id));
        return;
    case -213:
        (void)ScriptPrescriptionAddition::Add(static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(1)),Require(services_.prescription_addition,system_id));
        return;
    case -98:
        (void)ScriptInventoryChange::Add(arguments.GetArgDWord(0),
            static_cast<std::int32_t>(arguments.GetArgDWord(1)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(2)),
            Require(services_.inventory_change, system_id));
        return;
    case -178:
        (void)ScriptMoneyTransaction::Add(std::bit_cast<std::int32_t>(arguments.GetArgDWord(0)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(1)),
            Require(services_.money_transaction, system_id));
        return;
    case -179:
        (void)ScriptMoneyTransaction::Pay(std::bit_cast<std::int32_t>(arguments.GetArgDWord(0)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(1)),
            Require(services_.money_transaction, system_id));
        return;
    case -99:
        (void)ScriptInventoryChange::Remove(arguments.GetArgDWord(0),
            static_cast<std::int32_t>(arguments.GetArgDWord(1)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(2)),
            Require(services_.inventory_change, system_id));
        return;
    case -220:
        (void)arguments.SetReturnDWord(ScriptPalTestResult::Get(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            Require(services_.pal_test, system_id)));
        return;
    case -180:
        (void)ScriptArenaSkillState::HideSkillObject(Require(services_.arena_skill_state, system_id));
        return;
    case -181:
        (void)ScriptArenaSkillState::ShowSignpost(Require(services_.arena_skill_state, system_id));
        return;
    case -109:
        (void)ScriptPlayerEquipment::Add(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            arguments.GetArgDWord(1),
            Require(services_.player_equipment, system_id));
        return;
    case -74:
        (void)ScriptPlayerLockHandler::Lock(
            Require(services_.player_lock, system_id));
        return;
    case -75:
        ScriptPlayerLockHandler::Unlock(
            Require(services_.player_lock, system_id));
        return;
    case -84:
        (void)ScriptNpcBehavior::Pause(std::string(StringArgument(arguments,0)).c_str(),Require(services_.npc_behavior,system_id));return;
    case -85:
        ScriptNpcBehavior::Resume(std::string(StringArgument(arguments,0)).c_str(),Require(services_.npc_behavior,system_id));return;
    case -264: {
        ScriptPlayerTurn turn(Require(services_.player_turn,system_id));
        (void)turn.FaceCurrentToNpc(StringArgument(arguments,0),static_cast<std::uint8_t>(arguments.GetArgDWord(1)));return;
    }
    case -304:
        (void)ScriptMonsterBlend(Require(services_.monster_blend,system_id)).BlendOut(
            StringArgument(arguments,0),arguments.GetArgFloat(1),static_cast<std::uint8_t>(arguments.GetArgDWord(2)));return;
    case -305:
        (void)ScriptMonsterBlend(Require(services_.monster_blend,system_id)).BlendIn(
            StringArgument(arguments,0),arguments.GetArgFloat(1),static_cast<std::uint8_t>(arguments.GetArgDWord(2)));return;
    case -279:
        (void)ScriptPlayerBlend(Require(services_.npc_blend,system_id)).BlendNpcOut(
            StringArgument(arguments,0),arguments.GetArgFloat(1),
            static_cast<std::uint8_t>(arguments.GetArgDWord(2)));
        return;
    case -280:
        (void)ScriptPlayerBlend(Require(services_.npc_blend,system_id)).BlendNpcIn(
            StringArgument(arguments,0),arguments.GetArgFloat(1),
            static_cast<std::uint8_t>(arguments.GetArgDWord(2)));
        return;
    case -281: {
        ScriptPlayerTurn turn(Require(services_.player_turn,system_id));
        (void)turn.FaceNpcToCurrent(StringArgument(arguments,0),static_cast<std::uint8_t>(arguments.GetArgDWord(1)));return;
    }
    case -282: {
        ScriptPlayerTurn turn(Require(services_.player_turn,system_id));
        (void)turn.ResetNpcDirection(StringArgument(arguments,0));return;
    }
    case -276: {
        ScriptPlayerTurn turn(Require(services_.player_turn,system_id));
        (void)turn.StartNpc(StringArgument(arguments,0),arguments.GetArgFloat(1),static_cast<std::uint8_t>(arguments.GetArgDWord(2)));
        return;
    }
    case -277: {
        ScriptPlayerTurn turn(Require(services_.player_turn, system_id));
        (void)turn.FaceNpcToNpc(StringArgument(arguments, 0),
                              StringArgument(arguments, 1),
                              static_cast<std::uint8_t>(arguments.GetArgDWord(2)));
        return;
    }
    case -278: {
        ScriptPlayerTurn turn(Require(services_.player_turn, system_id));
        (void)turn.FaceNpcToPlayer(StringArgument(arguments, 0),
            static_cast<std::int32_t>(arguments.GetArgDWord(1)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(2)));
        return;
    }
    case -274: {
        ScriptPlayerPosition position(Require(services_.player_position,system_id), services_.completed_handlers);
        (void)position.SetNpc(StringArgument(arguments,0),{arguments.GetArgFloat(1),arguments.GetArgFloat(2),arguments.GetArgFloat(3)});
        return;
    }
    case -249: {
        ScriptPlayerPosition position(
            Require(services_.player_position, system_id), services_.completed_handlers);
        (void)position.Set(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            {arguments.GetArgFloat(1), arguments.GetArgFloat(2),
             arguments.GetArgFloat(3)});
        return;
    }
    case -250: {
        ScriptPlayerPosition position(
            Require(services_.player_position, system_id), services_.completed_handlers);
        (void)position.SetCurrent({arguments.GetArgFloat(0),
                                   arguments.GetArgFloat(1),
                                   arguments.GetArgFloat(2)});
        return;
    }
    case -192:
        (void)arguments.SetReturnFloat(
            Require(services_.player_position, system_id).CurrentY());
        return;
    case -193:
        (void)arguments.SetReturnFloat(
            Require(services_.player_position, system_id).CurrentZ());
        return;
    case -91: {
        auto& camera_mode = Require(services_.camera_mode, system_id);
        camera_mode.FlushMainCameraTailYaw(camera_mode.ActorControl());
        return;
    }
    case -201:
    case -202: {
        NativeScriptEntityTransformRuntime runtime(
            Require(services_.player_position,system_id),
            Require(services_.player_rotation,system_id),
            Require(services_.entity_transforms,system_id));
        if(system_id==-201)
            (void)ScriptEntityTransform::PlayerByIndex(
                static_cast<std::int32_t>(arguments.GetArgDWord(0)),
                arguments.GetArgFloat(1),arguments.GetArgFloat(2),
                arguments.GetArgFloat(3),arguments.GetArgFloat(4),runtime);
        else {
            const std::string name(StringArgument(arguments,0));
            (void)ScriptEntityTransform::NpcByName(name.c_str(),
                arguments.GetArgFloat(1),arguments.GetArgFloat(2),
                arguments.GetArgFloat(3),arguments.GetArgFloat(4),runtime);
        }
        return;
    }
    case -252: {
        ScriptPlayerRotation rotation(Require(services_.player_rotation, system_id), services_.completed_handlers);
        (void)rotation.SetAngle(static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            arguments.GetArgFloat(1));
        return;
    }
    case -253: {
        ScriptPlayerRotation rotation(
            Require(services_.player_rotation, system_id), services_.completed_handlers);
        (void)rotation.SetCurrentAngle(arguments.GetArgFloat(0));
        return;
    }
    case -286: {
        ScriptPlayerRotation rotation(
            Require(services_.player_rotation, system_id), services_.completed_handlers);
        (void)rotation.SetNpcAngle(StringArgument(arguments, 0),
                                   arguments.GetArgFloat(1));
        return;
    }
    case -254: {
        ScriptPlayerTurn turn(Require(services_.player_turn, system_id));
        (void)turn.FacePlayer(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            static_cast<std::int32_t>(arguments.GetArgDWord(1)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(2)));
        return;
    }
    case -312: {
        ScriptPlayerTurn turn(Require(services_.player_turn, system_id));
        (void)turn.FaceCurrentToObject(StringArgument(arguments, 0),
            static_cast<std::uint8_t>(arguments.GetArgDWord(1)));
        return;
    }
    case -256: {
        ScriptPlayerTurn turn(Require(services_.player_turn, system_id));
        (void)turn.FaceNpc(static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            StringArgument(arguments, 1), static_cast<std::uint8_t>(arguments.GetArgDWord(2)));
        return;
    }
    case -255: {
        ScriptPlayerTurn turn(Require(services_.player_turn, system_id));
        (void)turn.Start(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            arguments.GetArgFloat(1),
            static_cast<std::uint8_t>(arguments.GetArgDWord(2)));
        return;
    }
    case -297:
        (void)Require(services_.movie, system_id).Play(
            StringArgument(arguments, 0));
        return;
    case -318:
        (void)Require(services_.movie, system_id).PlayFinal(
            StringArgument(arguments, 0));
        return;
    case -110:
        ScriptGlobalCallbacks::OpenMovieFlag(
            arguments.GetArgDWord(0), Require(services_.globals, system_id));
        return;
    case -58:
        Require(services_.camera, system_id).AdjustYpr(
            arguments.GetArgFloat(0), arguments.GetArgFloat(1),
            arguments.GetArgFloat(2), static_cast<std::uint8_t>(arguments.GetArgDWord(3)));
        return;
    case -57:
        (void)ScriptNewGameHandler::Run(Require(services_.new_game, system_id));
        return;
    case -65: {
        auto& arena = Require(services_.arena, system_id);
        const std::string first(StringArgument(arguments, 0));
        const std::string second(StringArgument(arguments, 1));
        const std::string third(StringArgument(arguments, 2));
        (void)GameArenaLoadHandler::Load(
            {first.c_str()}, {second.c_str()}, {third.c_str()}, arena);
        return;
    }
    case -66:
        (void)GameArenaReadyHandler::Run(
            Require(services_.arena_ready, system_id));
        return;
    case -67:
        (void)GameArenaReadyHandler::Restore(
            Require(services_.arena_ready, system_id));
        return;
    case -164: {
        PlayerHandArguments bridge(Require(services_.player_gob_attachment,system_id),
            StringArgument(arguments,0),StringArgument(arguments,1));
        (void)ScriptPlayerGobAttachment::Attach(16u,32u,
            static_cast<std::int32_t>(arguments.GetArgDWord(2)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(3)),bridge);
        return;
    }
    case -166:
        (void)ScriptPlayerGobAttachment::Detach(static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            Require(services_.player_gob_attachment,system_id));
        return;
    case -168: {
        PlayerHandArguments bridge(Require(services_.player_gob_attachment,system_id),
            StringArgument(arguments,1), {});
        (void)ScriptPlayerGobAttachment::AttachEffect(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)), 16u,
            static_cast<std::uint8_t>(arguments.GetArgDWord(2)), bridge);
        return;
    }
    case -169: {
        PlayerHandArguments bridge(Require(services_.player_gob_attachment,system_id),
            StringArgument(arguments,0), {});
        (void)ScriptPlayerGobAttachment::AttachEffectToCurrent(16u,
            static_cast<std::uint8_t>(arguments.GetArgDWord(1)), bridge);
        return;
    }
    case -170:
        (void)ScriptPlayerGobAttachment::DetachEffect(
            static_cast<std::int32_t>(arguments.GetArgDWord(0)),
            Require(services_.player_gob_attachment,system_id));
        return;
    case -171:
        (void)ScriptPlayerGobAttachment::DetachEffectFromCurrent(
            Require(services_.player_gob_attachment,system_id));
        return;
    case -172: {
        NpcHandArguments bridge(Require(services_.npc_hand_effect,system_id),
            StringArgument(arguments,0),StringArgument(arguments,1));
        (void)ScriptNpcHandEffect::Attach(16u,32u,static_cast<std::uint8_t>(arguments.GetArgDWord(2)),bridge);
        return;
    }
    case -173:
    case -175: {
        NpcHandArguments bridge(Require(services_.npc_hand_effect,system_id),StringArgument(arguments,0));
        if(system_id == -173) (void)ScriptNpcHandEffect::Detach(16u,bridge);
        else (void)ScriptNpcHandEffect::DetachGob(16u,bridge);
        return;
    }
    case -174: {
        NpcHandArguments bridge(Require(services_.npc_hand_effect,system_id),
            StringArgument(arguments,0),StringArgument(arguments,1),StringArgument(arguments,2));
        (void)ScriptNpcHandEffect::AttachGob(16u,32u,48u,static_cast<std::uint8_t>(arguments.GetArgDWord(3)),bridge);
        return;
    }
    case -161:
        (void)arguments.SetReturnDWord(ScriptPartyMagicQuery::AnyMastered(Require(services_.party_magic_query,system_id))?1u:0u);return;
    case -158:
        ScriptSystemUi::Open(arguments.GetArgDWord(0),
                             Require(services_.system_ui, system_id));
        return;
    case -159:
        ScriptSystemUi::GrantSmith(
            static_cast<std::uint8_t>(arguments.GetArgDWord(0)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(1)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(2)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(3)),
            Require(services_.system_ui, system_id));
        return;
    case -160:
        ScriptSystemUi::GrantMagic(
            static_cast<std::uint8_t>(arguments.GetArgDWord(0)),
            static_cast<std::uint8_t>(arguments.GetArgDWord(1)),
            Require(services_.system_ui, system_id));
        return;
    case -157:
        ScriptSystemUi::Grant(
            arguments.GetArgDWord(0),
            static_cast<std::uint8_t>(arguments.GetArgDWord(1)),
            Require(services_.system_ui, system_id));
        return;
    case -310:
        (void)ScriptWorldMapWait(Require(services_.world_map, system_id)).Show();
        return;
    case -130: {
        const std::string name(StringArgument(arguments, 0));
        const std::array<std::uint32_t, 2> payload{0u, AddressOf(name.c_str())};
        (void)WorldMapButtons::DispatchExternalVisibility(AddressOf(payload.data()),
            arguments.GetArgDWord(1), Require(services_.world_map_external, system_id),
            Require(services_.world_map_buttons, system_id));
        return;
    }
    case -314:
        (void)ScriptPoetryWait(Require(services_.poetry, system_id)).Show(
            arguments.GetArgDWord(0), static_cast<std::uint8_t>(arguments.GetArgDWord(1)));
        return;
    default:
        throw std::runtime_error("PAL4 game script callback " +
                                 std::to_string(system_id) +
                                 " is not implemented");
    }
}

}  // namespace pal4
