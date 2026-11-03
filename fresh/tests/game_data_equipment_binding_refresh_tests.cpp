#include "pal4/game_data_equipment_binding_refresh.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataEquipmentBindingRefreshRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x1080, 0x3000}, {0x1084, 0x3008},
        {0x1018, 0x2000}, {0x101c, 0x2078}};
    std::vector<std::string> calls;
    std::uint32_t log_handle = 0x4000;
    std::uint32_t release_result = 0x7777;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint32_t BeginLog(std::string_view prefix,
                           std::uint32_t table_name) override {
        Check(prefix == "beginBindField, table:" && table_name == 0x1034,
              "begin log text and table name");
        calls.emplace_back("begin log");
        return log_handle;
    }
    void AssignEffects(std::uint32_t destination,
                       std::uint32_t source) override {
        Check(destination == 0x106c && source == 0x1014,
              "effect assignment offsets");
        calls.emplace_back("assign");
    }
    void InitializeRelationStructures(std::uint32_t object) override {
        Check(object == 0x106c, "EquipRelation initializer address");
        calls.emplace_back("init relation");
    }
    void InitializeEquipmentStructures(std::uint32_t object) override {
        Check(object == 0x106c, "Equip initializer address");
        calls.emplace_back("init equipment");
    }
    void BindEffects(std::uint32_t object) override {
        Check(object == 0x106c, "bind object address");
        calls.emplace_back("bind");
    }
    void EndLog(std::uint32_t log, std::string_view prefix,
                std::uint32_t table_name) override {
        Check(log == log_handle && prefix == "endBindField, table:" &&
                  table_name == 0x1034,
              "end log reuses buffer and table name");
        calls.emplace_back("end log");
    }
    std::uint32_t ReleaseLog(std::uint32_t log) override {
        Check(log == log_handle, "release receives temporary log");
        calls.emplace_back("release log");
        return release_result;
    }
};

void CheckEqualCounts() {
    Runtime relation;
    Check(pal4::GameDataEquipmentBindingRefresh::RefreshRelation(
              0x1000, relation) == 0 && relation.calls.empty(),
          "equal non-null relation counts need no bind");
    Runtime equip;
    equip.words[0x1080] = equip.words[0x1084] = 0;
    equip.words[0x1018] = equip.words[0x101c] = 0;
    Check(pal4::GameDataEquipmentBindingRefresh::RefreshEquipment(
              0x1000, equip) == 0x1014 && equip.calls.empty(),
          "equal null Equip counts return field-object address");
}

void CheckBindingRoutes() {
    Runtime relation;
    relation.words[0x101c] = 0x203c;
    Check(pal4::GameDataEquipmentBindingRefresh::RefreshRelation(
              0x1000, relation) == 0x7777 &&
              relation.calls == std::vector<std::string>{
                                    "begin log", "assign", "init relation",
                                    "bind", "end log", "release log"},
          "relation rebind uses its own initializer and releases log");

    Runtime equip;
    equip.words[0x101c] = 0x203c;
    Check(pal4::GameDataEquipmentBindingRefresh::RefreshEquipment(
              0x1000, equip) == 0x7777 &&
              equip.calls == std::vector<std::string>{
                                 "begin log", "assign", "init equipment",
                                 "bind", "end log", "release log"},
          "Equip rebind uses its own initializer");
}

void CheckNullLog() {
    Runtime runtime;
    runtime.words[0x101c] = 0x203c;
    runtime.log_handle = 0;
    Check(pal4::GameDataEquipmentBindingRefresh::RefreshRelation(
              0x1000, runtime) == 0 &&
              runtime.calls == std::vector<std::string>{
                                   "begin log", "assign", "init relation",
                                   "bind", "end log"},
          "null log still binds without a release call");
}

}  // namespace

int main() {
    try {
        CheckEqualCounts();
        CheckBindingRoutes();
        CheckNullLog();
        std::cout << "game_data_equipment_binding_refresh_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
