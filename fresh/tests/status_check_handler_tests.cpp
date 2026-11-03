#include "pal4/status_check_handler.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class FakeContext final : public pal4::IStatusCheckContext {
public:
    explicit FakeContext(const bool enabled, const int result)
        : enabled_(enabled), result_(result) {}

    [[nodiscard]] bool IsConditionalCallbackEnabled() const override {
        events.push_back("condition");
        return enabled_;
    }

    void InvokeConditionalCallback(const int argument) override {
        events.push_back("conditional:" + std::to_string(argument));
    }

    [[nodiscard]] int InvokeResultCallback(const int argument) override {
        events.push_back("result:" + std::to_string(argument));
        return result_;
    }

    mutable std::vector<std::string> events;

private:
    bool enabled_;
    int result_;
};

void ConditionalCallbackRunsBeforeResultCallback() {
    FakeContext context(true, 73);
    const pal4::StatusCheckHandler handler(19, 42, context);
    Check(handler.HandleStatusCheck() == 73, "returns result callback value");
    Check(context.events == std::vector<std::string>{
                               "condition", "conditional:19", "result:42"},
          "enabled path calls callbacks in binary order with the right args");
}

void DisabledConditionalCallbackIsSkipped() {
    FakeContext context(false, -8);
    const pal4::StatusCheckHandler handler(19, 42, context);
    Check(handler.HandleStatusCheck() == -8,
          "disabled path still returns result callback value");
    Check(context.events == std::vector<std::string>{"condition", "result:42"},
          "disabled path skips only the first callback");
}

void ZeroArgumentsAreForwardedUnchanged() {
    FakeContext context(true, 0);
    const pal4::StatusCheckHandler handler(0, 0, context);
    Check(handler.HandleStatusCheck() == 0, "zero callback result is preserved");
    Check(context.events == std::vector<std::string>{
                               "condition", "conditional:0", "result:0"},
          "zero arguments are not treated as missing values");
}

}  // namespace

int main() {
    ConditionalCallbackRunsBeforeResultCallback();
    DisabledConditionalCallbackIsSkipped();
    ZeroArgumentsAreForwardedUnchanged();
    std::cout << "3 status-check tests passed\n";
}
