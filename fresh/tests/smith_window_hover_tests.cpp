#include "pal4/smith_window.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISmithWindowHoverRuntime {
    static constexpr std::uint32_t object = 0x1000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {object + 0x40u, 0x4000u},
        {object + 0x44u, 0x4400u},
        {0x5000u + 0x138u, 0u},
        {0x6000u + 0x138u, 0x80000001u},
    };
    std::map<std::uint32_t, std::uint32_t> hovers{
        {0x4000u, 0x5000u}, {0x4400u, 0x6000u}};
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    std::uint32_t HoverItem(std::uint32_t list) override {
        calls.emplace_back("hover");
        return hovers.at(list);
    }
};

void CheckBothLists() {
    Runtime runtime;
    Check(pal4::SmithWindow::GetEquipmentHoverItemId(
              Runtime::object, runtime) == 0 &&
              pal4::SmithWindow::GetPrescriptionHoverItemId(
                  Runtime::object, runtime) ==
                  static_cast<std::int32_t>(0x80000001u) &&
              runtime.calls == (std::vector<std::string>{
                  "read", "hover", "read",
                  "read", "hover", "read"}),
          "both list offsets and signed 32-bit item IDs");

    runtime.calls.clear();
    runtime.hovers[0x4000u] = 0u;
    runtime.hovers[0x4400u] = 0u;
    Check(pal4::SmithWindow::GetEquipmentHoverItemId(
              Runtime::object, runtime) == -1 &&
              pal4::SmithWindow::GetPrescriptionHoverItemId(
                  Runtime::object, runtime) == -1 &&
              runtime.calls == (std::vector<std::string>{
                  "read", "hover", "read", "hover"}),
          "missing hover returns minus one without item dereference");
}

}  // namespace

int main() {
    try {
        CheckBothLists();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
