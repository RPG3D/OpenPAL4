#include "pal4/collection_ui_object_factory.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::CollectionUiObjectRuntimeStub {
    std::vector<char> events;
    std::uint32_t storage{0x8000u};
    bool throw_on_construct{};
    bool invoke_creator{};
    std::uint32_t invoked_result{};

    std::uint32_t Allocate(std::size_t size) override {
        Check(size == 0x5cu, "UI object allocation is 0x5c bytes");
        events.push_back('A');
        return storage;
    }
    std::uint32_t Construct(std::uint32_t memory,
                            std::uint32_t name) override {
        Check(memory == 0x8000u && name == 0x7000u,
              "UI constructor receives allocated storage and name");
        events.push_back('C');
        if (throw_on_construct) throw std::runtime_error("construct");
        return 0x8100u;
    }
    void Free(std::uint32_t memory) override {
        Check(memory == 0x8000u,
              "constructor failure releases the same allocation");
        events.push_back('F');
    }
    std::uint32_t FrameManager() override {
        events.push_back('M');
        return 0x9000u;
    }
    const char* CastWindowName() override {
        events.push_back('N');
        return "CastWindow";
    }
    std::uint32_t ProcessFrameEvent(
        std::uint32_t manager, const char* name,
        pal4::CollectionUiObjectCreator creator) override {
        Check(manager == 0x9000u && name &&
                  std::strcmp(name, "CastWindow") == 0 &&
                  creator == &pal4::CollectionUiObjectFactory::CreateObject,
              "UI frame registration receives CastWindow and create callback");
        events.push_back('P');
        if (invoke_creator) invoked_result = creator(0x7000u, *this);
        return 0x91u;
    }
};

void CheckFactory() {
    Runtime runtime;
    Check(pal4::CollectionUiObjectFactory::CreateObject(0x7000u,
                                                         runtime) == 0x8100u &&
              runtime.events == std::vector<char>{'A', 'C'},
          "successful factory forwards constructed object");
    runtime.events.clear();
    runtime.storage = 0u;
    Check(pal4::CollectionUiObjectFactory::CreateObject(0x7000u,
                                                         runtime) == 0u &&
              runtime.events == std::vector<char>{'A'},
          "zero allocation skips constructor");
    runtime.events.clear();
    runtime.storage = 0x8000u;
    runtime.throw_on_construct = true;
    bool threw = false;
    try {
        (void)pal4::CollectionUiObjectFactory::CreateObject(0x7000u,
                                                              runtime);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && runtime.events == std::vector<char>{'A', 'C', 'F'},
          "constructor failure releases storage before propagating");
}

void CheckRegistration() {
    Runtime runtime;
    runtime.invoke_creator = true;
    Check(pal4::CollectionUiObjectFactory::RegisterCreateObject(runtime) ==
              0x91u && runtime.invoked_result == 0x8100u &&
              runtime.events == std::vector<char>{'M', 'N', 'P', 'A', 'C'},
          "registration forwards factory callable to frame manager");
    runtime.events.clear();
    runtime.invoke_creator = false;
    Check(pal4::CollectionUiObjectFactory::RegisterCreateObjectThunk(
              runtime) == 0x91u &&
              runtime.events == std::vector<char>{'M', 'N', 'P'},
          "thunk forwards registration result");
}

}  // namespace

int main() {
    CheckFactory();
    CheckRegistration();
    std::cout << "collection UI object factory tests passed\n";
}
