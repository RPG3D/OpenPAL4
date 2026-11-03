#include "pal4/game_resource_manager.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

class Runtime final : public pal4::GameResourceManagerRuntimeStub {
public:
    std::uint32_t Allocate(std::uint32_t size) override {
        events.push_back("allocate");
        Check(size == 16, "scale path allocation size");
        return 0x100;
    }
    void StoreString(std::uint32_t address, std::string_view value) override {
        Check(address == 0x100 && value == "res/extSize.scl",
              "stored scale path");
        events.push_back("store");
    }
    std::string ReadString(std::uint32_t address) override {
        Check(address == 0x20, "directory address");
        return "res/";
    }
    std::uint32_t OpenDataStream(std::uint32_t type, std::uint32_t mode,
                                  std::string_view path) override {
        Check(type == 2 && mode == 1 && path == "res/extSize.scl",
              "scale stream opening");
        events.push_back("open");
        return stream;
    }
    std::uint32_t Free(std::uint32_t address) override {
        Check(address == 0x100, "temporary path free");
        events.push_back("free");
        return 0x123;
    }
    void ReadStreamWords(std::uint32_t source, std::uint32_t* destination,
                         std::uint32_t count) override {
        Check(source == stream && count == 3 && destination == manager + 49,
              "contiguous three-word read");
        events.push_back("read");
        destination[0] = 0x11;
        destination[1] = 0x22;
        destination[2] = 0x33;
    }
    std::uint32_t CloseDataStream(std::uint32_t source) override {
        Check(source == stream, "stream close handle");
        events.push_back("close");
        return 0x456;
    }

    std::uint32_t* manager{};
    std::uint32_t stream{0x900};
    std::vector<std::string> events;
};

void GateAndOpenFailure() {
    pal4::GameResourceManager manager;
    Runtime runtime;
    (void)manager.Construct();
    runtime.manager = manager.words.data();
    manager.words[2] = 0x20;
    manager.words[4] = 0x40;
    Check(manager.LoadExtSizeScl(runtime) == 0 && runtime.events.empty(),
          "missing clump gate");
    manager.words[5] = 0x700;
    manager.words[4] = 0;
    Check(manager.LoadExtSizeScl(runtime) == 0 && runtime.events.empty(),
          "missing name gate");
    manager.words[4] = 0x40;
    runtime.stream = 0;
    Check(manager.LoadExtSizeScl(runtime) == 0x123 &&
          runtime.events == std::vector<std::string>{
              "allocate", "store", "open", "free"} &&
          manager.words[49] == 0x3f800000u,
          "open failure returns free result without reading");
}

void SuccessfulRead() {
    pal4::GameResourceManager manager;
    Runtime runtime;
    (void)manager.Construct();
    runtime.manager = manager.words.data();
    manager.words[2] = 0x20;
    manager.words[4] = 0x40;
    manager.words[5] = 0x700;
    Check(manager.LoadExtSizeScl(runtime) == 0x456 &&
          manager.words[49] == 0x11 && manager.words[50] == 0x22 &&
          manager.words[51] == 0x33 &&
          runtime.events == std::vector<std::string>{
              "allocate", "store", "open", "free", "read", "close"},
          "scale words and stream close result");
}

}  // namespace

int main() {
    GateAndOpenFailure();
    SuccessfulRead();
}
