#include "pal4/monster_manager_object_processor.h"

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Event {
    std::string_view kind;
    std::uint32_t object{};
    std::uint32_t first{};
    std::uint32_t second{};
    bool operator==(const Event&) const = default;
};

struct Runtime final : pal4::MonsterManagerObjectProcessRuntimeStub {
    std::uint32_t ReadDword(std::uint32_t address) override {
        reads.push_back(address);
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        byte_reads.push_back(address);
        return bytes.at(address);
    }
    void UpdateGrid(std::uint32_t grid,
                    std::uint32_t first) override {
        events.push_back({"grid", grid, first});
    }
    void CallSlot34(std::uint32_t object,
                    std::uint32_t argument) override {
        events.push_back({"slot34", object, argument});
        if (globals != nullptr && replacement_nil != 0u) {
            globals->nil_address = replacement_nil;
        }
    }
    void CallSlot38(std::uint32_t object, std::uint32_t first,
                    std::uint32_t second) override {
        events.push_back({"slot38", object, first, second});
    }
    void ProcessCollision(std::uint32_t object, float scalar,
                          std::uint32_t argument) override {
        events.push_back({"collision", object, argument});
        collision_scalars.push_back(scalar);
    }
    void SetPursuitTarget(std::uint32_t object,
                          std::uint32_t target) override {
        events.push_back({"target", object, target});
    }
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> byte_reads;
    std::vector<std::uint32_t> reads;
    std::vector<Event> events;
    std::vector<float> collision_scalars;
    pal4::MonsterManagerTreeGlobals* globals{};
    std::uint32_t replacement_nil{};
};

constexpr std::uint32_t kManager = 0x5000u;
constexpr std::uint32_t kHeader = 0x6000u;
constexpr std::uint32_t kNil = 0x9000u;

void TestEmptyTree() {
    Runtime runtime;
    runtime.words = {{0x502cu, kHeader}, {kHeader, kHeader}};
    runtime.bytes = {{0x505cu, 0u}};
    pal4::MonsterManagerTreeGlobals globals{1u, kNil};
    Check(pal4::MonsterManagerObjectProcessor::Process(
              kManager, 0x11u, globals, runtime) == kHeader &&
              runtime.events.empty() && runtime.byte_reads.empty(),
          "basic empty-tree path returns header without callback");
    Check(pal4::MonsterManagerObjectProcessor::ProcessExtended(
              kManager, 0x22u, 0x33u, globals, runtime) == kHeader &&
              runtime.events.empty() &&
              runtime.byte_reads == std::vector<std::uint32_t>{0x505cu},
          "extended empty-tree path checks grid flag only");
}

void TestBasicActiveGateAndReturn() {
    Runtime runtime;
    constexpr std::uint32_t root = 0x7100u;
    constexpr std::uint32_t left = 0x7200u;
    constexpr std::uint32_t right = 0x7300u;
    runtime.words = {
        {0x502cu, kHeader}, {kHeader, left},
        {kHeader + 4u, root}, {kHeader + 8u, right},
        {root, left}, {root + 4u, kHeader},
        {root + 8u, right}, {root + 0x1cu, 0xa100u},
        {left + 4u, root}, {left + 8u, kNil},
        {left + 0x1cu, 0xa200u},
        {right, kNil}, {right + 4u, root},
        {right + 8u, kNil}, {right + 0x1cu, 0xa300u},
        {0xa100u + 0x1f0u, 0u},
        {0xa200u + 0x1f0u, 1u},
        {0xa300u + 0x1f0u, 0x80000000u}};
    pal4::MonsterManagerTreeGlobals globals{1u, kNil};
    Check(pal4::MonsterManagerObjectProcessor::Process(
              kManager, 0x1234u, globals, runtime) == kHeader,
          "basic update returns last traversal register");
    Check(runtime.events == std::vector<Event>{
              {"slot34", 0xa200u, 0x1234u},
              {"slot34", 0xa300u, 0x1234u}},
          "only active objects receive the basic virtual call");
}

