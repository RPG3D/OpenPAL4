#include "pal4/native_monster_encounter_storage.h"
#include "pal4/monster_behavior_manager.h"
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::uint32_t Word(std::uint32_t p) { return *reinterpret_cast<const std::uint32_t*>(p); }
}
int main() try {
    pal4::NativeMonsterEncounterStorage storage;
    pal4::MonsterBehaviorManagerState image{};
    for (unsigned i = 0; i < 130u; ++i) {
        storage.Append(image, {100u + i, i, i % 3u});
        storage.AddName(image, "m" + std::to_string(i) + std::string(i, 'x'));
    }
    const auto records = image.words[0x714u / 4u];
    auto history = image.words[0x724u / 4u];
    Check(image.words[0x718u / 4u] == records + 130u * 12u &&
          image.words[0x728u / 4u] == history + 130u * 16u, "host growth publishes logical counts");
    for (unsigned i = 0; i < 130u; ++i) {
        Check(Word(records + i * 12u) == 100u + i && Word(records + i * 12u + 4u) == i,
            "host record growth retains insertion order");
        const auto expected = "m" + std::to_string(i) + std::string(i, 'x');
        Check(expected == reinterpret_cast<const char*>(Word(history + i * 16u + 4u)),
            "short/long name pointers survive borrowed view growth");
        storage.AddName(image, expected);
    }
    Check(image.words[0x728u / 4u] == history + 130u * 16u,
        "duplicate names do not extend history");
    const std::string_view alias(reinterpret_cast<const char*>(Word(history + 4u)));
    image.words[0x728u / 4u] = history;
    image.words[0x718u / 4u] = records;
    storage.AddName(image, alias);
    storage.Append(image, {7, 8, 9});
    history = image.words[0x724u / 4u];
    Check(std::string_view(reinterpret_cast<const char*>(Word(history + 4u))) == "m0" &&
          image.words[0x728u / 4u] == history + 16u &&
          image.words[0x718u / 4u] == image.words[0x714u / 4u] + 12u &&
          Word(image.words[0x714u / 4u]) == 7u, "consumed arrays restart without stale records or aliased names");
    Check(!storage.ReleaseHistoryRecord(history + 1u) && storage.ReleaseHistoryRecord(history) &&
          !Word(history + 4u) && !Word(history + 8u) && !Word(history + 12u),
        "host clear releases only aligned owned views without reading a legacy header");
    Check(storage.ReleaseHistoryRecord(history), "clearing an owned empty record is idempotent");
    const auto end = image.words[0x718u / 4u];
    image.words[0x718u / 4u] = end + 1u;
    bool rejected{};
    try { storage.Append(image, {1, 2, 3}); } catch (const std::logic_error&) { rejected = true; }
    image.words[0x718u / 4u] = end;
    Check(rejected, "externally corrupted logical end rejects before host mutation");
    Check(storage.ReleaseBuffer(image.words[0x714u / 4u]) && storage.ReleaseBuffer(history),
        "creating owner handles both buffers at game destructor boundary");
    for (auto offset : {0x714u,0x718u,0x71cu,0x724u,0x728u,0x72cu})
        Check(!image.words[offset / 4u], "released host views are cleared");
    pal4::MonsterBehaviorManagerState next{};
    storage.AddName(next, "new");
    Check(storage.ReleaseBuffer(next.words[0x724u / 4u]), "storage rebinds after prior owner's full release");
    next.words[0x714u / 4u] = 1234u;
    rejected = false;
    try { storage.Append(next, {1, 2, 3}); } catch (const std::logic_error&) { rejected = true; }
    Check(rejected && next.words[0x714u / 4u] == 1234u,
        "host ownership does not replace an existing legacy or foreign allocation");
    std::cout << "PASS: native monster encounter STL ownership, view growth/consumption, aliasing and cleanup\n";
    return 0;
} catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
