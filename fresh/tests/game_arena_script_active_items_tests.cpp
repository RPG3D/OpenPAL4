#include "pal4/game_arena_script_active_items.h"

#include <cctype>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using Items = pal4::GameArenaScriptActiveItems;
using Op = pal4::GameArenaScriptActiveItemCall;

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Invocation {
    Op operation;
    std::vector<std::uint32_t> arguments;
    bool operator==(const Invocation&) const = default;
};

struct Runtime final : pal4::IGameArenaScriptActiveItemRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::string> names;
    std::vector<Invocation> calls;
    std::vector<std::uint32_t> freed;
    std::vector<std::uint32_t> allocations;
    std::uint32_t next = 0x9000u;
    std::uint32_t comparisons = 0u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        words[address] = value;
    }
    bool NamesEqualIgnoreCase(const std::uint32_t left,
                              const std::uint32_t right) override {
        ++comparisons;
        auto a = names[left];
        auto b = names[right];
        for (char& ch : a)
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        for (char& ch : b)
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        return a == b;
    }
    std::uint32_t Allocate(const std::uint32_t bytes) override {
        allocations.push_back(bytes);
        const auto result = next;
        next += bytes + 0x100u;
        return result;
    }
    void Free(const std::uint32_t address) override {
        freed.push_back(address);
    }
    void Call(const Op operation,
              const std::initializer_list<std::uint32_t> arguments) override {
        calls.push_back({operation, arguments});
        if (operation == Op::SetStateFlag) words[arguments.begin()[0] + 0x1f0u] = 0u;
    }
};

void NameAndFlagGates() {
    Runtime runtime;
    const auto script = 0x5000u;
    const auto object = 0x6000u;
    Check(Items::AddSceneObject(script, 0u, 7u, runtime) == 0u,
          "null scene object rejected");
    Check(runtime.allocations.empty(), "null object has no allocation");
    runtime.words[object + 4u] = 0x7000u;
    runtime.words[object + 0x184u] = 0u;
    Check(Items::AddSceneObject(script, object, 7u, runtime) == 0u,
          "scene object requires active flag");
    runtime.words[object + 0x184u] = 1u;
    Check(Items::AddSceneObject(script, object, 7u, runtime) == 1u,
          "active scene object inserted");
    runtime.words[0x6104u] = 0x7100u;
    runtime.words[0x6284u] = 1u;
    runtime.names[0x7000u] = "Entrance";
    runtime.names[0x7100u] = "eNtRaNcE";
    Check(Items::AddSceneObject(script, 0x6100u, 8u, runtime) == 0u,
          "case-insensitive duplicate rejected");
    Check(runtime.comparisons == 1u && runtime.words[script + 0x270u] == 0x900cu,
          "duplicate does not grow the array");
}

void GrowthAndCapacityReuse() {
    Runtime runtime;
    const auto script = 0x5000u;
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto object = 0x6000u + index * 0x200u;
        const auto name = 0x7000u + index * 0x100u;
        runtime.words[object + 4u] = name;
        runtime.words[object + 0x184u] = 1u;
        runtime.names[name] = "item" + std::to_string(index);
        Check(Items::AddSceneObject(script, object, 0x40u + index, runtime) == 1u,
              "scene item insertion");
    }
    Check(runtime.allocations == std::vector<std::uint32_t>{12u, 24u, 48u},
          "capacity grows by max(size, one)");
    Check(runtime.freed == std::vector<std::uint32_t>{0u, 0x9000u, 0x910cu},
          "old allocation is freed on growth");
    const auto begin = runtime.words[script + 0x26cu];
    Check(runtime.words[script + 0x270u] == begin + 48u &&
              runtime.words[script + 0x274u] == begin + 48u,
          "used and capacity ends reflect four entries");
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        const auto slot = begin + index * 12u;
        Check(runtime.words[slot] == 0x6000u + index * 0x200u &&
                  runtime.words[slot + 4u] == 0x40u + index &&
                  runtime.words[slot + 8u] == 0u,
              "all three record words survive growth and reuse");
    }
}

