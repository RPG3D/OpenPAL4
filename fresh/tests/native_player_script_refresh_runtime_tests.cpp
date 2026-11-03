#include "pal4/native_skill_system_owner.h"
#include "pal4/native_player_combat_events.h"
#include "pal4/native_player_script_refresh_runtime.h"
#include "pal4/player_manager_slot.h"
#include "pal4/native_player_skill_callbacks.h"
#include "pal4/native_skill_record_catalog.h"
#include "pal4/native_game_database.h"
#include "pal4/script_skill_collection.h"
#include "pal4/native_player_skill_runtime.h"
#include "pal4/random_number_generator.h"
#include "pal4/native_player_field_schema.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Address(const void* pointer) {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(pointer));
}

pal4::NativeGameDbTable EmptySkillTable() {
    pal4::NativeGameDbTable table;
    for (const auto name : {"ID", "TYPE", "TRIGER", "EXIST_TIME", "FUNCTION",
                           "ATTACHED_SKILL", "MUTEX_ID", "STATUS_ID"})
        table.fields.push_back({name, 1u});
    for (unsigned i = 1u; i <= 9u; ++i)
        table.fields.push_back({"SKILL_PARAM_" + std::to_string(i), 3u});
    for (const auto name : {"EF", "BIND", "EF1", "BIND1"})
        table.fields.push_back({name, 2u});
    return table;
}

struct Cleanup final : pal4::IScriptSkillListCleanupRuntime {
    explicit Cleanup(pal4::NativePlayerSkillCallbacks& callbacks) : callbacks(callbacks) {}
    unsigned ReadWord(unsigned address) override { return *reinterpret_cast<const unsigned*>(address); }
    void WriteWord(unsigned address, unsigned value) override { *reinterpret_cast<unsigned*>(address) = value; }
    unsigned ResolveSkillRecord(unsigned record) override { ++resolved; return callbacks.ResolveSkillRecord(record); }
    unsigned InvokeVirtual(unsigned object, unsigned offset, std::span<const unsigned> arguments) override {
        ++notifications;
        Check(offset == 304u && arguments.size() == 1u && arguments[0] != 0u,
              "cleanup resolves fallback before original removal notification");
        return callbacks.InvokeVirtual(object, offset, arguments);
    }
    pal4::NativePlayerSkillCallbacks& callbacks;
    unsigned resolved{}, notifications{};
};

void TestRefreshWithEmptyInventoryAndSkills() {
    std::array<std::uint32_t, pal4::PlayerManagerSlot::kObjectBytes / 4u>
        slot{};
    const auto script = Address(slot.data()) +
                        pal4::PlayerManagerSlot::kScriptOffset;
    auto write = [script](std::uint32_t offset, std::uint32_t value) {
        *reinterpret_cast<std::uint32_t*>(script + offset) = value;
    };
    auto read = [script](std::uint32_t offset) {
        return *reinterpret_cast<const std::uint32_t*>(script + offset);
    };
    write(4u, 0x12345678u);
    write(8u, 100u);
    write(16u, 20u);
    write(0x220u + 7u * 4u, 125u);
    write(0x220u + 8u * 4u, 120u);
    write(0x220u + 9u * 4u, 25u);
    pal4::NativeBaseObjectMemory memory;
    pal4::NativeInventorySession inventory;
    Check(inventory.Start() != 0u, "construct empty inventory");
    pal4::NativePlayerScriptRefreshRuntime refresh(memory, inventory);
    pal4::NativeSkillRecordCatalog catalog(EmptySkillTable());
    pal4::NativePlayerSkillCallbacks callbacks(catalog, refresh);
    write(0u, pal4::PlayerManagerSlot::kScriptVtable);
    Check(callbacks.InvokeVirtual(script, 276u, {}) == 20u &&
              read(0x154u) == 0x12345678u &&
              read(0x220u + 7u * 4u) == 100u &&
              read(0x220u + 8u * 4u) == 100u &&
              read(0x220u + 9u * 4u) == 20u &&
              read(0x288u) == 0u,
          "refresh copies stats, processes empty lists, clamps properties and updates health");
    Check(memory.OutstandingAllocations() == 0u,
          "empty skill batch does not allocate");
    Check(refresh.SetValue(script, 4u, 678u) == 678u &&
              read(0xe0u) == 678u && read(0x220u + 4u * 4u) == 678u,
          "refresh property setter preserves the original progression mirror write");

    // Original Player notification slots are void empty functions. They must
    // not reset attributes, dispatch effects or refresh stats on removal.
    const auto before = slot;
    for (const auto offset : {280u,284u,288u,292u,296u,300u,304u,308u,312u,316u,320u}) {
        const auto count = offset == 280u || offset == 284u ? 4u :
            offset == 296u || offset == 304u ? 1u : offset == 300u ? 2u : 3u;
        const std::array<unsigned,4> arguments{catalog.ResolveRecord(0u), 1u, script, script};
        (void)callbacks.InvokeVirtual(script, offset, std::span(arguments).first(count));
    }
    Check(slot == before, "reviewed player skill notifications leave all player data unchanged");
    bool rejected{};
    try { (void)callbacks.InvokeVirtual(script,304u,{}); } catch (const std::invalid_argument&) { rejected=true; }
    Check(rejected && slot == before, "bad notification arguments fail before mutation");
    for(const auto table:{pal4::PlayerManagerSlot::kScriptVtable,pal4::PlayerManagerSlot::kScriptCleanupVtable}) {
        write(0u,table);
        for(const auto offset:{140u,148u,156u,164u,180u,188u,196u,204u}) {
            (void)callbacks.InvokeVirtual(script,offset,std::array{1u});
            const auto saved=slot;
            rejected=false;
            try{(void)callbacks.InvokeVirtual(script,offset,{});}catch(const std::invalid_argument&){rejected=true;}
            Check(rejected && saved==slot,"status setter arity rejects before mutation");
            (void)callbacks.InvokeVirtual(script,offset,std::array{0u});
        }
        for(const auto index:{158u,159u,160u,161u,163u,164u,165u,166u})
            Check(read(index*4u)==0u,"both Player script tables bind real status reset setters");
    }
    write(0u,pal4::PlayerManagerSlot::kScriptVtable);
    write(0u, 0x12345678u);
    rejected=false;
    try { (void)callbacks.InvokeVirtual(script,304u,std::array{0u}); } catch (const std::invalid_argument&) { rejected=true; }
    Check(rejected, "unknown vtable cannot acquire empty player notification semantics");
    write(0u,pal4::PlayerManagerSlot::kScriptCleanupVtable);
    write(0x154u,0x87654321u);
    (void)callbacks.InvokeVirtual(script,276u,{});
    Check(read(0x154u) == 0x87654321u, "cleanup vtable refresh entry is the original empty function");

    std::array<unsigned,6> timed{0u,11u,12u,catalog.ResolveRecord(0u),21u,22u};
    std::array<unsigned,4> permanent{catalog.ResolveRecord(0u),31u,0u,41u};
    write(680u,Address(timed.data())); write(684u,Address(timed.data())+sizeof(timed));
    write(688u,read(684u));
    write(696u,Address(permanent.data())); write(700u,Address(permanent.data())+sizeof(permanent));
    write(704u,read(700u));
    Cleanup cleanup(callbacks);
    (void)pal4::ScriptSkillCollection::CleanupObjectLists(script,cleanup);
    Check(cleanup.resolved == 4u && cleanup.notifications == 4u &&
              read(680u) == read(684u) && read(696u) == read(700u) &&
              read(688u) == Address(timed.data())+sizeof(timed) &&
              read(704u) == Address(permanent.data())+sizeof(permanent) &&
              read(0x154u) == 0x87654321u,
          "nonempty native timed/permanent cleanup resolves null records and retains buffers without refreshing");
    inventory.Close();
    Check(inventory.OutstandingAllocations() == 0u,
          "refresh inventory releases native state");
}

