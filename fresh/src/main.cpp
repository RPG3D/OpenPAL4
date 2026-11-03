#include "pal4/status_check_handler.h"

#include <iostream>

namespace {

class DemoContext final : public pal4::IStatusCheckContext {
public:
    [[nodiscard]] bool IsConditionalCallbackEnabled() const override {
        return true;
    }

    void InvokeConditionalCallback(const int argument) override {
        std::cout << "conditional callback: " << argument << '\n';
    }

    [[nodiscard]] int InvokeResultCallback(const int argument) override {
        return argument + 1;
    }
};

}  // namespace

int main() {
    DemoContext context;
    const pal4::StatusCheckHandler handler(17, 41, context);
    std::cout << "result: " << handler.HandleStatusCheck() << '\n';
}
