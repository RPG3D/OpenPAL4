#include "pal4/callback_record_invoker.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICallbackRecordRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t function = 0;
    std::uint32_t argument = 0;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    std::uint32_t Invoke(std::uint32_t target,
                         std::uint32_t value) override {
        calls.emplace_back("invoke");
        function = target;
        argument = value;
        return 0x89abcdefu;
    }
};

}  // namespace

int main() {
    try {
        Runtime first;
        first.words[0x1004u] = 0x7000u;
        first.words[0x1008u] = 0x8000u;
        Check(pal4::CallbackRecordInvoker::InvokeFunctionFirst(
                  0x1000u, first) == 0x89abcdefu,
              "function-first return forwarding");
        Check(first.function == 0x7000u && first.argument == 0x8000u &&
              first.calls == std::vector<std::string>{
                  "read:4100", "read:4104", "invoke"},
              "function-first fields and order");

        Runtime second;
        second.words[0x1004u] = 0x8000u;
        second.words[0x1008u] = 0x7000u;
        Check(pal4::CallbackRecordInvoker::InvokeArgumentFirst(
                  0x1000u, second) == 0x89abcdefu,
              "argument-first return forwarding");
        Check(second.function == 0x7000u && second.argument == 0x8000u &&
              second.calls == std::vector<std::string>{
                  "read:4104", "read:4100", "invoke"},
              "argument-first fields and order");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
