#include "pal4/native_monster_record_catalog.h"
#include "pal4/native_combat_actor_copy_runtime.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <tuple>

namespace {
void Check(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
std::uint32_t Address(const void* p) { return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(p)); }
std::string_view Text(std::uint32_t address, std::uint32_t length) {
    return {reinterpret_cast<const char*>(static_cast<std::uintptr_t>(address)), length};
}
}
int main(int argc, char** argv) {
    try {
        Check(argc == 2, "original game root required");
        pal4::NativeGameDatabase db;
        Check(db.Load(argv[1]), "original database load");
        std::array<std::uint32_t, 292u> actor{};
        pal4::NativeCombatActorCopyRuntime copy;
        std::array<std::string, 11u> expected_texts;
        std::array<std::uint32_t, 84u> expected_properties{};
        std::size_t total = 0;
        {
            pal4::NativeMonsterRecordCatalog catalog(db);
            const auto& table = db.Tables().at("Monster");
            Check(table.fields.size() == 91u && catalog.Count() == 127u,
                  "original Monster table dimensions");
            Check(catalog.UnboundFields() == std::vector<std::string>{"MONSTER_MAGIC_6", "ACTTYPE"},
                  "exact original unbound columns retain original empty converter");
            std::cout << "Original unbound columns:";
            for (const auto& name : catalog.UnboundFields()) std::cout << ' ' << name;
            std::cout << '\n';
            Check(catalog.Count() == table.rows.size() && catalog.Count() > 0u,
                  "catalog owns every Monster row");
            const auto column = [&](std::string_view name) {
                auto f = std::find_if(table.fields.begin(), table.fields.end(),
                    [&](const auto& value) { return value.name == name; });
                Check(f != table.fields.end(), "original Monster field exists");
                return static_cast<std::size_t>(f - table.fields.begin());
            };
            constexpr std::pair<std::string_view, std::uint32_t> scalars[]{
                {"MONSTER_ID",400u},{"CA_MAX_HP",4u},{"CA_MAX_MP",12u},
                {"CA_SPEED",24u},{"MONSTER_MAGIC_1",336u},{"MONSTER_MAGIC_5",352u},
                {"MA_MAX_DROP_MONEY",356u},{"MA_MIN_DROP_MONEY",360u},
                {"MA_ATK_ADDED_RAGE",364u},{"MDT_1_ID",368u},{"MDT_4_ID",380u},
                {"COUNT",404u},{"IS_BOSS",584u},{"PHYSICAL_ATK_TARGETT",588u},
                {"PHYSICAL_ATK_RANGE",592u},
            };
            constexpr std::pair<std::string_view, std::uint32_t> floats[]{
                {"CA_ADDITIONAL_CRITICAL",36u},{"CA_ATTACK_PHYSICAL_REACT",76u},
                {"MDT_1_RATE",384u},{"MDT_4_RATE",396u},
            };
            constexpr std::string_view texts[]{"MA_NAME","MA_ICON","MA_MODEL","MA_TEXTURE",
                "MA_DESC","EFF_FINAL_HIT","SOUND_WOUNDED1","SOUND_WOUNDED2",
                "SOUND_WOUNDED3","SOUND_WOUNDED4","SOUND_WOUNDED5"};
            for (std::size_t i = 0; i < table.rows.size(); ++i) {
                const auto& row = table.rows[i];
                const auto id = std::bit_cast<std::int32_t>(std::get<std::uint32_t>(row[column("MONSTER_ID")]));
                const auto address = catalog.Find(id);
                Check(catalog.Owns(address) && address == Address(catalog.Record(i).data()) &&
                      !catalog.Owns(address + 4u), "signed lookup returns original source row");
                const auto& words = catalog.Record(i);
                for (const auto& [name, offset, fallback] :
                     {std::tuple{"COMBAT_POSITION", 328u, 6u},
                      std::tuple{"AI_TYPE", 332u, 0xffffffffu}}) {
                    const auto field = std::find_if(table.fields.begin(), table.fields.end(),
                        [&](const auto& entry) { return entry.name == name; });
                    const auto expected = field == table.fields.end() ? fallback :
                        std::get<std::uint32_t>(row[static_cast<std::size_t>(field-table.fields.begin())]);
                    Check(words[offset/4u] == expected, "declared BASIC fields override original defaults");
                }
                for (const auto& [name, offset] : scalars)
                    Check(words[offset/4u] == std::get<std::uint32_t>(row[column(name)]),
                          "original Monster integer offset");
                for (const auto& [name, offset] : floats)
                    Check(words[offset/4u] == std::bit_cast<std::uint32_t>(std::get<float>(row[column(name)])),
                          "original Monster float bit preservation");
                for (std::size_t j = 0; j < 11u; ++j) {
                    const auto& value = std::get<std::string>(row[column(texts[j])]);
                    const auto word = 102u + j*4u;
                    Check(words[word+2u] == value.size() && words[word+3u] == value.size(),
                          "original Monster text lengths");
                    if (value.empty()) Check(words[word+1u] == 0u, "empty text has no allocation");
                    else {
                        Check(Text(words[word+1u], words[word+2u]) == value &&
                              copy.ReadByte(words[word+1u]-1u) == 0xfeu,
                              "original bytes retained and host copy is mandatory");
                    }
                }
                ++total;
            }
            for (const auto missing : {std::numeric_limits<std::int32_t>::min(), -1, 0,
                                       std::numeric_limits<std::int32_t>::max()})
                Check(catalog.Find(missing) == 0u, "missing signed IDs return null");
            for (const auto id : {2579,2580}) {
                const auto source = catalog.Find(id);
                Check(source != 0u, "first cave battle has actual DB record");
                std::cout << "Cave monster " << id << " model="
                          << Text(copy.ReadWord(source+444u),copy.ReadWord(source+448u))
                          << " HP=" << copy.ReadWord(source+4u) << '\n';
                const auto* words = reinterpret_cast<const std::uint32_t*>(source);
                for (std::size_t j=0; j<11u; ++j)
                    expected_texts[j] = Text(words[103u+j*4u],words[104u+j*4u]);
                std::copy_n(words,84u,expected_properties.begin());
                Check(pal4::ActorManagerCombatActor::CopyFrom(Address(actor.data()),source,copy) == Address(actor.data()),
                      "shared 531170 copies real original monster");
                Check(actor[240u] == static_cast<std::uint32_t>(id), "combat actor preserves DB identity");
                Check(actor[243u] != words[103u], "actor text uses independent host storage");
            }
            Check(copy.OwnedTextCount() > 0u, "native actor text has a host owner");
            bool size_rejected = false;
            try { copy.SetTextSize(Address(actor.data())+968u, actor[244u]+1u); }
            catch(const std::logic_error&) { size_rejected = true; }
            Check(size_rejected, "host text size rejects an incompatible buffer length");
            // Malformed binding/row fixtures use the real loaded table and restore
            // it after each probe; no separate database parser is introduced.
            auto& mutable_table = const_cast<pal4::NativeGameDbTable&>(table);
            const auto original = mutable_table;
            const auto rejects = [&] {
                bool rejected = false;
                try { pal4::NativeMonsterRecordCatalog invalid(db); }
                catch(const std::runtime_error&) { rejected = true; }
                mutable_table = original;
                Check(rejected, "invalid Monster table is rejected");
            };
            for (const auto* name : {"COMBAT_POSITION", "AI_TYPE"}) {
                const auto field = std::find_if(mutable_table.fields.begin(), mutable_table.fields.end(),
                    [&](const auto& entry) { return entry.name == name; });
                if (field != mutable_table.fields.end()) {
                    const auto offset = static_cast<std::size_t>(field - mutable_table.fields.begin());
                    mutable_table.fields.erase(field);
                    for (auto& row : mutable_table.rows) row.erase(row.begin()+offset);
                }
            }
            {
                pal4::NativeMonsterRecordCatalog defaults(db);
                for (std::size_t i = 0; i < defaults.Count(); ++i)
                    Check(defaults.Record(i)[82u] == 6u && defaults.Record(i)[83u] == 0xffffffffu,
                          "omitted BASIC fields retain original constructor defaults");
            }
            mutable_table = original;
            mutable_table.fields[column("MA_MODEL")].type = 1u;
            rejects();
            mutable_table.fields[column("MA_MODEL")].name = "UNSUPPORTED_MODEL_FIELD";
            rejects();
            mutable_table.rows[0u].pop_back();
            rejects();
            mutable_table.rows[0u][column("MONSTER_ID")] = mutable_table.rows[1u][column("MONSTER_ID")];
            rejects();
            mutable_table.rows[0u][column("MA_MODEL")] = std::uint32_t{1u};
            rejects();
            // Catalog and database storage may disappear before the combat actor.
            Check(!db.Load(std::filesystem::path(argv[1])/"missing"), "database intentionally unloaded");
        }
        for (std::size_t i=0; i<84u; ++i)
            Check(actor[1u+i] == expected_properties[i] && actor[85u+i] == expected_properties[i],
                  "both native property snapshots survive source destruction");
        for (std::size_t i=0; i<11u; ++i)
            Check(Text(actor[243u+i*4u],actor[244u+i*4u]) == expected_texts[i],
                  "native copied text survives source destruction");
        copy.ReleaseActorTexts(Address(actor.data()));
        Check(copy.OwnedTextCount() == 0u, "actor text cleanup releases every buffer");
        copy.ReleaseActorTexts(Address(actor.data()));
        bool missing=false;
        try { pal4::NativeMonsterRecordCatalog invalid(db); }
        catch(const std::runtime_error&) { missing=true; }
        Check(missing, "missing database cannot prepare monster records");
        std::cout << "Verified original Monster records=" << total
                  << "; cave2579/2580 shared-copy and source-lifetime/cleanup passed\n";
    } catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
