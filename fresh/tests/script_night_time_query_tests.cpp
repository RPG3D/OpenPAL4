#include "pal4/script_night_time_query.h"

#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptNightTimeQueryRuntime {
    std::string source = "y";
    std::string literal;
    std::string fail_at;
    std::vector<std::string> calls;

    void Record(std::string call) {
        calls.push_back(call);
        if (call == fail_at) throw std::runtime_error("injected failure");
    }
    std::uint32_t ArenaManager() override {
        Record("arena");
        return 0x1000u;
    }
    std::uint32_t ExtractTail(std::uint32_t address,
                              std::uint32_t count) override {
        Check(address == 0x10e0u && count == 1u,
              "night flag source and extraction count");
        Record("extract");
        return 0x2000u;
    }
    std::uint32_t InitializeString(std::string_view value) override {
        Check(value == "y", "lowercase comparison literal");
        literal = value;
        Record("literal");
        return 0x3000u;
    }
    std::int32_t CompareCaseInsensitive(std::uint32_t left,
                                        std::uint32_t right) override {
        Check(left == 0x2000u && right == 0x3000u,
              "comparison operand order");
        Record("compare");
        if (source.size() != literal.size()) return 1;
        for (std::size_t i = 0; i < source.size(); ++i)
            if (std::tolower(static_cast<unsigned char>(source[i])) !=
                std::tolower(static_cast<unsigned char>(literal[i])))
                return 1;
        return 0;
    }
    void DestroyString(std::uint32_t handle) override {
        Check(handle == 0x2000u || handle == 0x3000u,
              "native string cleanup handle");
        Record(handle == 0x3000u ? "destroy-literal" : "destroy-extracted");
    }
};

void ValuesAndOrder() {
    for (const auto* value : {"y", "Y", "n", "", "yes"}) {
        Runtime runtime;
        runtime.source = value;
        Check(pal4::ScriptNightTimeQuery::IsNightTime(runtime) ==
                  (runtime.source == "y" || runtime.source == "Y"),
              "nighttime flag comparison");
        Check(runtime.calls == std::vector<std::string>{
                  "arena", "extract", "literal", "compare",
                  "destroy-literal", "destroy-extracted"},
              "string construction and reverse cleanup order");
    }
}

void FailureCleanup() {
    for (const auto* fail : {"literal", "compare"}) {
        Runtime runtime;
        runtime.fail_at = fail;
        bool caught = false;
        try {
            (void)pal4::ScriptNightTimeQuery::IsNightTime(runtime);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && runtime.calls.back() == "destroy-extracted",
              "extracted buffer released on failure");
        if (std::string_view(fail) == "compare")
            Check(runtime.calls[runtime.calls.size() - 2u] ==
                      "destroy-literal", "literal released on comparison error");
    }
}

}  // namespace

int main() {
    try {
        ValuesAndOrder();
        FailureCleanup();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