void AddSkill(pal4::NativeGameDbTable& table, unsigned id, unsigned function,
              float value, float selector=0.0f, unsigned next=0u,
              unsigned trigger=1u, unsigned duration=0xffffffffu) {
    std::vector<pal4::NativeGameDbValue> row;
    for (const auto& field : table.fields) {
        if (field.type == 2u) row.emplace_back(std::string{});
        else if (field.type == 3u) row.emplace_back(field.name == "SKILL_PARAM_1" ? value :
            field.name == "SKILL_PARAM_2" ? selector : 0.0f);
        else row.emplace_back(field.name == "ID" ? id : field.name == "FUNCTION" ? function :
            field.name == "TRIGER" ? trigger : field.name == "EXIST_TIME" ? duration :
            field.name == "ATTACHED_SKILL" ? next : 0u);
    }
    table.rows.push_back(std::move(row));
}

void TestNativeCombatEvents() {
    pal4::NativePlayerCombatEvents events;
    for(unsigned index=1u;index<=77u;++index)
        Check(events.Lookup(index)!=0u,"all77 original event registrations have owned images");
    for(const auto index:{0u,79u,0xffffffffu})
        Check(events.Lookup(index)==0u,"unregistered and out-of-range event indices stay null");
    Check(events.Lookup(78u)==0xffffffffu,"index78 retains the original adjacent-data sentinel");
    std::array<unsigned,pal4::PlayerManagerSlot::kObjectBytes/4u> slot{};
    const auto script=Address(slot.data())+pal4::PlayerManagerSlot::kScriptOffset;
    const auto Write=[](unsigned address,unsigned value) { *reinterpret_cast<unsigned*>(address)=value; };
    const auto Read=[](unsigned address) { return *reinterpret_cast<const unsigned*>(address); };
    Write(script,pal4::PlayerManagerSlot::kScriptVtable);
    const auto basic=pal4::NativePlayerBasicFieldSchema::Build();
    const auto fields=pal4::NativePlayerFieldSchema::Build();
    const auto Offset=[](const auto& schema,const char* name,unsigned origin) {
        for(const auto& field:schema) if(field.name==name) return field.slot_offset-origin;
        throw std::runtime_error("missing event test field");
    };
    const auto physical=Offset(fields,"CA_PHYSICAL",0x654u);
    const auto basic_physical=Offset(basic,"CA_PHYSICAL",0x658u);
    Write(script+4u+basic_physical,40u);
    Write(script+0x154u+basic_physical,60u);
    pal4::NativePlayerCombatEvents::Record record{};
    events.Build(record,script,7u,0.5f,1u);
    Check(std::bit_cast<float>(record[1])==30.0f,"mode1 reads current BASIC and retains floating getter return");
    events.Build(record,script,7u,0.5f,3u);
    Check(std::bit_cast<float>(record[1])==20.0f,"mode3 reads distinct base BASIC");
    events.Process(record,script);
    Check(Read(script+physical)==80u,"integer event adds scaled base value to native current attribute");
    events.Build(record,script,7u,23.75f,2u);
    events.Process(record,script);
    Check(Read(script+physical)==23u,"mode2 sets integer attribute with original truncation");
    events.Build(record,script,7u,-2.5f,0u);
    Check(events.Matches(record,std::bit_cast<unsigned>(2.0f)) && std::bit_cast<float>(record[1])==-5.0f,
          "enabled event query mutates the record multiplier");
    events.Process(record,script);
    Check(Read(script+physical)==18u,"mode0 adds the queried signed value");
    const auto water=Offset(fields,"ERR_HUMAN_RACE_ATK",0x654u);
    events.Build(record,script,50u,0.25f,2u); events.Process(record,script);
    events.Build(record,script,50u,0.125f,0u); events.Process(record,script);
    Check(std::bit_cast<float>(Read(script+water))==0.375f,"float events preserve fractional set and increment");
    Write(script+0x158u,96u);
    events.Build(record,script,2u,0.25f,4u);
    Check(std::bit_cast<float>(record[1])==24.0f,"mode4 HP reads live resource word");
    Write(script+0x160u,80u);
    events.Build(record,script,6u,0.25f,4u);
    Check(std::bit_cast<float>(record[1])==20.0f,"mode4 MP reads separate live resource word");
    for(const auto index:{0u,79u}) {
        events.Build(record,script,index,1.0f,0u);
        Check(record==pal4::NativePlayerCombatEvents::Record{0u,0u,2u},"missing event yields original invalid record");
        events.Process(record,script);
    }
    for(const auto mode:{4u,5u,0xffffffffu}) {
        events.Build(record,script,7u,1.0f,mode);
        Check(record==pal4::NativePlayerCombatEvents::Record{0u,0u,2u},"invalid mode or mode4 nonresource rejects record");
    }
    events.Build(record,script,39u,3.0f,0u);
    Check(!events.Matches(record,std::bit_cast<unsigned>(2.0f)) && std::bit_cast<float>(record[1])==3.0f,
          "status event query is disabled and leaves scale unchanged");
    record={0x1234u,0u,0u};
    bool rejected{};
    try { events.Process(record,script); } catch(const std::invalid_argument&) { rejected=true; }
    Check(rejected,"foreign event cannot be invoked");
    events.Build(record,script,78u,1.0f,0u); rejected=false;
    try { events.Process(record,script); } catch(const std::invalid_argument&) { rejected=true; }
    Check(rejected,"original adjacent-data sentinel is preserved but cannot be called as an owned event");
    Write(script,0u); rejected=false;
    try { events.Build(record,script,7u,1.0f,0u); } catch(const std::invalid_argument&) { rejected=true; }
    Check(rejected,"non-Player target cannot acquire Player event semantics");
}

