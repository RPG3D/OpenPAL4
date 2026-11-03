#include "pal4/script_puzzle_game.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

class Runtime final : public pal4::IScriptPuzzleRuntime {
public:
    pal4::PuzzleWaitState state{};
    std::vector<std::string> calls;
    bool allocation_fails = false;
    bool attribute_throws = false;
    std::int32_t mode = -1;
    std::int32_t type = -1;

    void ClearPuzzleUiFlag() override {
        calls.emplace_back("clear ui");
    }
    std::uint32_t CreateDocument() override {
        calls.emplace_back("create document");
        return 0x4000u;
    }
    void UpsertIntAttribute(std::uint32_t document,
                            std::string_view key,
                            std::int32_t value) override {
        Check(document == 0x4000u, "puzzle document handle");
        calls.emplace_back("attribute:" + std::string(key));
        if (attribute_throws && key == "Type")
            throw std::runtime_error("XML attribute failure");
        if (key == "Mode") mode = value;
        else if (key == "Type") type = value;
        else throw std::runtime_error("unexpected puzzle attribute");
    }
    void EnterState(std::uint32_t state_id,
                    std::uint32_t document) override {
        Check(state_id == 8u && document == 0x4000u,
              "puzzle state transition");
        calls.emplace_back("enter state");
    }
    pal4::PuzzleWaitState* AllocateWait(std::uint32_t bytes) override {
        Check(bytes == 0x20u, "puzzle wait allocation size");
        calls.emplace_back("allocate");
        return allocation_fails ? nullptr : &state;
    }
    std::uint32_t ScriptExecutor() override {
        calls.emplace_back("executor");
        return 0x6000u;
    }
    void ConstructWait(pal4::PuzzleWaitState& handler,
                       std::uint8_t wait_mode,
                       std::uint32_t queue) override {
        Check(&handler == &state && wait_mode == 1u &&
                  queue == 0x6054u, "puzzle wait constructor");
        calls.emplace_back("construct");
    }
    void FlushWait(pal4::PuzzleWaitState& handler) override {
        Check(&handler == &state && handler.vtable_address == 0x84960cu,
              "puzzle handler vtable before flush");
        calls.emplace_back("flush");
    }
    void AddHandler(std::uint32_t queue,
                    pal4::PuzzleWaitState* handler) override {
        Check(queue == 0x6054u &&
                  handler == (allocation_fails ? nullptr : &state),
              "puzzle queue handler");
        calls.emplace_back("add");
    }
    std::uint32_t DestroyDocument(std::uint32_t document) override {
        Check(document == 0x4000u, "destroy puzzle document");
        calls.emplace_back("destroy document");
        return 0x1234u;
    }
};

void StartCases() {
    Runtime puzzle;
    pal4::ScriptPuzzleGame game(puzzle);
    Check(game.StartPuzzle(17) == 0x1234u &&
              puzzle.mode == 0 && puzzle.type == 17 &&
              puzzle.calls == std::vector<std::string>{
                  "clear ui", "create document", "attribute:Mode",
                  "attribute:Type", "enter state", "allocate", "executor",
                  "construct", "flush", "executor", "add",
                  "destroy document"},
          "ordinary puzzle enters state eight and queues shared wait");

    Runtime jigsaw;
    pal4::ScriptPuzzleGame jigsaw_game(jigsaw);
    Check(jigsaw_game.StartJigsaw(-3) == 0x1234u &&
              jigsaw.mode == 1 && jigsaw.type == -3,
          "jigsaw preserves signed type and mode one");

    Runtime failed;
    failed.allocation_fails = true;
    pal4::ScriptPuzzleGame failed_game(failed);
    Check(failed_game.StartPuzzle(0) == 0x1234u &&
              failed.calls == std::vector<std::string>{
                  "clear ui", "create document", "attribute:Mode",
                  "attribute:Type", "enter state", "allocate",
                  "executor", "add", "destroy document"},
          "failed wait allocation still queues null and destroys XML");

    Runtime throws;
    throws.attribute_throws = true;
    pal4::ScriptPuzzleGame throws_game(throws);
    bool caught = false;
    try {
        static_cast<void>(throws_game.StartPuzzle(5));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && throws.calls == std::vector<std::string>{
              "clear ui", "create document", "attribute:Mode",
              "attribute:Type", "destroy document"},
          "XML failure releases temporary document");
}

}  // namespace

int main() {
    try {
        StartCases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
