#include "pal4/script_party_magic_query.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptPartyMagicQueryRuntime {
    std::array<bool, 4> present{true, true, true, true};
    std::array<bool, 4> in_party{true, true, true, true};
    std::array<bool, 4> has_magic{false, false, false, false};
    std::vector<std::string> calls;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x1000u;
    }
    std::uint32_t PlayerByIndex(std::uint32_t manager,
                                std::uint32_t index) override {
        Check(manager == 0x1000u && index < 4u, "indexed party lookup");
        calls.emplace_back("player:" + std::to_string(index));
        return present[index] ? 0x2000u + index * 0x1000u : 0u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address >= 0x2b08u && (address - 0x2b08u) % 0x1000u == 0u,
              "party membership offset");
        const auto index = (address - 0x2b08u) / 0x1000u;
        Check(index < 4u, "party membership index");
        calls.emplace_back("member:" + std::to_string(index));
        return in_party[index] ? 2u : 0u;
    }
    bool HasAnyValues(std::uint32_t collection) override {
        Check(collection >= 0x2938u &&
                  (collection - 0x2938u) % 0x1000u == 0u,
              "magic collection offset");
        const auto index = (collection - 0x2938u) / 0x1000u;
        Check(index < 4u, "magic collection index");
        calls.emplace_back("magic:" + std::to_string(index));
        return has_magic[index];
    }
};

void ScanBranches() {
    Runtime none;
    Check(!pal4::ScriptPartyMagicQuery::AnyMastered(none) &&
              none.calls.size() == 16u,
          "scans four players when none has magic");
    Runtime gated;
    gated.present[0] = false;
    gated.in_party[1] = false;
    gated.has_magic[0] = true;
    gated.has_magic[1] = true;
    gated.has_magic[2] = true;
    Check(pal4::ScriptPartyMagicQuery::AnyMastered(gated) &&
              gated.calls == std::vector<std::string>{
                  "manager", "player:0", "manager", "player:1", "member:1",
                  "manager", "player:2", "member:2", "magic:2"},
          "missing and non-party players skip magic check; first match stops");
    Runtime last;
    last.has_magic[3] = true;
    Check(pal4::ScriptPartyMagicQuery::AnyMastered(last) &&
              last.calls.back() == "magic:3" && last.calls.size() == 16u,
          "fourth player is included");
}

}  // namespace

int main() {
    try {
        ScanBranches();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