void TestNativeSkillEffectsAndRefresh() {
    auto table = EmptySkillTable();
    AddSkill(table,1u,9105u,0.25f,1.9f,2u);
    AddSkill(table,2u,9109u,0.75f);
    AddSkill(table,3u,9105u,0.5f,3.8f);
    AddSkill(table,4u,9112u,0.33f);
    AddSkill(table,5u,9113u,0.66f);
    AddSkill(table,6u,9999u,99.0f); // Missing callback stops chain in original.
    AddSkill(table,7u,9001u,1.0f); // Invalid event index exits without consuming RNG.
    AddSkill(table,8u,9109u,0.125f,0.0f,0u,2u);
    AddSkill(table,9u,9109u,0.875f,0.0f,0u,1u,10u);
    AddSkill(table,10u,9105u,99.0f,std::numeric_limits<float>::quiet_NaN());
    AddSkill(table,11u,9000u,1.0f,12.9f,0u,2u);
    AddSkill(table,12u,9109u,0.925f);
    AddSkill(table,13u,9112u,0.8f);
    AddSkill(table,14u,9000u,-1.0f,12.0f,0u,3u);
    AddSkill(table,15u,9100u,1.0f,1.0f,0u,4u);
    AddSkill(table,16u,9100u,-1.0f,1.0f,0u,5u);
    AddSkill(table,17u,9100u,1.0f,-1.0f,0u,6u);
    AddSkill(table,18u,9114u,50.9f,0.125f,0u,7u);
    AddSkill(table,19u,9114u,56.9f,-0.25f,0u,8u);
    AddSkill(table,20u,9114u,49.99f,99.0f,0u,9u);
    AddSkill(table,21u,9114u,std::numeric_limits<float>::quiet_NaN(),99.0f,0u,10u);
    AddSkill(table,22u,9132u,1.0f,0.0f,23u,11u);
    AddSkill(table,23u,9109u,0.625f);
    const std::array no_op_functions{9101u,9102u,9103u,9104u,9120u,9121u};
    for(unsigned i=0;i<no_op_functions.size();++i)
        AddSkill(table,24u+i,no_op_functions[i],99.0f,0.0f,23u,12u+i);
    AddSkill(table,30u,9001u,1.0f,39.9f);
    AddSkill(table,31u,9001u,1.0f,47.9f);
    AddSkill(table,32u,9001u,-1.0f,39.0f);
    for(auto& row:table.rows) {
        const auto id=std::get<unsigned>(row[0]);
        if(id<30u || id>32u)continue;
        for(std::size_t i=0;i<table.fields.size();++i) {
            if(table.fields[i].name=="SKILL_PARAM_3")row[i]=2.9f;
            if(table.fields[i].name=="SKILL_PARAM_4")row[i]=23.9f;
        }
    }
    for(const auto row:{14u,15u,16u})
        for(std::size_t i=0;i<table.fields.size();++i) {
            const auto& name=table.fields[i].name;
            if(name=="SKILL_PARAM_3") table.rows[row][i]=2.0f;
            if(name=="SKILL_PARAM_4") table.rows[row][i]=7.0f;
            if(name=="SKILL_PARAM_5") table.rows[row][i]=5.0f;
            if(name=="SKILL_PARAM_6") table.rows[row][i]=0.0f;
            if(name=="SKILL_PARAM_7") table.rows[row][i]=50.0f;
            if(name=="SKILL_PARAM_8") table.rows[row][i]=0.25f;
            if(name=="SKILL_PARAM_9") table.rows[row][i]=2.0f;
        }
    // A second nonzero output, a missing ID, zero slots and an invalid float
    // exercise the eight original outcome parameter conversions in order.
    for(std::size_t i=0;i<table.fields.size();++i) {
        if(table.fields[i].name=="SKILL_PARAM_3") table.rows[10][i]=13.5f;
        if(table.fields[i].name=="SKILL_PARAM_4") table.rows[10][i]=123456.0f;
        if(table.fields[i].name=="SKILL_PARAM_9") table.rows[10][i]=std::numeric_limits<float>::quiet_NaN();
    }
    pal4::NativeSkillRecordCatalog catalog(table);
    pal4::NativeBaseObjectMemory memory;
    pal4::NativeInventorySession inventory;
    Check(inventory.Start() != 0u,"skill effect fixture starts inventory");
    pal4::NativePlayerScriptRefreshRuntime refresh(memory,inventory);
    pal4::NativePlayerSkillCallbacks callbacks(catalog,refresh);
    pal4::RandomNumberGenerator random;
    pal4::NativePlayerCombatEvents property_events;
    pal4::NativeSkillSystemOwner system(random,property_events);
    (void)system.Construct("H:/PAL4/objs");
    pal4::NativePlayerSkillRuntime skills(memory,catalog,callbacks,system);
    refresh.BindSkills(&skills);
    std::array<unsigned,pal4::PlayerManagerSlot::kObjectBytes/4u> source{},other{};
    const auto script=Address(source.data())+pal4::PlayerManagerSlot::kScriptOffset;
    const auto target=Address(other.data())+pal4::PlayerManagerSlot::kScriptOffset;
    auto Write=[](unsigned address,unsigned value) { *reinterpret_cast<unsigned*>(address)=value; };
    auto Read=[](unsigned address) { return *reinterpret_cast<const unsigned*>(address); };
    auto Float=[&](unsigned object,unsigned index) { return std::bit_cast<float>(Read(object+0x248u+index*4u)); };
    for (const auto object : {script,target}) {
        Write(object,pal4::PlayerManagerSlot::kScriptVtable);
        Write(object+8u,100u);
        Write(object+0x220u+7u*4u,80u);
    }
    Check(!skills.Apply(script,123456u,script) && memory.OutstandingAllocations()==0u,
          "missing skill lookup leaves native lists untouched");
    Check(skills.Apply(script,1u,script) && Float(script,2u)==0.25f && Float(script,4u)==0.75f &&
              Read(script+700u)-Read(script+696u)==8u && Read(script+0x288u)==0u,
          "native application allocates permanent list, dispatches9105 then attached9109 and updates health");
    (void)refresh.Refresh(script);
    Check(Float(script,2u)==0.25f && Float(script,4u)==0.75f,
          "nonempty actual refresh reapplies skill effects after copying baseline stats");
    Check(skills.Apply(script,3u,script) && Float(script,3u)==0.5f,
          "9105 truncates selector3.8 and writes property3");
    Check(skills.Apply(script,4u,script) && skills.Apply(script,5u,script) &&
              Float(script,0u)==0.33f && Float(script,1u)==0.66f,
          "9112 and9113 reuse native property0/1 writes");
    skills.Process(script,1u,target);
    Check(Float(target,2u)==0.25f && Float(target,3u)==0.5f && Float(target,4u)==0.75f,
          "batch effects write the actual target rather than the source");
    Check(skills.Apply(script,8u,script) && Float(script,4u)==0.75f,
          "trigger2 skill is collected without firing trigger1");
    skills.Process(script,2u,script);
    Check(Float(script,4u)==0.125f,"matching later trigger runs collected skill");
    Check(skills.Apply(script,9u,script) && Float(script,4u)==0.875f &&
              Read(script+684u)-Read(script+680u)==12u &&
              Read(Read(script+680u)+8u)==0u,
          "finite skill uses native12-byte timed list with initialized counter");
    Check(skills.Apply(script,6u,script),"unregistered function ends chain as in original");
    const auto no_write=Float(script,2u);
    Check(skills.Apply(script,10u,script) && Float(script,2u)==no_write,
          "invalid selector conversion does not select a property write");
    Check(skills.Apply(script,11u,script),"probability skill collects without firing a mismatched trigger");
    random.Seed(123u);
    pal4::RandomNumberGenerator expected;
    expected.Seed(123u);
    (void)expected.Next();
    skills.Process(script,2u,target);
    Check(Float(target,4u)==0.925f && Float(target,0u)==0.8f &&
              Read(target+700u)-Read(target+696u)==16u &&
              random.Next()==expected.Next(),
          "9000 shares legacy RNG and recursively applies converted nonzero skill IDs to actual target");
    Check(skills.Apply(script,14u,script),"negative probability skill collects without firing");
    const auto target_end=Read(target+700u);
    random.Seed(123u); expected.Seed(123u); (void)expected.Next();
    skills.Process(script,3u,target);
    Check(Read(target+700u)==target_end && random.Next()==expected.Next(),
          "failed9000 roll consumes exactly one shared RNG value and applies no child skills");
    unsigned physical{},water{};
    for(const auto& field:pal4::NativePlayerFieldSchema::Build()) {
        if(field.name=="CA_PHYSICAL") physical=field.slot_offset-0x654u;
        if(field.name=="ERR_HUMAN_RACE_ATK") water=field.slot_offset-0x654u;
    }
    Check(physical!=0u && water!=0u,"9100 fixture fields resolve");
    for(const auto id:{15u,16u,17u}) Check(skills.Apply(script,id,script),"9100 collects under explicit trigger");
    Write(target+physical,40u); Write(target+water,0u);
    random.Seed(123u); expected.Seed(123u); (void)expected.Next(); (void)expected.Next();
    skills.Process(script,4u,target);
    Check(Read(target+physical)==45u && std::bit_cast<float>(Read(target+water))==0.25f &&
          random.Next()==expected.Next(),
          "9100 applies both real events, consumes two shared rolls and enumerates empty Player groups without extra application");
    random.Seed(123u); expected.Seed(123u); (void)expected.Next();
    skills.Process(script,5u,target);
    Check(Read(target+physical)==45u && random.Next()==expected.Next(),
          "9100 rejected first probability leaves attributes intact and consumes one roll");
    random.Seed(123u); expected.Seed(123u); (void)expected.Next(); (void)expected.Next();
    skills.Process(script,6u,target);
    Check(Read(target+physical)==50u && random.Next()==expected.Next(),
          "9100 rejected group probability retains direct effects and consumes both rolls");
    for(const auto id:{18u,19u,20u,21u,22u})
        Check(skills.Apply(script,id,script),"indexed/handle skills collect under explicit triggers");
    Write(target+water,std::bit_cast<unsigned>(0.5f));
    skills.Process(script,7u,target);
    Check(std::bit_cast<float>(Read(target+water))==0.625f,
          "9114 truncates index50.9 and increments fractional target race attack");
    unsigned beast{};
    for(const auto& field:pal4::NativePlayerFieldSchema::Build())
        if(field.name=="ERR_BEAST_RACE_ATK") beast=field.slot_offset-0x654u;
    Check(beast!=0u,"9114 upper-bound field resolves");
    Write(target+beast,std::bit_cast<unsigned>(0.5f));
    skills.Process(script,8u,target);
    Check(std::bit_cast<float>(Read(target+beast))==0.25f,"9114 accepts truncated upper bound56 and signed delta");
    const auto target_before=other;
    skills.Process(script,9u,target); skills.Process(script,10u,target);
    Check(other==target_before,"9114 out-of-range and invalid selector leave all target fields unchanged");
    random.Seed(123u); expected.Seed(123u);
    skills.Process(script,11u,target);
    auto expected_target=target_before;
    expected_target[(target-Address(other.data())+0x248u+4u*4u)/4u]=std::bit_cast<unsigned>(0.625f);
    Check(Float(target,4u)==0.625f && random.Next()==expected.Next(),
          "9132 constructs/resets a local handle then continues its attached chain without consuming RNG");
    Check(other==expected_target,"9132 affects only its attached property's target word");
    std::array<unsigned,2u> handle{123u,456u};
    const auto before_handle=other;
    Check(callbacks.InvokeVirtual(target,384u,std::array{Address(handle.data()),0xdeadu,0xbeefu})==Address(handle.data()) &&
          handle==std::array<unsigned,2u>{0xffffffffu,0u} && other==before_handle,
          "Player384 constructs the output, ignores text sources and preserves receiver fields");
    for(unsigned i=0;i<no_op_functions.size();++i) {
        Check(system.FindCallback(no_op_functions[i])==0x7e1840u,
              "actual SkillSystem registers original empty callback");
        Check(skills.Apply(script,24u+i,script),"empty callback skill collects under later trigger");
        Write(target+0x248u+4u*4u,0u);
        auto expected_no_op=other;
        expected_no_op[(target-Address(other.data())+0x248u+4u*4u)/4u]=std::bit_cast<unsigned>(0.625f);
        random.Seed(123u); expected.Seed(123u);
        skills.Process(script,12u+i,target);
        Check(other==expected_no_op && random.Next()==expected.Next(),
              "registered empty callback continues attached skill without target side effects or RNG use");
    }
    random.Seed(123u);expected.Seed(123u);
    Check(skills.Trigger(script,7u,target) && random.Next()==expected.Next(),
          "9001 invalid index exits before RNG or event lookup");
    for(const auto id:{30u,31u}) {
        const auto name=id==30u?"EAST_A":"EAST_I";
        unsigned status{};
        for(const auto& field:pal4::NativePlayerBasicFieldSchema::Build())
            if(field.name==name)status=0x154u+field.slot_offset-0x658u;
        Check(status!=0u,"9001 independent BASIC status field resolves");
        Write(script+status,0u);Write(target+status,2u);
        Write(target+0x258u,0u);
        const auto source_before=source;
        const auto target_list_end=Read(target+700u);
        random.Seed(123u);expected.Seed(123u);(void)expected.Next();
        Check(skills.Trigger(script,id,target) && Float(target,4u)==0.625f && source==source_before &&
                  Read(target+700u)==target_list_end && random.Next()==expected.Next(),
              "9001 truncates index/status/skill, reads target BASIC facet and triggers child immediately without collecting it");
        Write(target+status,1u);Write(target+0x258u,0u);
        random.Seed(123u);expected.Seed(123u);(void)expected.Next();
        Check(skills.Trigger(script,id,target) && Float(target,4u)==0.0f && random.Next()==expected.Next(),
              "9001 mismatched target status consumes one roll and suppresses child");
    }
    const auto before_probability=other;
    random.Seed(123u);expected.Seed(123u);(void)expected.Next();
    Check(skills.Trigger(script,32u,target) && other==before_probability && random.Next()==expected.Next(),
          "9001 failed probability preserves target and consumes exactly one roll");
    bool rejected{};
    Cleanup cleanup(callbacks);
    (void)pal4::ScriptSkillCollection::CleanupObjectLists(script,cleanup);
    (void)pal4::ScriptSkillCollection::CleanupObjectLists(target,cleanup);
    for(const auto object:{script,target})
        for (const auto offset : {680u,696u}) {
            (void)memory.Free(Read(object+offset));
            Write(object+offset,0u); Write(object+offset+4u,0u); Write(object+offset+8u,0u);
        }
    refresh.BindSkills(nullptr);
    Check(memory.OutstandingAllocations()==0u,"skill list growth and exception cleanup release all native buffers");
    system.Destroy();
    const auto before_destroyed=source;
    rejected=false;
    try { (void)skills.Apply(script,7u,script); } catch(const std::logic_error&) { rejected=true; }
    Check(rejected && source==before_destroyed,"retired SkillSystem rejects before collection mutation");
    inventory.Close();
}

