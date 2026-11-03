#include "pal4/game_database_data.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Call {
    std::string name;
    std::uint32_t first = 0u;
    std::uint32_t second = 0u;
    std::uint32_t third = 0u;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::IGameDatabaseDataRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1144u, 0x2010u}, {0x1148u, 0x3010u}};
    std::vector<Call> calls;
    std::vector<std::uint8_t> load_results{1u, 1u};
    std::size_t next_load = 0u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.push_back({"read", address});
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        calls.push_back({"write", address, value});
        words[address] = value;
    }
    void ResetManager(const std::uint32_t manager) override {
        calls.push_back({"reset", manager});
    }
    void CallHelper(const std::uint32_t helper,
                    const std::uint32_t component) override {
        calls.push_back({"helper", helper, component});
    }
    std::uint8_t LoadDatabase(const std::uint32_t manager,
                              const std::uint32_t path,
                              const std::uint32_t callback_table) override {
        calls.push_back({"load", manager, path, callback_table});
        return load_results.at(next_load++);
    }
    std::uint32_t FindStringValue(const std::uint32_t manager,
                                  const std::uint32_t text) override {
        calls.push_back({"find", manager, text});
        return 0x55aau;
    }
};

constexpr std::array<std::pair<std::uint32_t, std::uint32_t>, 11> kRegister{{
    {0u, 0x4e56a0u}, {12u, 0x4f3b40u}, {32u, 0x4e99a0u},
    {52u, 0x5013a0u}, {88u, 0x4d5400u}, {128u, 0x4e2990u},
    {140u, 0x4db190u}, {148u, 0x4fae20u}, {164u, 0x4deb00u},
    {172u, 0x4cbef0u}, {184u, 0x4cfc90u},
}};

constexpr std::array<std::pair<std::uint32_t, std::uint32_t>, 11> kVerify{{
    {0u, 0x4e56c0u}, {12u, 0x4f3b80u}, {32u, 0x4e99e0u},
    {52u, 0x501440u}, {88u, 0x4d5430u}, {128u, 0x4e29b0u},
    {140u, 0x4db1b0u}, {148u, 0x4fae60u}, {164u, 0x4deb20u},
    {172u, 0x4cbf20u}, {184u, 0x4cfcd0u},
}};

void CheckHelpers(const std::vector<Call>& calls, const std::size_t start,
                  const auto& expected) {
    for (std::size_t i = 0u; i < expected.size(); ++i)
        Check(calls.at(start + i) ==
                  Call{"helper", expected[i].second,
                       0x1000u + expected[i].first},
              "GameDB table helper order and component offset");
}

void PrimaryLoadRegistersAndVerifiesAllTables() {
    Runtime runtime;
    Check(pal4::GameDatabaseData::InitializeFromDatabase(
              0x1000u, 0x9000u, runtime) == 1u,
          "successful primary load returns one");
    Check(runtime.calls.at(0) == Call{"read", 0x1144u} &&
              runtime.calls.at(1) == Call{"reset", 0x2010u},
          "primary load resets first manager before table registration");
    CheckHelpers(runtime.calls, 2u, kRegister);
    Check(runtime.calls.at(13) == Call{"read", 0x1144u} &&
              runtime.calls.at(14) ==
              Call{"load", 0x2010u, 0x9000u, 0x84488cu},
          "primary load passes the input path and callback table");
    CheckHelpers(runtime.calls, 15u, kVerify);
    Check(runtime.calls.at(26) == Call{"read", 0x1148u} &&
              runtime.calls.at(27) == Call{"reset", 0x3010u} &&
              runtime.calls.at(28) ==
                  Call{"helper", 0x4eef50u, 0x106cu} &&
              runtime.calls.at(29) == Call{"read", 0x1148u} &&
              runtime.calls.at(30) ==
                  Call{"load", 0x3010u, 0x8b45a8u, 0x84488cu} &&
              runtime.calls.at(31) ==
                  Call{"helper", 0x4eef80u, 0x106cu} &&
              runtime.calls.size() == 32u,
          "primary load initializes shop data after all verification");
}

void LoadFailuresRespectVerificationGates() {
    Runtime primary_failure;
    primary_failure.load_results = {0u};
    Check(pal4::GameDatabaseData::InitializeFromDatabase(
              0x1000u, 0x9000u, primary_failure) == 0u &&
              primary_failure.calls.size() == 15u,
          "failed primary load skips verification and shop data");
    CheckHelpers(primary_failure.calls, 2u, kRegister);

    Runtime shop_failure;
    shop_failure.load_results = {1u, 0u};
    Check(pal4::GameDatabaseData::InitializeFromDatabase(
              0x1000u, 0x9000u, shop_failure) == 1u &&
              shop_failure.calls.size() == 31u &&
              shop_failure.calls.back() ==
                  Call{"load", 0x3010u, 0x8b45a8u, 0x84488cu},
          "primary load succeeds independently of the later shop load");

    Runtime shop_only;
    shop_only.load_results = {2u};
    Check(pal4::GameDatabaseData::InitializeShopData(
              0x1000u, 0x7777u, shop_only) == 1u &&
              shop_only.calls == std::vector<Call>{
                  {"read", 0x1148u}, {"reset", 0x3010u},
                  {"helper", 0x4eef50u, 0x106cu},
                  {"read", 0x1148u},
                  {"load", 0x3010u, 0x7777u, 0x84488cu},
                  {"helper", 0x4eef80u, 0x106cu}},
          "standalone shop load normalizes a nonzero result to one");
}

void CleanupAndStringLookupUseTheFirstManager() {
    Runtime runtime;
    Check(pal4::GameDatabaseData::Cleanup(0x1000u, runtime) == 0u &&
              runtime.calls == std::vector<Call>{
                  {"read", 0x1144u}, {"reset", 0x2010u},
                  {"read", 0x1148u}, {"reset", 0x3010u},
                  {"write", 0x1070u, 0u}, {"write", 0x1074u, 0u},
                  {"write", 0x1078u, 0u}, {"write", 0x107cu, 0u}},
          "GameDB cleanup resets managers then clears four shop words");
    runtime.calls.clear();
    Check(pal4::GameDatabaseData::FindStringValue(
              0x1000u, 0x9000u, runtime) == 0x55aau &&
              runtime.calls == std::vector<Call>{
                  {"read", 0x1144u}, {"find", 0x2010u, 0x9000u}},
          "GameDB string lookup forwards the first manager and text");
}

}  // namespace

int main() {
    try {
        PrimaryLoadRegistersAndVerifiesAllTables();
        LoadFailuresRespectVerificationGates();
        CleanupAndStringLookupUseTheFirstManager();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
