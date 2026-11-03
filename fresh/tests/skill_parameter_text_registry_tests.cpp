#include "pal4/skill_parameter_text_registry.h"

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
    std::uint32_t address;
    std::string text;
    bool operator==(const Call&) const = default;
};

struct Runtime final : pal4::ISkillParameterTextRuntime {
    std::vector<Call> calls;
    std::uint32_t fail_copy_index = 99;
    std::uint32_t fail_set_index = 99;
    std::uint32_t fail_destroy_index = 99;
    int register_result = -7;

    static std::uint32_t Index(std::uint32_t address) {
        return (address - pal4::SkillParameterTextRegistry::kFirstAddress) /
               pal4::SkillParameterTextRegistry::kTextBytes;
    }
    void InitializeText(std::uint32_t address) override {
        calls.push_back({"initialize", address, {}});
    }
    bool CopyLiteral(std::uint32_t address,
                     std::string_view text) override {
        calls.push_back({"copy", address, std::string(text)});
        return Index(address) != fail_copy_index;
    }
    int SetExceptionMessage(std::uint32_t address,
                            std::string_view text) override {
        calls.push_back({"set", address, std::string(text)});
        if (Index(address) == fail_set_index)
            throw std::runtime_error("set failed");
        return static_cast<int>(200 + Index(address));
    }
    void DestroyText(std::uint32_t address) override {
        calls.push_back({"destroy", address, {}});
        if (Index(address) == fail_destroy_index)
            throw std::runtime_error("destroy failed");
    }
    int RegisterCleanup() override {
        calls.push_back({"register", 0x4f2d90, {}});
        return register_result;
    }
};

void CheckInitialize() {
    Runtime runtime;
    Check(pal4::SkillParameterTextRegistry::Initialize(runtime) == 208 &&
              runtime.calls.size() == 18,
          "initialization returns ninth SetExceptionMessage result");
    for (std::uint32_t index = 0; index < 9; ++index) {
        const auto address = 0x8ef248u + index * 16;
        const auto text = "SKILL_PARAM_" + std::to_string(index + 1);
        Check(runtime.calls[index * 2] ==
                  Call{"initialize", address, {}} &&
                  runtime.calls[index * 2 + 1] ==
                      Call{index < 6 ? "copy" : "set", address, text},
              "each literal follows its 16-byte slot initialization");
    }

    Runtime allocation_failure;
    allocation_failure.fail_copy_index = 2;
    Check(pal4::SkillParameterTextRegistry::Initialize(
              allocation_failure) == 208 &&
              allocation_failure.calls.size() == 18 &&
              allocation_failure.calls[6] ==
                  Call{"initialize", 0x8ef278, {}} &&
              allocation_failure.calls.back() ==
                  Call{"set", 0x8ef2c8, "SKILL_PARAM_9"},
          "allocation failure continues to later parameters");
}

void CheckRegistrationAndInterruption() {
    Runtime runtime;
    Check(pal4::SkillParameterTextRegistry::InitializeAndRegisterCleanup(
              runtime) == -7 &&
              runtime.calls.size() == 19 &&
              runtime.calls.back() == Call{"register", 0x4f2d90, {}},
          "cleanup registration follows all nine initialized strings");

    Runtime interrupted;
    interrupted.fail_set_index = 7;
    try {
        static_cast<void>(
            pal4::SkillParameterTextRegistry::InitializeAndRegisterCleanup(
                interrupted));
        Check(false, "set failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "set failed" &&
                  interrupted.calls.size() == 16 &&
                  interrupted.calls.back() ==
                      Call{"set", 0x8ef2b8, "SKILL_PARAM_8"},
              "failure stops before ninth parameter and registration");
    }
}

void CheckDestroy() {
    Runtime runtime;
    pal4::SkillParameterTextRegistry::Destroy(runtime);
    Check(runtime.calls.size() == 9,
          "all nine text objects are destroyed");
    for (std::uint32_t index = 0; index < 9; ++index)
        Check(runtime.calls[index] ==
                  Call{"destroy", 0x8ef2c8u - index * 16, {}},
              "vector destruction runs from ninth slot to first");

    Runtime interrupted;
    interrupted.fail_destroy_index = 6;
    try {
        pal4::SkillParameterTextRegistry::Destroy(interrupted);
        Check(false, "destructor failure must propagate");
    } catch (const std::runtime_error& error) {
        Check(std::string(error.what()) == "destroy failed" &&
                  interrupted.calls.size() == 3 &&
                  interrupted.calls.back() ==
                      Call{"destroy", 0x8ef2a8, {}},
              "destruction stops at failing seventh slot");
    }
}

}  // namespace

int main() {
    try {
        CheckInitialize();
        CheckRegistrationAndInterruption();
        CheckDestroy();
        std::cout << "skill_parameter_text_registry_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
