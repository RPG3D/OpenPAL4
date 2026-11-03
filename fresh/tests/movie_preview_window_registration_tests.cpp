#include "pal4/movie_preview_window_registration.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::MoviePreviewWindowRegistrationRuntimeStub {
    std::vector<std::string> calls;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    bool fail_construct{};
    bool fail_insert{};
    bool allow_null_callback{};
    std::uint32_t next_allocation{0x5000u};

    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.emplace_back("allocate:" + std::to_string(bytes));
        const auto address = next_allocation;
        next_allocation += 0x1000u;
        return address;
    }
    std::uint32_t Construct(std::uint32_t storage,
                            std::uint32_t name) override {
        Check(storage == 0x5000u && name == 0x7000u,
              "factory forwards storage and name");
        calls.emplace_back("construct");
        if (fail_construct) throw std::runtime_error("construct");
        return storage;
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x5000u, "failed constructor storage freed");
        calls.emplace_back("free");
    }
    std::uint32_t FrameManager() override {
        calls.emplace_back("manager");
        return 0x8000u;
    }
    std::uint32_t RegisteredName() override {
        calls.emplace_back("cached-name");
        return 0x9000u;
    }
    std::uint32_t LiteralName() override {
        calls.emplace_back("literal-name");
        return 0x9100u;
    }
    std::uint32_t RegisterFactory(
        std::uint32_t manager, std::uint32_t name,
        Creator creator) override {
        Check(manager == 0x8000u && name == 0x9000u &&
                  creator == &pal4::MoviePreviewWindowRegistration::CreateObject,
              "factory registration uses cached name and creator");
        calls.emplace_back("register-factory");
        return 0xabceu;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write");
    }
    void InsertOrReplace(std::uint32_t tree, std::string_view command,
                         std::uint32_t& callback) override {
        Check(tree == 0x8104u && command == "show_movie_preview" &&
                  callback == (allow_null_callback ? 0u : 0x5000u),
              "show command registered in UIFrameManager tree");
        calls.emplace_back("insert");
        if (fail_insert) throw std::runtime_error("insert");
    }
    void DeleteCallback(std::uint32_t callback,
                        std::uint32_t flags) override {
        Check(callback == 0x5000u && flags == 1u,
              "local callback released with deleting flag");
        calls.emplace_back("delete");
    }
};

void CheckRegistration() {
    using Registration = pal4::MoviePreviewWindowRegistration;
    const char* name = nullptr;
    Check(std::string_view(Registration::RegisterNameThunk(name)) ==
                  "moviePreviewWindow" && name != nullptr,
          "name thunk stores movie preview name");
    Runtime runtime;
    Check(Registration::CreateObject(0x7000u, runtime) == 0x5000u &&
              runtime.calls == std::vector<std::string>{
                  "allocate:112", "construct"},
          "factory allocates 0x70 bytes and constructs object");
    runtime.calls.clear();
    Check(Registration::RegisterFactoryThunk(runtime) == 0xabceu &&
              runtime.calls == std::vector<std::string>{
                  "manager", "cached-name", "register-factory"},
          "factory registration uses cached name");
    runtime = Runtime{};
    Check(Registration::RegisterShowCommandThunk(runtime) == 0x8e4488u &&
              runtime.words.at(0x5000u) == 0x843494u &&
              runtime.words.at(0x5004u) == 0x9100u &&
              runtime.calls == std::vector<std::string>{
                  "literal-name", "allocate:8", "write", "write",
                  "manager", "insert", "delete"},
          "command callback stores literal name and releases local owner");
}

void CheckUnwind() {
    using Registration = pal4::MoviePreviewWindowRegistration;
    Runtime runtime;
    runtime.fail_construct = true;
    try {
        (void)Registration::CreateObject(0x7000u, runtime);
        Check(false, "constructor failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "allocate:112", "construct", "free"},
              "factory frees construction storage on exception");
    }
    runtime = Runtime{};
    runtime.fail_insert = true;
    try {
        (void)Registration::RegisterShowCommand(runtime);
        Check(false, "insert failure propagates");
    } catch (const std::runtime_error&) {
        Check(runtime.calls.back() == "delete",
              "callback released after insertion exception");
    }
    runtime = Runtime{};
    runtime.next_allocation = 0u;
    Check(Registration::CreateObject(0x7000u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{"allocate:112"},
          "null allocation skips constructor");
    runtime = Runtime{};
    runtime.next_allocation = 0u;
    runtime.allow_null_callback = true;
    Check(Registration::RegisterShowCommand(runtime) == 0x8e4488u &&
              runtime.calls == std::vector<std::string>{
                  "literal-name", "allocate:8", "manager", "insert"},
          "null callback still registers command without local release");
}

}  // namespace

int main() {
    CheckRegistration();
    CheckUnwind();
    std::cout << "Movie preview window registration tests passed\n";
}
