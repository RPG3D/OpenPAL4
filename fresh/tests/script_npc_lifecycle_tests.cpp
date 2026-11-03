#include "pal4/script_npc_lifecycle.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptNpcLifecycleRuntime {
    std::uint32_t found_npc = 0;
    std::string expected_name = "npc";
    std::string expected_resource = "actor";
    unsigned npc_manager_calls = 0;
    std::vector<std::string> calls;

    std::uint32_t NpcManager() override {
        calls.emplace_back("npc-manager");
        return 0x6000u + npc_manager_calls++;
    }
    std::uint32_t FindNpc(std::uint32_t manager,
                          std::string_view name) override {
        Check(manager == 0x6000u && name == expected_name,
              "first NPC manager and nullable lookup name");
        calls.push_back("find:" + std::string(name));
        return found_npc;
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena-manager");
        return 0x8000u;
    }
    std::uint32_t LeaveNpcScript(std::uint32_t arena,
                                 std::uint32_t npc) override {
        Check(arena == 0x8000u && npc == found_npc,
              "delete removes found NPC from arena");
        calls.emplace_back("leave-script");
        return 0x41u;
    }
    void SetStateFlag(std::uint32_t npc,
                      std::uint32_t flag) override {
        Check(npc == 0x9000u && flag == 1u, "created NPC visible flag");
        calls.emplace_back("state-flag");
    }
    std::uint32_t EnterNpcScript(std::uint32_t arena,
                                 std::uint32_t npc) override {
        Check(arena == 0x8000u && npc == 0x9000u,
              "created NPC arena registration");
        calls.emplace_back("enter-script");
        return 0x56u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        if (address == 0xa00cu) return 0xc000u;
        if (address == 0xa000u) return 0xb000u;
        if (address == 0x9654u) return 0xa000u;
        throw std::runtime_error("unexpected record read");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0xa048u && value == 1u,
              "NPC info +0x48 is set before object creation");
        calls.emplace_back("write-info-flag");
    }
    std::uint32_t CreateOrGetInfo(
        std::uint32_t manager, std::string_view name,
        std::string_view resource, std::array<std::uint32_t, 3> position,
        std::array<std::uint32_t, 3> rotation) override {
        Check(manager == 0x6001u && name == expected_name &&
              resource == expected_resource &&
              position == std::array<std::uint32_t, 3>{
                  0x3f800000u, 0xc0000000u, 0x40400000u} &&
              rotation == std::array<std::uint32_t, 3>{0u, 0u, 0u},
              "NPC info name, actor and raw position/rotation words");
        calls.emplace_back("create-info");
        return 0xa000u;
    }
    std::uint32_t CreateAndLoadNpc(
        std::uint32_t manager, std::uint32_t name,
        std::uint32_t resource, std::uint32_t position,
        std::uint32_t rotation, std::uint32_t info,
        std::uint32_t enabled) override {
        Check(manager == 0x6002u && name == 0xb000u &&
              resource == 0xc000u && position == 0xa024u &&
              rotation == 0xa030u && info == 0xa000u && enabled == 1u,
              "NPC creation uses fields and pointers from info record");
        calls.emplace_back("create-npc");
        return 0x9000u;
    }
    void DeleteNpc(std::uint32_t manager,
                   std::uint32_t name) override {
        Check(manager == 0x6001u && name == 0xb000u,
              "NPC object deletion manager and name");
        calls.emplace_back("delete-npc");
    }
    std::uint32_t RemoveNpcInfo(std::uint32_t manager,
                                 std::uint32_t name) override {
        Check(manager == 0x6002u && name == 0xb000u,
              "NPC info deletion reacquires manager and name");
        calls.emplace_back("remove-info");
        return 0x57u;
    }
};

void ExistingNpc() {
    Runtime runtime;
    runtime.found_npc = 0x9000u;
    Check(pal4::ScriptNpcLifecycle::Create(
              "npc", "actor", {1u, 2u, 3u}, runtime) == 0x9000u &&
          runtime.calls == std::vector<std::string>{
              "npc-manager", "find:npc"},
          "existing NPC returns immediately without creation");
}

void CreateNpc() {
    Runtime runtime;
    Check(pal4::ScriptNpcLifecycle::Create(
              "npc", "actor",
              {0x3f800000u, 0xc0000000u, 0x40400000u}, runtime) == 0x56u &&
          runtime.calls == std::vector<std::string>{
              "npc-manager", "find:npc", "npc-manager", "create-info",
              "read:40972", "read:40960", "write-info-flag",
              "npc-manager", "create-npc", "state-flag",
              "arena-manager", "enter-script"},
          "new NPC creation preserves record, manager and arena order");

    Runtime empty_strings;
    empty_strings.expected_name.clear();
    empty_strings.expected_resource.clear();
    Check(pal4::ScriptNpcLifecycle::Create(
              nullptr, nullptr,
              {0x3f800000u, 0xc0000000u, 0x40400000u},
              empty_strings) == 0x56u &&
          empty_strings.calls[1] == "find:" &&
          empty_strings.calls[3] == "create-info",
          "null NPC name and resource use empty strings");
}

void DeleteNpc() {
    Runtime missing;
    missing.expected_name.clear();
    Check(pal4::ScriptNpcLifecycle::Delete(nullptr, missing) == 0u &&
          missing.calls == std::vector<std::string>{
              "npc-manager", "find:"},
          "missing NPC returns without side effects and null uses empty name");

    Runtime found;
    found.found_npc = 0x9000u;
    Check(pal4::ScriptNpcLifecycle::Delete("npc", found) == 0x57u &&
          found.calls == std::vector<std::string>{
              "npc-manager", "find:npc", "arena-manager", "leave-script",
              "read:38484", "read:40960", "npc-manager", "delete-npc",
              "read:40960", "npc-manager", "remove-info"},
          "delete removes script, object and info in original order");
}

}  // namespace

int main() {
    try {
        ExistingNpc();
        CreateNpc();
        DeleteNpc();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
