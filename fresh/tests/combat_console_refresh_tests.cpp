#include "pal4/combat_console.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kConsole = 0x5000u;

struct Iterator final : pal4::ICombatConsoleActorIterator {
    std::vector<std::string>& calls;
    std::string name;
    std::vector<std::uint32_t> actors;
    std::size_t position = 0u;

    Iterator(std::vector<std::string>& log, std::string label,
             std::vector<std::uint32_t> values)
        : calls(log), name(std::move(label)), actors(std::move(values)) {}
    ~Iterator() override { calls.push_back("destroy-" + name); }
    bool AtEnd() override {
        calls.push_back("end-" + name);
        return position == actors.size();
    }
    std::uint32_t Current() override {
        calls.push_back("current-" + name);
        return actors.at(position);
    }
    void Advance() override {
        calls.push_back("advance-" + name);
        ++position;
    }
};

struct Runtime final : pal4::ICombatConsoleSceneRuntime {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, bool>> visibility;
    std::vector<std::uint32_t> roles{0xa1u, 0xa2u};
    std::vector<std::uint32_t> monsters{0xb1u, 0xb2u};
    std::vector<std::uint32_t> count_monsters{0xc1u, 0xc2u, 0xc3u};
    std::uint32_t state = 2u;
    std::uint32_t last_image = 0u;
    std::uint32_t last_animation = 0u;
    float last_x = -1.0f;

    Runtime() {
        for (std::uint32_t index = 0u; index < 9u; ++index)
            words[kConsole + 224u + index * 4u] = 0x8000u + index * 0x100u;
        words[kConsole + 220u] = 0x7900u;
    }
    std::uint32_t GameState() override {
        calls.push_back("state");
        return state;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read");
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back(value ? "enable" : "disable");
        words[address] = value;
    }
    void SetChildVisible(std::uint32_t child, bool visible) override {
        calls.push_back(visible ? "show" : "hide");
        visibility.emplace_back(child, visible);
    }
    std::string ImageSetName() override {
        calls.push_back("image-set");
        return "atlas";
    }
    void SetImage(std::uint32_t image, std::u32string_view,
                  std::u32string_view name) override {
        calls.push_back(name == U"role" ? "image-role" : "image-monster");
        last_image = image;
    }
    std::uint32_t SetPosition(std::uint32_t image, std::uint32_t mode,
                              std::uint32_t rectangle) override {
        Check(mode == 1u && rectangle == kConsole + 44u,
              "image rectangle and mode");
        calls.push_back("place-image");
        return image;
    }
    float ReadFloat(std::uint32_t address) override {
        calls.push_back("geometry");
        if (address == kConsole + 44u) return 0.0f;
        if (address == kConsole + 52u) return 64.0f;
        Check(address == kConsole + 48u, "animation y address");
        return 20.0f;
    }
    std::uint32_t SetPosition(std::uint32_t image, std::uint32_t mode,
                              float x, float y) override {
        Check(mode == 1u && y == 20.0f, "animation mode and y");
        calls.push_back("animate");
        last_animation = image;
        last_x = x;
        return 0x1234u;
    }
    std::uint32_t ActorFromEvent(std::uint32_t event) override {
        Check(event == 0xe0u, "event pointer");
        calls.push_back("event-actor");
        return 0xa1u;
    }
    std::int32_t ActorIndex(std::uint32_t actor) override {
        calls.push_back("index");
        switch (actor) {
        case 0xa1u: return 1;
        case 0xa2u: return 2;
        case 0xb1u: return 4;
        case 0xb2u: return 5;
        default: throw std::runtime_error("actor index");
        }
    }
    float ActorPositionProgress(std::uint32_t actor) override {
        Check(actor == 0xa1u || actor == 0xb1u, "active actor progress");
        calls.push_back("progress");
        return 32.0f;
    }
    bool ActorState(std::uint32_t actor) override {
        calls.push_back("actor-state");
        return actor == 0xa2u || actor == 0xb2u;
    }
    std::string ActorImageName(std::uint32_t actor) override {
        calls.push_back("name");
        return actor == 0xa1u ? "role" : "monster";
    }
    std::unique_ptr<pal4::ICombatConsoleActorIterator> RoleActors() override {
        calls.push_back("roles");
        return std::make_unique<Iterator>(calls, "roles", roles);
    }
    std::unique_ptr<pal4::ICombatConsoleActorIterator> MonsterActors() override {
        calls.push_back("monsters");
        return std::make_unique<Iterator>(calls, "monsters", monsters);
    }
    std::unique_ptr<pal4::ICombatConsoleActorIterator>
    CountMonsterActors() override {
        calls.push_back("count-actors");
        return std::make_unique<Iterator>(calls, "count", count_monsters);
    }
    bool ActorStateZero(std::uint32_t actor) override {
        calls.push_back("state-zero");
        return actor == 0xc1u;
    }
    bool ActorReady(std::uint32_t actor) override {
        calls.push_back("ready");
        return actor == 0xc2u;
    }
    void SetCombatCountText(std::uint32_t window,
                            std::string_view text) override {
        Check(window == 0x7900u && text == "2", "count text and window");
        calls.push_back("count");
    }
};

