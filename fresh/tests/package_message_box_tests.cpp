#include "pal4/package_message_box.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IPackageMessageBoxRuntime {
public:
    std::uint32_t LastError() override {
        events.emplace_back("last_error");
        return error;
    }
    std::string SystemErrorMessage(std::uint32_t code,
                                    std::uint32_t language) override {
        events.emplace_back("system_message");
        last_code = code;
        last_language = language;
        return system_message;
    }
    int Show(const char* text, const char* caption,
             std::uint32_t type) override {
        events.emplace_back("show");
        last_text = text;
        last_caption = caption;
        last_type = type;
        return result;
    }
    std::uint32_t error = 5u;
    std::uint32_t last_code = 0u;
    std::uint32_t last_language = 0u;
    std::uint32_t last_type = 0u;
    int result = 7;
    std::string system_message = "Access denied.";
    std::string last_text;
    std::string last_caption;
    std::vector<std::string> events;
};

}  // namespace

int main() {
    Runtime runtime;
    pal4::PackageMessageBox box(runtime);
    Check(box.ShowFormatted("Missing directory: %s (%d)", "fonts", 3) == 7 &&
          runtime.last_text == "Missing directory: fonts (3)" &&
          runtime.last_caption == "Message" && runtime.last_type == 0u &&
          runtime.events == std::vector<std::string>{"show"},
          "formatted message uses Message caption and plain type");

    runtime.events.clear();
    runtime.result = 2;
    Check(box.ShowSystemError(0x10u, "Read %s failed: %u", "game.cpk", 4u) == 2 &&
          runtime.last_text ==
              "Read game.cpk failed: 4\nSystem Error Code : 5\nError Message : Access denied." &&
          runtime.last_caption == "Error" && runtime.last_type == 0x10u &&
          runtime.last_code == 5u && runtime.last_language == 0x400u &&
          runtime.events == std::vector<std::string>{
              "last_error", "system_message", "show"},
          "system error captures code before formatting and appends message");

    runtime.events.clear();
    runtime.error = 0;
    runtime.system_message.clear();
    Check(box.ShowSystemError(0u, "empty") == 2 &&
          runtime.last_text ==
              "empty\nSystem Error Code : 0\nError Message : " &&
          runtime.events.size() == 3u,
          "empty system message remains an explicit boundary result");
    std::cout << "3 package message box cases passed\n";
}
