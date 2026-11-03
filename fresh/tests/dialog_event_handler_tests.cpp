#include "pal4/dialog_event_handler.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Memory final : pal4::IDialogEventHandlerMemory {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;

    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        writes.emplace_back(address, value);
    }
};

void CheckConstructor(const bool alternate,
                      const std::uint32_t vtable) {
    Memory memory;
    const auto result = alternate ?
        pal4::DialogEventHandler::ConstructAlt(
            0x1000u, 0x1234u, 0x5678u, 0x9abcu, memory) :
        pal4::DialogEventHandler::Construct(
            0x1000u, 0x1234u, 0x5678u, 0x9abcu, memory);
    Check(result == 0x1000u &&
              memory.writes ==
              (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x1008u, 0x1234u}, {0x100cu, 0x5678u},
                  {0x1010u, 0x9abcu}, {0x1000u, vtable}}),
          "handler stores callback, argument, owner and vtable in order");
}

}  // namespace

int main() {
    try {
        CheckConstructor(false,
                         pal4::DialogEventHandler::kMessageBoxVtable);
        CheckConstructor(true,
                         pal4::DialogEventHandler::kYesNoVtable);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