void AnimatedObject() {
    Runtime runtime;
    const auto script = 0x5000u;
    const auto object = 0x6000u;
    runtime.words[object + 4u] = 0x7000u;
    runtime.words[object + 20u] = 0x8000u;
    runtime.words[object + 0x184u] = 1u;
    Check(Items::AddAnimatedObject(script, object, 0xabcdu, runtime) == 0u &&
              runtime.calls.empty(), "animated path requires zero flag");
    runtime.words[object + 0x184u] = 0u;
    Check(Items::AddAnimatedObject(script, object, 0xabcdu, runtime) == 1u,
          "animated object inserted");
    Check(runtime.calls == std::vector<Invocation>{
              {Op::SetVirtual156, {object, 1u}},
              {Op::SetMaterialFlags, {0x8000u, 0u}},
              {Op::AddAnimatedObject, {script, object}}},
          "animated side effects and arguments preserve IDA order");
    const auto slot = runtime.words[script + 0x27cu];
    Check(runtime.words[slot] == object &&
              runtime.words[slot + 4u] == 0xabcdu &&
              runtime.words[slot + 8u] == 0u,
          "animated record stores object, context and zero");
    Check(Items::AddAnimatedObject(script, object, 0u, runtime) == 0u &&
              runtime.calls.size() == 3u,
          "duplicate animated object has no side effects");
}

void Character() {
    Runtime runtime;
    const auto script = 0x5000u;
    const auto object = 0x6000u;
    runtime.words[object + 4u] = 0x7000u;
    Check(Items::AddCharacter(script, object, 3u, runtime) == 0u,
          "character requires visible flag");
    runtime.words[object + 0x1f0u] = 1u;
    Check(Items::AddCharacter(script, object, 3u, runtime) == 1u,
          "character insertion succeeds");
    Check(runtime.calls == std::vector<Invocation>{{Op::SetStateFlag, {object, 0u}}} &&
              runtime.words[object + 0x1f0u] == 0u,
          "state flag cleared before character record is active");
    const auto slot = runtime.words[script + 0x28cu];
    Check(runtime.words[slot] == object && runtime.words[slot + 4u] == 3u &&
              runtime.words[slot + 8u] == 0u,
          "character array uses distinct offset and exact record");
    pal4::GameArenaScriptActiveItemRuntimeStub stub;
    Check(Items::AddCharacter(script, 0u, 3u, stub) == 0u,
          "native boundary stub links");
}

void RecordCopies() {
    Runtime runtime;
    runtime.words[0x7000u] = 11u;
    runtime.words[0x7004u] = 12u;
    runtime.words[0x7008u] = 13u;
    runtime.words[0x700cu] = 21u;
    runtime.words[0x7010u] = 22u;
    runtime.words[0x7014u] = 23u;
    Check(Items::CopyRange(0x7000u, 0x7018u, 0x8000u, runtime) == 0x8018u &&
              runtime.words[0x8000u] == 11u &&
              runtime.words[0x8004u] == 12u &&
              runtime.words[0x8008u] == 13u &&
              runtime.words[0x800cu] == 21u &&
              runtime.words[0x8010u] == 22u &&
              runtime.words[0x8014u] == 23u,
          "range helper copies both 12-byte records and returns end");
    Check(Items::CopyRange(0x7000u, 0x7000u, 0x8200u, runtime) == 0x8200u,
          "empty range returns original destination");
    Check(Items::CopyRange(0x7000u, 0x700cu, 0u, runtime) == 12u,
          "null range destination skips the copy and still advances");
    Check(Items::Fill(0x8100u, 3u, 0x7000u, runtime) == 0x8124u &&
              runtime.words[0x8100u] == 11u &&
              runtime.words[0x810cu] == 11u &&
              runtime.words[0x8118u] == 11u &&
              runtime.words[0x8120u] == 13u,
          "fill helper repeats all three words and returns end");
    Check(Items::Fill(0x8200u, 0u, 0x7000u, runtime) == 0u &&
              Items::Fill(0u, 2u, 0x7000u, runtime) == 24u,
          "zero count and null destination match helper return rules");
    Check(Items::CopyRange(0x7000u, 0x700cu, 0x7004u, runtime) == 0x7010u &&
              runtime.words[0x7004u] == 11u &&
              runtime.words[0x7008u] == 11u &&
              runtime.words[0x700cu] == 11u,
          "overlapping copy reads each word after the preceding write");
}

}  // namespace

int main() {
    NameAndFlagGates();
    GrowthAndCapacityReuse();
    AnimatedObject();
    Character();
    RecordCopies();
}