std::size_t First(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(), call);
    Check(found != runtime.calls.end(), "missing expected operation");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void HideAllAndThunk() {
    Runtime runtime;
    pal4::CombatConsoleWindow::HideAllFromEvent(kConsole, 0xdeadbeefu,
                                                runtime);
    Check(runtime.visibility.size() == 9u, "nine images hidden");
    for (std::uint32_t index = 0u; index < 9u; ++index)
        Check(runtime.visibility[index] ==
                  std::pair{0x8000u + index * 0x100u, false},
              "image hiding follows role then monster slot order");
    Check(runtime.calls.size() == 18u &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "disable") == 0,
          "hide-all does not change image flags");
}

void EventAnimationAdapters() {
    Runtime roles;
    Check(pal4::CombatConsoleWindow::AnimateRoleEvent(
              kConsole, 0xe0u, roles) == 0x1234u &&
              roles.last_animation == 0x8100u && roles.last_x == 32.0f,
          "role event animates actor slot");
    Check(roles.calls[0] == "event-actor" &&
              roles.calls[1] == "progress" && roles.calls[2] == "index" &&
              roles.calls[3] == "state",
          "event wrapper reads progress before actor index");

    Runtime monsters;
    monsters.state = 1u;
    Check(pal4::CombatConsoleWindow::AnimateMonsterEvent(
              kConsole, 0xe0u, monsters) == 0x1234u &&
              monsters.last_animation == 0x8400u,
          "monster event uses second slot family");
}

void RefreshBothCollections() {
    Runtime runtime;
    runtime.state = 2u;
    pal4::CombatConsoleWindow::RefreshFromEvent(kConsole, 0xe0u, runtime);
    Check(runtime.visibility.size() == 13u,
          "refresh hides nine slots then shows/hides four actors");
    Check(runtime.visibility[9] == std::pair{0x8100u, true} &&
              runtime.visibility[10] == std::pair{0x8200u, false} &&
              runtime.visibility[11] == std::pair{0x8700u, true} &&
              runtime.visibility[12] == std::pair{0x8800u, false},
          "role and monster actors use their own image families");
    Check(runtime.words.at(0x8100u + 280u) == 1u &&
              runtime.words.at(0x8200u + 280u) == 0u &&
              runtime.words.at(0x8700u + 280u) == 1u &&
              runtime.words.at(0x8800u + 280u) == 0u,
          "only active images are enabled");
    Check(runtime.last_image == 0x8700u &&
              runtime.last_animation == 0x8700u && runtime.last_x == 32.0f,
          "second family image setup and animation");
    Check(First(runtime, "name") < First(runtime, "index") &&
              First(runtime, "roles") < First(runtime, "monsters") &&
              First(runtime, "monsters") < First(runtime, "count-actors") &&
              First(runtime, "count") < First(runtime, "destroy-count") &&
              First(runtime, "destroy-count") <
                  First(runtime, "destroy-monsters") &&
              First(runtime, "destroy-monsters") <
                  First(runtime, "destroy-roles"),
          "image name precedes index and iterators release in reverse order");
    Check(std::count(runtime.calls.begin(), runtime.calls.end(), "index") == 6,
          "live actors read index twice and hidden actors once");
    Check(std::count(runtime.calls.begin(), runtime.calls.end(),
                     "event-actor") == 0,
          "refresh thunk ignores its event payload");
}

void EmptyCollections() {
    Runtime runtime;
    runtime.roles.clear();
    runtime.monsters.clear();
    pal4::CombatConsoleWindow::Refresh(kConsole, runtime);
    Check(runtime.visibility.size() == 9u &&
              First(runtime, "count") < First(runtime, "destroy-monsters"),
          "empty collections still hide images and update count");
}

void CountPredicates() {
    Runtime runtime;
    pal4::CombatConsoleWindow::UpdateCombatCount(kConsole, runtime);
    Check(std::count(runtime.calls.begin(), runtime.calls.end(),
                     "state-zero") == 3 &&
              std::count(runtime.calls.begin(), runtime.calls.end(),
                         "ready") == 2,
          "ready predicate is short-circuited for state-zero actors");
    Check(First(runtime, "count") < First(runtime, "destroy-count"),
          "count text is set before iterator release");
}

}  // namespace

int main() {
    try {
        HideAllAndThunk();
        EventAnimationAdapters();
        RefreshBothCollections();
        EmptyCollections();
        CountPredicates();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
