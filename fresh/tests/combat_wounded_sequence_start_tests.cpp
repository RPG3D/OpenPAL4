#include "pal4/combat_wounded_sequence_start.h"
#include "pal4/combat_wounded_sequence_task.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t kTask = 0x1000u;
constexpr std::uint32_t kActor = 0x2000u;

struct Addition {
    std::uint32_t container;
    std::uint32_t child;
    std::uint32_t text;
    std::array<std::uint32_t, 3> timing;
};

struct Runtime final : pal4::IWoundedSequenceStartRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {kTask + 44u, kActor},
        {0x8f960cu, 0x9000u}, {0x8f9770u, 0x9100u}};
    std::vector<std::string> calls;
    std::vector<std::uint32_t> factories;
    std::vector<std::uint32_t> children{0x3000u, 0x3100u};
    std::vector<Addition> additions;
    std::size_t child_index = 0u;
    std::uint32_t actor_state = 2u;
    std::uint32_t set_state_count = 0u;
    std::uint32_t assigned_header = 0u;
    std::uint32_t assigned_source = 0u;
    bool throw_on_sound = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write");
    }
    std::uint32_t ActorVirtual168(std::uint32_t actor) override {
        Check(actor == kActor, "actor +168 argument");
        calls.emplace_back("state168");
        return actor_state;
    }
    void CheckActorStateGroup(std::uint32_t actor) override {
        Check(actor == kActor, "actor state group argument");
        calls.emplace_back("state_group");
    }
    void SetActorState(std::uint32_t actor,
                       std::uint32_t state,
                       std::uint32_t mode) override {
        Check(actor == kActor && state == 3u && mode == 1u,
              "wounded state transition");
        ++set_state_count;
        calls.emplace_back("set_state");
    }
    pal4::WoundedSoundArgument ActorVirtual456(
        std::uint32_t actor, std::uint32_t mode) override {
        Check(actor == kActor && mode == 0u, "sound argument query");
        calls.emplace_back("sound_argument");
        return {0x9200u, 0x9300u};
    }
    void PlaySoundEffect(std::uint32_t argument,
                         std::uint32_t mode) override {
        Check(argument == 0x9200u && mode == 1u,
              "combat sound argument");
        calls.emplace_back("play_sound");
        if (throw_on_sound) throw std::runtime_error("sound failed");
    }
    void ReleaseSoundText(std::uint32_t temporary_text,
                          std::uint8_t flag) override {
        Check(temporary_text == 0x9300u && flag == 1u,
              "sound text release arguments");
        calls.emplace_back("release_sound_text");
    }
    std::uint32_t CreateChildTask(std::uint32_t vtable) override {
        factories.push_back(vtable);
        calls.emplace_back("create");
        Check(child_index < children.size(), "unexpected child");
        return children[child_index++];
    }
    std::uint32_t AddTaskAndInitialize(
        std::uint32_t container, std::uint32_t child,
        std::uint32_t text,
        const std::array<std::uint32_t, 3>& timing) override {
        additions.push_back({container, child, text, timing});
        calls.emplace_back("add");
        if (additions.size() == 1u) words[kTask + 44u] = 0x2100u;
        if (additions.size() == 2u) words[kTask + 44u] = 0x2200u;
        return 0x9400u + static_cast<std::uint32_t>(additions.size());
    }
    void AssignText(std::uint32_t header,
                    std::uint32_t source,
                    std::uint32_t start,
                    std::uint32_t count) override {
        Check(start == 0u && count == 0xffffffffu,
              "message text range");
        assigned_header = header;
        assigned_source = source;
        calls.emplace_back("assign_text");
    }
};

void StartWithTwoChildren() {
    Runtime r;
    r.actor_state = 3u;
    Check(pal4::WoundedSequenceTask::Start(kTask, r) == 0x9402u,
          "second add result");
    Check(r.set_state_count == 1u &&
              r.factories == std::vector<std::uint32_t>{
                  0x845d14u, 0x845a80u},
          "wounded state and factories");
    Check(r.additions.size() == 2u &&
              r.additions[0].container == kTask + 4u &&
              r.additions[0].child == 0x3000u &&
              r.additions[0].text == 0x9000u &&
              r.additions[0].timing ==
                  std::array<std::uint32_t, 3>{1u, 0u, 0u} &&
              r.additions[1].child == 0x3100u &&
              r.additions[1].text == 0x9100u,
          "wounded child attachments");
    Check(r.words[0x3000u + 60u] == 0x2100u &&
              r.words[0x3100u + 60u] == 0x2200u &&
              r.assigned_header == 0x3000u + 64u &&
              r.assigned_source == 0x8fa31cu,
          "wounded fields and actor rereads");
    Check(r.calls[1] == "state168" &&
              r.calls[3] == "state_group" &&
              r.calls[5] == "set_state" &&
              r.calls[7] == "sound_argument" &&
              r.calls[8] == "play_sound" &&
              r.calls[9] == "release_sound_text",
          "state and sound order");
}

void StateAndTextFallback() {
    Runtime r;
    r.words[0x8f960cu] = 0u;
    r.words[0x8f9770u] = 0u;
    Check(pal4::WoundedSequenceTask::Start(kTask, r) == 0x9402u &&
              r.set_state_count == 0u,
          "non-three actor state");
    Check(r.additions[0].text == 0x841964u &&
              r.additions[1].text == 0x841964u,
          "text fallbacks");
}

void NullChildren() {
    Runtime first;
    first.children = {0u};
    Check(pal4::WoundedSequenceTask::Start(kTask, first) == 0u &&
              first.additions.empty() &&
              first.factories.size() == 1u,
          "null first child");
    Runtime second;
    second.children = {0x3000u, 0u};
    Check(pal4::WoundedSequenceTask::Start(kTask, second) == 0u &&
              second.additions.size() == 1u &&
              second.factories.size() == 2u,
          "null second child");
}

void SoundFailureCleansTemporaryText() {
    Runtime r;
    r.throw_on_sound = true;
    bool caught = false;
    try {
        (void)pal4::WoundedSequenceTask::Start(kTask, r);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && r.factories.empty() &&
              r.calls.back() == "release_sound_text",
          "sound failure cleanup");
}

}  // namespace

int main() {
    try {
        StartWithTwoChildren();
        StateAndTextFallback();
        NullChildren();
        SoundFailureCleansTemporaryText();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