void TestOriginalPropertySkills(const char* root) {
    pal4::NativeGameDatabase database;
    Check(database.Load(root),"original skill effect database loads");
    pal4::NativeSkillRecordCatalog catalog(database);
    pal4::NativeBaseObjectMemory memory;
    pal4::NativeInventorySession inventory;
    Check(inventory.Start()!=0u,"original skill effect inventory starts");
    pal4::NativePlayerScriptRefreshRuntime refresh(memory,inventory);
    pal4::NativePlayerSkillCallbacks callbacks(catalog,refresh);
    pal4::RandomNumberGenerator random;
    pal4::NativePlayerCombatEvents property_events;
    pal4::NativeSkillSystemOwner system(random,property_events);
    (void)system.Construct("H:/PAL4/objs");
    pal4::NativePlayerSkillRuntime skills(memory,catalog,callbacks,system);
    refresh.BindSkills(&skills);
    std::array<unsigned,pal4::PlayerManagerSlot::kObjectBytes/4u> slot{};
    const auto script=Address(slot.data())+pal4::PlayerManagerSlot::kScriptOffset;
    const auto Read=[](unsigned address) { return *reinterpret_cast<const unsigned*>(address); };
    const auto Write=[](unsigned address,unsigned value) { *reinterpret_cast<unsigned*>(address)=value; };
    Write(script,pal4::PlayerManagerSlot::kScriptVtable);
    Write(script+8u,100u);
    unsigned exercised{},event_skills{},indexed_skills{},handle_skills{};
    const auto basic_fields=pal4::NativePlayerBasicFieldSchema::Build();
    const auto script_fields=pal4::NativePlayerFieldSchema::Build();
    // Independent names for the event indices used by original trigger1
    // passives, checked against581760. Do not derive this from native handles.
    const std::array<std::pair<unsigned,const char*>,33> event_names{{
        {1u,"CA_MAX_HP"},{5u,"CA_MAX_MP"},{17u,"CA_ADDITIONAL_CRITICAL"},
        {18u,"CA_FEND_OFF"},{19u,"CA_ADDITIONAL_HITTING"},{22u,"CA_ATTACK_PHYSICAL_REACT"},
        {24u,"CA_ATTACK_FIRE_ADDITIONAL"},{26u,"CA_ATTACK_AIR_ADDITIONAL"},
        {28u,"CA_ATTACK_WATER_EXTRACT"},{47u,"EAST_I"},{48u,"CA_PHYSICAL_ATTACK_PARAM"},
        {29u,"CA_ATTACK_FIRE_EXTRACT"},{30u,"CA_ATTACK_THUNDER_EXTRACT"},
        {31u,"CA_ATTACK_AIR_EXTRACT"},{32u,"CA_ATTACK_EARTH_EXTRACT"},
        {33u,"CA_ATTACK_WATER_REACT"},{34u,"CA_ATTACK_FIRE_REACT"},
        {35u,"CA_ATTACK_THUNDER_REACT"},{36u,"CA_ATTACK_AIR_REACT"},{37u,"CA_ATTACK_EARTH_REACT"},
        {49u,"CA_MAGIC_ATTACK_PARAM"},{53u,"ERR_GHOST_RACE_ATK"},
        {58u,"EASA_WATER_IMMUNITY"},{59u,"EASA_FIRE_IMMUNITY"},{60u,"EASA_THUNDER_IMMUNITY"},
        {61u,"EASA_AIR_IMMUNITY"},{62u,"EASA_EARTH_IMMUNITY"},{63u,"EASB_BONDAGE_IMMUNITY"},
        {64u,"EASB_SEAL_IMMUNITY"},{65u,"EASB_FORBIDEN_IMMUNITY"},{66u,"EASB_SLEEP_IMMUNITY"},
        {67u,"EASB_MAD_IMMUNITY"},{68u,"EASB_DISORDER_IMMUNITY"}
    }};
    for (std::size_t i=0;i<catalog.Size();++i) {
        const auto& root_record=catalog.At(i);
        if(root_record[3]==0xffffffffu && root_record[2]==1u &&
           (root_record[4]==9114u || root_record[4]==9132u)) {
            for(unsigned offset=4u;offset<0x294u;offset+=4u) Write(script+offset,0u);
            for(const auto& field:basic_fields)
                Write(script+4u+field.slot_offset-0x658u,
                      field.type==2u ? std::bit_cast<unsigned>(0.25f) : 100u);
            (void)refresh.Refresh(script);
            Check(root_record[14]==0u,"original indexed/handle passive has no unverified attached effect");
            unsigned location{},expected{};
            if(root_record[4]==9114u) {
                const auto index=static_cast<unsigned>(std::bit_cast<float>(root_record[5]));
                Check(index>=50u && index<=56u,"original9114 selector lies in verified range");
                const std::array<const char*,7u> names{"ERR_HUMAN_RACE_ATK","ERR_DEMON_RACE_ATK",
                    "ERR_FAIRY_RACE_ATK","ERR_GHOST_RACE_ATK","ERR_IMMORTAL_RACE_ATK",
                    "ERR_SPIRIT_RACE_ATK","ERR_BEAST_RACE_ATK"};
                for(const auto& field:script_fields) if(field.name==names[index-50u])
                    location=script+field.slot_offset-0x654u;
                Check(location!=0u,"original9114 attribute resolves independently");
                expected=std::bit_cast<unsigned>(0.25f+std::bit_cast<float>(root_record[6]));
                ++indexed_skills;
            } else ++handle_skills;
            std::array<unsigned,0x284u/4u> before{};
            for(unsigned word=0;word<before.size();++word) before[word]=Read(script+word*4u);
            Check(skills.Apply(script,root_record[0],script),"original indexed/handle skill executes unchanged");
            if(location) Check(Read(location)==expected,"original9114 reaches exact race attack value");
            else for(unsigned word=0;word<before.size();++word)
                Check(Read(script+word*4u)==before[word],"original9132 changes no Player scalar attributes");
            (void)refresh.Refresh(script);
            if(location) Check(Read(location)==expected,"original9114 refresh reapplies delta without compounding");
            Cleanup cleanup(callbacks);
            (void)pal4::ScriptSkillCollection::CleanupObjectLists(script,cleanup);
            for(unsigned offset=4u;offset<0x294u;offset+=4u) Write(script+offset,0u);
            Write(script+8u,100u);
            std::cout << "Original native indexed/handle Skill=" << root_record[0] << " FUNCTION=" << root_record[4] << '\n';
        }
        if(root_record[3]==0xffffffffu && root_record[2]==1u && root_record[4]==9100u) {
            // Each original passive is isolated so a previous test's permanent
            // list cannot affect refresh ordering or compound its attributes.
            for(unsigned offset=4u;offset<0x294u;offset+=4u) Write(script+offset,0u);
            for(const auto& field:basic_fields)
                Write(script+4u+field.slot_offset-0x658u,
                      field.type==2u ? std::bit_cast<unsigned>(0.25f) : 100u);
            (void)refresh.Refresh(script);
            std::unordered_map<unsigned,unsigned> expected;
            auto chain=catalog.Address(root_record[0]);
            for(unsigned depth=0;chain!=0u;++depth) {
            Check(depth<catalog.Size(),"original event attached chain terminates");
            Check(Read(chain+16u)==9100u,"original event chain uses implemented event outcomes");
            for(const auto parameter:{32u,44u}) {
            const auto index=static_cast<unsigned>(std::bit_cast<float>(Read(chain+parameter)));
            if(index==0u) continue;
            const char* name=nullptr;
            for(const auto& entry:event_names) if(entry.first==index) name=entry.second;
            Check(name!=nullptr,"original9100 index has an independent event name");
            const pal4::NativePlayerBasicField* field=nullptr;
            for(const auto& candidate:script_fields) if(candidate.name==name) field=&candidate;
            Check(field!=nullptr,"original9100 target field resolves");
            const auto location=script+field->slot_offset-0x654u;
            const auto old=expected.contains(location) ? expected.at(location) : Read(location);
            const auto before=field->type==2u ? std::bit_cast<float>(old) : static_cast<float>(old);
            const auto base=field->type==2u ? 0.25f : 100.0f;
            const auto scale=std::bit_cast<float>(Read(chain+parameter+4u));
            const auto mode=static_cast<unsigned>(std::bit_cast<float>(Read(chain+parameter+8u)));
            Check(mode==0u || mode==2u || mode==3u,"original9100 uses expected tested modes");
            const auto result=mode==2u ? scale : before+(mode==3u ? base*scale : scale);
            expected[location]=field->type==2u ? std::bit_cast<unsigned>(result) : static_cast<unsigned>(result);
            }
            const auto next=Read(chain+56u);
            chain=next ? catalog.Address(next) : 0u;
            }
            random.Seed(123u);
            Check(skills.Apply(script,root_record[0],script),"unchanged original9100 passive executes");
            for(const auto& [location,value]:expected)
                Check(Read(location)==value,"original9100 modifies the independently expected attribute and value");
            (void)refresh.Refresh(script);
            for(const auto& [location,value]:expected)
                Check(Read(location)==value,"original9100 survives full native refresh without compounding");
            std::cout << "Original event Skill=" << root_record[0] << " params=";
            for(unsigned p=5u;p<14u;++p) std::cout << std::bit_cast<float>(root_record[p]) << ',';
            std::cout << '\n';
            Cleanup cleanup(callbacks);
            (void)pal4::ScriptSkillCollection::CleanupObjectLists(script,cleanup);
            for(unsigned offset=4u;offset<0x294u;offset+=4u) Write(script+offset,0u);
            Write(script+8u,100u);
            ++event_skills;
        }
        if(root_record[3]==0xffffffffu && root_record[2]==1u && root_record[4]==9000u) {
            std::cout << "Original probability Skill=" << root_record[0]
                      << " probability=" << std::bit_cast<float>(root_record[5]) << " outcomes=";
            for(unsigned p=6u;p<14u;++p) {
                const auto id=static_cast<unsigned>(std::bit_cast<float>(root_record[p]));
                if(!id) continue;
                const auto child=catalog.Address(id);
                Check(child!=0u,"original passive probability outcome resolves");
                std::cout << id << ":fn" << Read(child+16u) << ":trigger" << Read(child+8u)
                          << ":mutex" << Read(child+60u) << ',';
            }
            std::cout << '\n';
            for(const auto& [table_name,table]:database.Tables()) {
                for(const auto& row:table.rows)
                    for(std::size_t column=0u;column<row.size();++column)
                        if(table.fields[column].name!="ID" &&
                           std::holds_alternative<unsigned>(row[column]) &&
                           std::get<unsigned>(row[column])==root_record[0])
                            std::cout << "Probability reference Skill=" << root_record[0]
                                      << " table=" << table_name << " field=" << table.fields[column].name
                                      << " row=" << (&row-table.rows.data()) << '\n';
            }
        }
        if (root_record[3]!=0xffffffffu || root_record[2]!=1u ||
            (root_record[4]!=9105u && root_record[4]!=9109u &&
             root_record[4]!=9112u && root_record[4]!=9113u)) continue;
        std::array<unsigned,5> expected{};
        auto address=catalog.Address(root_record[0]);
        // Independent expectation for the entire actual attached chain. Any
        // unexpected callback fails this test instead of skipping original data.
        for (unsigned depth=0;address!=0u;++depth) {
            Check(depth<catalog.Size(),"original property skill chain terminates");
            const auto function=Read(address+16u);
            unsigned index{};
            if (function==9105u) {
                const auto selector=static_cast<int>(std::bit_cast<float>(Read(address+24u)));
                Check(selector==1 || selector==3,"original property selector has a supported branch");
                index=selector==1 ? 2u : 3u;
            } else if(function==9109u) index=4u;
            else if(function==9112u) index=0u;
            else if(function==9113u) index=1u;
            else Check(false,"original property chain requires another real effect implementation");
            expected[index]=Read(address+20u);
            const auto next=Read(address+56u);
            address=next ? catalog.Address(next) : 0u;
        }
        Check(skills.Apply(script,root_record[0],script),"original permanent property skill applies");
        for(unsigned index=0;index<5u;++index)
            Check(Read(script+0x248u+index*4u)==expected[index],"original Skill fields reach exact native property words");
        (void)refresh.Refresh(script);
        for(unsigned index=0;index<5u;++index)
            Check(Read(script+0x248u+index*4u)==expected[index],"original skill survives full nonempty refresh");
        Cleanup cleanup(callbacks);
        (void)pal4::ScriptSkillCollection::CleanupObjectLists(script,cleanup);
        ++exercised;
        std::cout << "Original native property Skill=" << root_record[0] << " FUNCTION=" << root_record[4] << '\n';
    }
    Check(exercised==3u,"all three original permanent trigger1 property skills execute");
    Check(event_skills==35u,"all35 unchanged original permanent trigger1 event skills execute");
    Check(indexed_skills==1u && handle_skills==1u,"all original permanent trigger1 indexed/handle skills execute");
    for(const auto offset:{680u,696u}) (void)memory.Free(Read(script+offset));
    refresh.BindSkills(nullptr);
    Check(memory.OutstandingAllocations()==0u,"original skill list storage releases");
    inventory.Close();
}

}  // namespace

int main(int argc,char** argv) {
    TestNativeCombatEvents();
    TestRefreshWithEmptyInventoryAndSkills();
    TestNativeSkillEffectsAndRefresh();
    if(argc>1) TestOriginalPropertySkills(argv[1]);
    return EXIT_SUCCESS;
}
