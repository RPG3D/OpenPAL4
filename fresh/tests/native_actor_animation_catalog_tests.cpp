#include "pal4/native_actor_animation_catalog.h"
#include "pal4/native_actor_animation_events.h"
#include "pal4/native_package_archive.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestParameterFormat() {
    std::vector<std::uint8_t> bytes;
    const auto word = [&](std::uint32_t value) {
        for (unsigned shift = 0u; shift < 32u; shift += 8u)
            bytes.push_back(static_cast<std::uint8_t>(value >> shift));
    };
    const auto text = [&](std::string_view value) {
        word(static_cast<std::uint32_t>(value.size()));
        bytes.insert(bytes.end(), value.begin(), value.end());
    };
    word(20070323u);
    word(2u);
    text("message");
    word(3u);
    text("file"); text(std::string(100u, 'x'));
    text("file"); text("second");
    text("nul"); text(std::string_view("a\0ignored", 9u));
    text("message");
    word(0u);
    std::vector<pal4::AnimationStringTableData> tables;
    Check(pal4::AnimationStringTableCollection::LoadFromBytes(tables, bytes) &&
              tables.size() == 2u && tables[0].entries.size() == 3u &&
              tables[1].entries.empty(),
          "AMD host loader retains table order and duplicate entries");
    const auto* table = pal4::AnimationStringTableCollection::Find(tables, "message");
    Check(table == &tables[0] &&
              *pal4::AnimationStringTable::FindString(*table, "file") ==
                  std::string(100u, 'x') &&
              *pal4::AnimationStringTable::FindString(*table, "nul") == "a" &&
              !pal4::AnimationStringTable::FindString(*table, "missing"),
          "AMD lookup uses first match and strings stop at the first NUL");
    for (std::size_t size = 0u; size < bytes.size(); ++size) {
        Check(!pal4::AnimationStringTableCollection::LoadFromBytes(
                  tables, std::span<const std::uint8_t>(bytes).first(size)) &&
                  tables.empty(),
              "every truncated AMD prefix rejects and clears previous data");
    }
    bytes[0] ^= 1u;
    Check(!pal4::AnimationStringTableCollection::LoadFromBytes(tables, bytes),
          "AMD rejects incorrect file magic");
}

std::string_view EventText(const pal4::AnimationEventData& event,
                           std::size_t offset) {
    const std::string_view field(
        reinterpret_cast<const char*>(event.data() + offset), 64u);
    return field.substr(0u, field.find('\0'));
}

}  // namespace

