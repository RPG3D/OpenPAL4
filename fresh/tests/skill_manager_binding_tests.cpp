#include "pal4/skill_manager_binding.h"

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

struct Call {
    std::string kind;
    std::string text;
    std::uint32_t system;
    std::uint32_t value;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::ISkillManagerBindingRuntime {
    std::vector<Call> calls;
    std::array<std::uint32_t, 4> handles = {
        0x2000, 0x3000, 0x4000, 0x5000};
    int fail_register = -1;

    static std::size_t Index(std::string_view table) {
        const std::array<std::string_view, 4> names = {
            "Skill", "Stunt", "Magic", "WuLing"};
        for (std::size_t index = 0; index < names.size(); ++index)
            if (names[index] == table) return index;
        throw std::runtime_error("unexpected table");
    }
    std::uint8_t RegisterHandler(std::uint32_t system,
                                 std::string_view table,
                                 std::uint32_t callback) override {
        calls.push_back({"register", std::string(table), system, callback});
        const auto index = Index(table);
        if (static_cast<int>(index) == fail_register)
            throw std::runtime_error("register failed");
        return static_cast<std::uint8_t>(index + 1);
    }
    std::uint32_t FindTable(std::uint32_t system,
                            std::string_view table) override {
        calls.push_back({"find", std::string(table), system, 0});
        return handles[Index(table)];
    }
    std::uint32_t FindPropertyString(std::uint32_t system,
                                     std::string_view table) override {
        calls.push_back({"find_property", std::string(table), system, 0});
        return handles[Index(table)];
    }
    void LogError(std::string_view message) override {
        calls.push_back({"log", std::string(message), 0, 0});
    }
};

void CheckHandlers() {
    Runtime runtime;
    pal4::SkillManagerBinding binding(0x1000);
    Check(binding.RegisterHandlers(runtime) == 3 &&
              runtime.calls == std::vector<Call>{
                                   {"register", "Skill", 0x1000, 0x4f3d90},
                                   {"register", "Stunt", 0x1000, 0x4f49c0},
                                   {"register", "Magic", 0x1000, 0x4f55a0}},
          "three handlers register in order and forward final byte");
    runtime.calls.clear();
    runtime.fail_register = 1;
    try {
        static_cast<void>(binding.RegisterHandlers(runtime));
        Check(false, "registration failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "register failed" &&
                  runtime.calls.size() == 2,
              "registration stops before Magic on Stunt failure");
    }
}

void CheckTables() {
    Runtime runtime;
    pal4::SkillManagerBinding binding(0x1000);
    Check(binding.VerifyTables(runtime) &&
              binding.Handles() == runtime.handles &&
              runtime.calls == std::vector<Call>{
                                   {"find", "Skill", 0x1000, 0},
                                   {"find", "Stunt", 0x1000, 0},
                                   {"find", "Magic", 0x1000, 0},
                                   {"find_property", "WuLing", 0x1000, 0}},
          "four required handles use the native lookup order and service");

    const std::array<std::string, 4> names = {
        "Skill", "Stunt", "Magic", "WuLing"};
    for (std::size_t missing = 0; missing < names.size(); ++missing) {
        Runtime broken;
        broken.handles[missing] = 0;
        pal4::SkillManagerBinding candidate(0x1000);
        const auto expected =
            "ERROR [ palDBSkillManager::verifyDataTable < the table " +
            names[missing] + "  doesn't exist>]";
        try {
            static_cast<void>(candidate.VerifyTables(broken));
            Check(false, "missing table must throw");
        } catch (const std::runtime_error& error) {
            Check(std::string(error.what()) == expected &&
                      broken.calls.size() == missing + 2 &&
                      broken.calls.back() == Call{"log", expected, 0, 0} &&
                      candidate.Handles()[missing] == 0,
                  "missing table logs original format and throws immediately");
        }
    }
}

}  // namespace

int main() {
    try {
        CheckHandlers();
        CheckTables();
        std::cout << "skill_manager_binding_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
