#include "pal4/native_floor_combat_areas.h"
#include "pal4/floor_sound_lookup.h"
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
pal4::NativeGameDbTable Table() {
    pal4::NativeGameDbTable table;
    table.name="CombatScene";
    table.fields={{"ID",1u},{"WORLD",2u},{"COMBAT_SCENE",2u},{"FLOOR",1u}};
    table.rows={
        {1u,std::string("A"),std::string("old"),49u},
        {2u,std::string("A"),std::string("second"),49u},
        {1u,std::string("A"),std::string("replacement"),49u},
        {3u,std::string("B"),std::string("plus"),50u},
        {4u,std::string("C"),std::string("minus"),48u},
        {5u,std::string("D"),std::string("previous"),100u},
        {6u,std::string("E"),std::string{},49u},
        {7u,std::string("E"),std::string("later"),50u},
        {8u,std::string("ZERO"),std::string("zero"),0u}};
    return table;
}
}
int main(int argc, char** argv) try {
    auto table=Table();
    pal4::NativeFloorCombatAreas areas(table);
    Check(areas.Records().size()==8u,"duplicate ID replaces its original row position");
    Check(areas.Resolve("a",127,255)=="replacement","CRT truncation/current match/case folding/original first row");
    Check(areas.Resolve("B",127,255)=="plus","current+1 lookup follows current");
    Check(areas.Resolve("C",125,255)=="minus","current-1 follows current and plus candidate");
    Check(areas.Resolve("D",127,255)=="previous","fallback floor is tried last");
    Check(areas.Resolve("E",127,255).empty(),"matched empty area stops at first match");
    Check(areas.Resolve("missing",127,255).empty(),"lookup does not fabricate a combat area");
    Check(areas.Resolve("ZERO",-1,255)=="zero","negative scaled values truncate toward zero");
    const auto maximum=std::numeric_limits<std::int32_t>::max();
    const auto minimum=std::numeric_limits<std::int32_t>::min();
    Check(pal4::FloorSoundLookup::Candidates(maximum,17)[1]==minimum &&
          pal4::FloorSoundLookup::Candidates(minimum,17)[2]==maximum,"original signed candidate wrap");
    table.rows[0][3]=std::string("invalid");
    bool rejected{};
    try { pal4::NativeFloorCombatAreas bad(table); } catch(const std::invalid_argument&) { rejected=true; }
    Check(rejected,"native schema values reject before a partial catalog is published");
    if(argc==2) {
        pal4::NativeGameDatabase database;
        Check(database.Load(argv[1]),"load original packed database");
        for(const auto& field:database.Tables().at("CombatScene").fields)
            std::cout<<"Original CombatScene field "<<field.name<<" type="<<field.type<<'\n';
        pal4::NativeFloorCombatAreas original(database);
        Check(!original.Records().empty(),"original CombatScene table contains records");
        std::cout<<"Original CombatScene rows="<<original.Records().size()<<'\n';
        for(const auto& record:original.Records())
            if(record.world=="M01")
                std::cout<<"M01 row id="<<record.id<<" floor="<<record.floor<<" area="<<record.area<<'\n';
        const auto cave=original.Resolve("M01",25,0);
        Check(!cave.empty(),"retained original cave floor sample resolves via real database");
        std::cout<<"Original retained M01 floor25/fallback0 -> "<<cave<<'\n';
    }
    std::cout<<"PASS: native floor combat area lookup, schema/row ownership and original fallback order\n";
    return 0;
} catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<'\n';return 1; }
