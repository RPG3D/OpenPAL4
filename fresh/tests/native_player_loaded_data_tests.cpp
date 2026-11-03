#include "pal4/native_player_loaded_data_runtime.h"
#include "pal4/native_player_slot_copy_runtime.h"
#include "pal4/player_manager_object.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Address(const void* value) {
    return static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(value));
}

std::uint32_t Read(std::uint32_t address) {
    std::uint32_t value{};
    std::memcpy(&value, reinterpret_cast<const void*>(address), sizeof(value));
    return value;
}

void Write(std::uint32_t address, std::uint32_t value) {
    std::memcpy(reinterpret_cast<void*>(address), &value, sizeof(value));
}

struct Inventory final : pal4::PlayerManagerSlotInventoryRuntimeStub {
    std::vector<std::string>& events;
    explicit Inventory(std::vector<std::string>& calls) : events(calls) {}
    std::uint32_t ReadDword(std::uint32_t address) override {
        return Read(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        Write(address, value);
        events.emplace_back(value == 0u ? "disable" : "enable");
    }
    std::uint32_t InventoryManager() override {
        throw std::logic_error("empty equipment needs no inventory manager");
    }
};

struct Processor final : pal4::PlayerManagerSlotProcessRuntimeStub {
    std::uint32_t manager;
    std::vector<std::string>& events;
    pal4::NativePlayerSlotCopyRuntime copy;
    std::array<std::array<std::uint32_t, 80u>, 4u> records{};

    Processor(std::uint32_t address, std::vector<std::string>& calls)
        : manager(address), events(calls) {
        for (std::uint32_t i = 0u; i < records.size(); ++i) {
            records[i][0] = 50u + i;
            records[i][0xe0u / 4u] = i;
        }
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        return Read(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        Write(address, value);
    }
    void ClearCollectionFirstFive(std::uint32_t address) override {
        const auto index = (address - manager - 0x938u) /
            pal4::PlayerManagerObject::kElementBytes;
        if (index != 0u) {
            const auto previous_script = manager +
                (index - 1u) * pal4::PlayerManagerObject::kElementBytes +
                0x654u;
            Check(Read(previous_script + 83u * 4u) == index - 1u &&
                      Read(previous_script + 167u * 4u) == index - 1u,
                  "previous script index precedes next slot processing");
        }
        events.emplace_back("process" + std::to_string(index));
    }
    std::uint32_t GameDatabase() override { return 0x9000u; }
    std::uint32_t FindRecord(std::uint32_t table,
                             std::uint32_t key) override {
        if (table != 0x9020u || key >= records.size())
            throw std::invalid_argument("PAL4 test Role lookup");
        return Address(records[key].data());
    }
    pal4::IPlayerManagerSlotCopyRuntime& CopyRuntime() override {
        return copy;
    }
    std::uint32_t GetElementByIndex(std::uint32_t,
                                    std::uint32_t) override {
        return 100u;
    }
};

}  // namespace

int main() {
    pal4::PlayerManagerObjectState players;
    const auto manager = Address(&players);
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        const auto slot = manager +
            i * pal4::PlayerManagerObject::kElementBytes;
        Write(slot + 0xa48u, i);
        Write(slot + 0x7acu, 60u + i);
        Write(slot + 0x7b4u, 30u + i);
        Write(slot + 0xb08u, 1u);
        Write(slot + 0x654u + 83u * 4u, 0xffffffffu);
        Write(slot + 0x654u + 167u * 4u, 0xffffffffu);
    }
    std::vector<std::string> events;
    Inventory inventory(events);
    Processor processor(manager, events);
    pal4::NativePlayerLoadedDataRuntime runtime(
        manager, inventory, processor);
    Check(pal4::PlayerManagerOperations::ProcessLoadedData(
              manager, runtime) == 1u,
          "loaded player loop enables the first slot");
    Check(events == std::vector<std::string>{
              "disable", "process0", "disable", "process1",
              "disable", "process2", "disable", "process3",
              "enable"},
          "four slot processors run between inventory and script work");
    for (std::uint32_t i = 0u; i < 4u; ++i) {
        const auto slot = manager +
            i * pal4::PlayerManagerObject::kElementBytes;
        const auto script = slot + 0x654u;
        Check(Read(slot + 0xb08u) == (i == 0u ? 1u : 0u) &&
                  Read(slot + 0x658u) == 50u + i &&
                  Read(script + 83u * 4u) == i &&
                  Read(script + 167u * 4u) == i,
              "loaded slots contain copied Role data and script indices");
    }
    bool rejected = false;
    try {
        runtime.ProcessGameData(manager + 4u);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    Check(rejected, "postprocess rejects unaligned player addresses");
    std::cout << "PASS: native player postprocess routes four slots\n";
}