int main(int argc, char** argv) {
    TestParameterFormat();
    Check(argc == 2, "original game root argument");
    const std::filesystem::path root(argv[1]);
    pal4::NativePackageArchive archive;
    Check(archive.Open(
              (root / "gamedata" / "PALActor.cpk").string().c_str()),
          "open original actor archive");
    constexpr std::array<std::pair<std::string_view, std::size_t>, 4> models{{
        {"101", 56u}, {"103", 52u}, {"106", 43u}, {"105", 42u}}};
    pal4::NativeActorAnimationCatalog catalog;
    std::size_t loaded_sidecars = 0u;
    std::size_t parameter_tables = 0u;
    for (const auto& [model, expected_actions] : models) {
        const auto tab = std::string(model) + "\\" +
                         std::string(model) + ".tab";
        Check(archive.Find(tab.c_str()) >= 0 &&
                  catalog.Load(root, model) &&
                  catalog.Entries().size() == expected_actions &&
                  catalog.Entries().front().first == "C01" &&
                  catalog.Entries().front().second == "C01.anm",
              "original actor animation script uses reconstructed parser");
        const auto first_clip = std::string(model) + "\\" +
                                catalog.Entries().front().second;
        Check(archive.Find(first_clip.c_str()) >= 0,
              "first action points to original RenderWare clip");
        for (const auto& [action, filename] : catalog.Entries()) {
            Check(!action.empty() && !filename.empty(),
                  "original actor action has a name and clip");
            const auto clip = std::string(model) + "\\" + filename;
            if (archive.Find(clip.c_str()) < 0)
                std::cerr << "Missing " << clip << '\n';
            Check(archive.Find(clip.c_str()) >= 0,
                  "original actor action clip exists in PALActor CPK");
            pal4::NativeActorAnimationEvents messages;
            const auto amd = std::string(model) + "\\" +
                filename.substr(0u, filename.size() - 4u) + ".amd";
            const auto raw = archive.ReadMember(amd.c_str());
            if (raw) {
                Check(messages.Load(root, model, filename) &&
                          messages.SidecarLoaded(),
                      "original animation AMD sidecar loads with its AMF");
                ++loaded_sidecars;
                parameter_tables += messages.SidecarTables().size();
                if (model == "101" && action == "Z05") {
                    const auto* effect = pal4::AnimationStringTableCollection::Find(
                        messages.SidecarTables(), "ef1");
                    Check(effect &&
                              pal4::AnimationStringTable::FindString(*effect, "ef") &&
                              *pal4::AnimationStringTable::FindString(*effect, "ef") == "EFF4" &&
                              pal4::AnimationStringTable::FindString(*effect, "bind") &&
                              *pal4::AnimationStringTable::FindString(*effect, "bind") ==
                                  "hook_ef_pos",
                          "original Z05 effect retains its resource and actor hook");
                }
                if (loaded_sidecars <= 2u) {
                    for (const auto& table : messages.SidecarTables()) {
                        std::cout << model << '/' << action << " AMD table=" << table.name << '\n';
                        for (const auto& [key, value] : table.entries)
                            std::cout << "  " << key << '=' << value << '\n';
                    }
                }
            }
        }
        if (model == "101") {
            const auto action = std::find_if(
                catalog.Entries().begin(), catalog.Entries().end(),
                [](const auto& entry) { return entry.first == "J08"; });
            Check(action != catalog.Entries().end(),
                  "Tianhe has the Q01 J08 action");
            pal4::NativeActorAnimationEvents events;
            Check(events.Load(root, model, action->second),
                  "load original Q01 J08 animation messages");
            Check(events.Events().size() == 1u && !events.SidecarLoaded() &&
                      events.SidecarTables().empty(),
                  "Q01 J08 has one original animation event");
            const auto& event = events.Events().front();
            std::uint32_t trigger_bits{};
            std::memcpy(&trigger_bits, event.data() + 192u, 4u);
            Check(EventText(event, 0u) == "101" &&
                      EventText(event, 64u) == "J08" &&
                      EventText(event, 128u) == "HOLD" &&
                      std::bit_cast<float>(trigger_bits) > 2.4f &&
                      std::bit_cast<float>(trigger_bits) < 2.5f,
                  "original J08 HOLD event has a real animation trigger");
            Check(!events.Load(root, model, "missing.anm") &&
                      events.Events().empty() && !events.SidecarLoaded() &&
                      events.SidecarTables().empty(),
                  "missing animation message clears prior action events");
        }
    }
    Check(!catalog.Load(root, "missing") && catalog.Entries().empty(),
          "invalid model clears previous animation catalog");
    pal4::NativePackageArchive objects;
    Check(objects.Open((root / "gamedata" / "palobject.cpk").string().c_str()),
          "open original object archive for Machine actions");
    Check(catalog.Load(objects, "OM07", "OM07") &&
              catalog.Entries().size() == 1u &&
              catalog.Entries().front().first == "OM07" &&
              catalog.Entries().front().second == "OM07.anm",
          "Q01 Machine action table uses reconstructed parser");
    Check(!catalog.Load(objects, "../OM07", "OM07") &&
              catalog.Entries().empty(),
          "invalid object archive directory clears prior actions");
    Check(loaded_sidecars == 26u && parameter_tables == 41u,
          "four original actor catalogs retain all 26 AMD files and 41 tables");
    std::cout << "PASS: four original actor animation catalogs, AMD files="
              << loaded_sidecars << " tables=" << parameter_tables << '\n';
}