void TestExtendedGridAndTwoArguments() {
    Runtime runtime;
    constexpr std::uint32_t node = 0x7100u;
    runtime.words = {
        {0x502cu, kHeader}, {kHeader, node},
        {kHeader + 4u, node}, {kHeader + 8u, node},
        {node + 4u, kHeader}, {node + 8u, kNil},
        {node + 0x1cu, 0xa000u},
        {0xa000u + 0x1f0u, 1u}};
    runtime.bytes = {{0x505cu, 1u}};
    pal4::MonsterManagerTreeGlobals globals{1u, kNil};
    Check(pal4::MonsterManagerObjectProcessor::ProcessExtended(
              kManager, 0x12u, 0x34u, globals, runtime) == node &&
              runtime.events == std::vector<Event>{
                  {"grid", 0x5038u, 0x12u},
                  {"slot38", 0xa000u, 0x12u, 0x34u}},
          "extended update refreshes grid before the two-arg virtual call");
    runtime.events.clear();
    runtime.bytes[0x505cu] = 0u;
    Check(pal4::MonsterManagerObjectProcessor::ProcessExtended(
              kManager, 0x56u, 0x78u, globals, runtime) == node &&
              runtime.events == std::vector<Event>{
                  {"slot38", 0xa000u, 0x56u, 0x78u}},
          "disabled grid flag skips grid update without skipping objects");
}

void TestNilReloadAfterVirtualCall() {
    Runtime runtime;
    constexpr std::uint32_t node = 0x7100u;
    constexpr std::uint32_t new_nil = 0x9100u;
    runtime.words = {
        {0x502cu, kHeader}, {kHeader, node},
        {kHeader + 4u, node}, {kHeader + 8u, node},
        {node + 4u, kHeader}, {node + 8u, new_nil},
        {node + 0x1cu, 0xa000u},
        {0xa000u + 0x1f0u, 1u}};
    pal4::MonsterManagerTreeGlobals globals{1u, kNil};
    runtime.globals = &globals;
    runtime.replacement_nil = new_nil;
    Check(pal4::MonsterManagerObjectProcessor::Process(
              kManager, 0x55u, globals, runtime) == node &&
              globals.nil_address == new_nil &&
              runtime.events == std::vector<Event>{
                  {"slot34", 0xa000u, 0x55u}},
          "virtual call may change shared nil before successor lookup");
}

void TestCollisionAndTargetGates() {
    Runtime runtime;
    constexpr std::uint32_t root = 0x7100u;
    constexpr std::uint32_t left = 0x7200u;
    constexpr std::uint32_t right = 0x7300u;
    runtime.words = {
        {0x502cu, kHeader}, {kHeader, left},
        {kHeader + 4u, root}, {kHeader + 8u, right},
        {root, left}, {root + 4u, kHeader},
        {root + 8u, right}, {root + 0x1cu, 0xa100u},
        {left + 4u, root}, {left + 8u, kNil},
        {left + 0x1cu, 0xa200u},
        {right, kNil}, {right + 4u, root},
        {right + 8u, kNil}, {right + 0x1cu, 0xa300u},
        {0xa100u + 0x1f0u, 1u},
        {0xa200u + 0x1f0u, 0u},
        {0xa300u + 0x1f0u, 1u},
        {0xa100u + 0x730u, 1u},
        {0xa300u + 0x730u, 2u}};
    pal4::MonsterManagerTreeGlobals globals{1u, kNil};
    Check(pal4::MonsterManagerObjectProcessor::ProcessCollisions(
              kManager, 0.625f, 0x1234u, globals, runtime) == kHeader &&
              runtime.events == std::vector<Event>{
                  {"collision", 0xa300u, 0x1234u}} &&
              runtime.collision_scalars == std::vector<float>{0.625f},
          "collision requires active flag and state not equal to one");
    const auto count_reads = [&runtime](std::uint32_t address) {
        std::size_t count{};
        for (const auto read : runtime.reads) {
            if (read == address) ++count;
        }
        return count;
    };
    Check(count_reads(0xa100u + 0x730u) == 1u &&
              count_reads(0xa200u + 0x730u) == 0u &&
              count_reads(0xa300u + 0x730u) == 1u,
          "collision gate reads state once for active objects only");
    runtime.events.clear();
    Check(pal4::MonsterManagerObjectProcessor::SetPursuitTargetAll(
              kManager, 0x5566u, globals, runtime) == kHeader &&
              runtime.events == std::vector<Event>{
                  {"target", 0xa100u, 0x5566u},
                  {"target", 0xa300u, 0x5566u}},
          "target update reaches active objects regardless of collision state");
}

}  // namespace

int main() {
    TestEmptyTree();
    TestBasicActiveGateAndReturn();
    TestExtendedGridAndTwoArguments();
    TestNilReloadAfterVirtualCall();
    TestCollisionAndTargetGates();
}
