#include "pal4/native_mission_record_catalog.h"
#include "pal4/native_game_database.h"
#include "pal4/native_pal_game_iv_texture_storage.h"
#include "pal4/native_system_setting_stats_data.h"
#include "pal4/native_dialog_settings_snapshot.h"
#include "pal4/native_inventory_session.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

namespace {
void Check(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
void Set(std::uint32_t address,std::int32_t value) {
    std::memcpy(reinterpret_cast<void*>(static_cast<std::uintptr_t>(address)),&value,4u);
}
}

int main(int argc,char** argv) try {
    if(argc!=2) return 1;
    pal4::NativeGameDatabase database;
    Check(database.Load(argv[1]),"load original database");
    pal4::NativeMissionRecordCatalog catalog(database);
    Check(!catalog.Records().empty(),"original Mission records are populated");
    const auto sorted=catalog.Sorted();
    Check(sorted.size()==database.Tables().at("Mission").rows.size(),"all original Mission rows retained");
    for(std::size_t i=1;i<sorted.size();++i)
        Check(sorted[i-1]->category>=sorted[i]->category,"shared original signed category order");
    pal4::NativePalGameIVTextureStorage game([](std::uint32_t){});
    (void)game.Construct();
    const auto stories=game.StoryObjects();
    Check(stories.size()==38u,"actual PALGameIV Story storage");
    pal4::NativeDialogSettingsSnapshot settings;
    settings.Reset(settings.State());
    pal4::NativeInventorySession inventory;
    pal4::NativeSystemSettingStatsData stats(database,settings,game,inventory);
    for(const auto pointer:stories) Set(pointer,-1);
    std::size_t probes{};
    for(std::size_t category=0;category<stories.size();++category) {
        std::set<std::int32_t> levels{-1,0,1};
        for(const auto& record:catalog.Records()) if(record.category==category) {
            levels.insert(record.required);
            if(record.required>INT32_MIN) levels.insert(record.required-1);
        }
        for(const auto level:levels) {
            Set(stories[category],level);
            const auto assigned=catalog.Assigned(game);
            Check(assigned.size()==stories.size(),"one optional task per actual Story slot");
            double expected_progress=0.0;
            for(std::size_t slot=0;slot<stories.size();++slot) {
                const auto limit=slot==category?level:-1;
                const pal4::NativeMissionRecord* best=nullptr;
                for(const auto& record:catalog.Records())
                    if(record.category==slot && record.required<=limit && (!best || record.required>best->required))
                        best=&record;
                Check(assigned[slot].has_value()==(best!=nullptr),"locked tasks remain absent");
                settings.WriteStoryWord(settings.State().Word(184u)+static_cast<std::uint32_t>(4u*slot),
                    std::bit_cast<std::uint32_t>(limit));
                if(!best) continue;
                expected_progress+=best->progress;
                const auto chosen=std::find_if(catalog.Records().begin(),catalog.Records().end(),
                    [&](const auto& record) { return record.id==assigned[slot]->id && record.category==slot &&
                        record.required==best->required && record.title==assigned[slot]->title &&
                        record.availability==assigned[slot]->availability; });
                Check(chosen!=catalog.Records().end(),"highest unlocked original task/title/availability reaches UI record");
            }
            Check(stats.CalculateStatValue(stats.GlobalData())==static_cast<float>(expected_progress),
                "production SystemSetting scoring shares catalogue and preserves original row result");
            ++probes;
        }
        Set(stories[category],-1);
    }
    game.Destroy();
    try { (void)catalog.Assigned(game); Check(false,"retired game must reject task reads"); }
    catch(const std::logic_error&) {}
    std::cout<<"PASS: original Mission rows="<<catalog.Records().size()<<" Story threshold probes="<<probes<<'\n';
    return 0;
} catch(const std::exception& error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; return 1; }
