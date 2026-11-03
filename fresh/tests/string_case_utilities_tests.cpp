#include "pal4/string_case_utilities.h"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestUppercaseCopy() {
    Check(pal4::StringCaseUtilities::UppercaseCopy("").empty() &&
              pal4::StringCaseUtilities::UppercaseCopy("mEsSaGeTaSk") ==
                  "MESSAGETASK" &&
              pal4::StringCaseUtilities::UppercaseCopy("a_2-Z") == "A_2-Z",
          "uppercase copy converts letters and retains other ASCII bytes");
    constexpr std::string_view embedded{"foo\0bar", 7u};
    Check(pal4::StringCaseUtilities::UppercaseCopy(embedded) == "FOO",
          "uppercase copy ends at the first C-string terminator");
}

void TestComparison() {
    using pal4::StringCaseUtilities;
    Check(StringCaseUtilities::CompareInsensitive(nullptr, nullptr) == 0 &&
              StringCaseUtilities::CompareInsensitive(nullptr, "") == 0 &&
              StringCaseUtilities::CompareInsensitive("MessageTask",
                                                      "mEsSaGeTaSk") == 0,
          "null and mixed-case equal names compare equal");
    Check(StringCaseUtilities::CompareInsensitive("a", "B") < 0 &&
              StringCaseUtilities::CompareInsensitive("B", "a") > 0 &&
              StringCaseUtilities::CompareInsensitive("Task", "Tasks") < 0 &&
              StringCaseUtilities::CompareInsensitive("Tasks", "Task") > 0,
          "comparison preserves byte and prefix order");
    Check(StringCaseUtilities::CompareInsensitive("a\0z", "A") == 0,
          "comparison ignores bytes after the C-string terminator");
}

}  // namespace

int main() {
    TestUppercaseCopy();
    TestComparison();
}
