#include "pal4/collection_iterator_factory.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
#include <new>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::ICollectionIteratorFactoryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t allocated{0x5000u};
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        return allocated;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
};

using Factory = std::uint32_t (*)(
    pal4::CollectionIteratorFactoryState&, std::uint32_t,
    pal4::ICollectionIteratorFactoryRuntime&);

void TestBasicFactories() {
    const std::array<Factory, 5> factories{
        &pal4::CollectionIteratorFactory::Create1,
        &pal4::CollectionIteratorFactory::Create2,
        &pal4::CollectionIteratorFactory::Create3,
        &pal4::CollectionIteratorFactory::Create4,
        &pal4::CollectionIteratorFactory::Create5};
    const std::array<std::uint32_t, 5> vtables{
        0x849748u, 0x849760u, 0x849778u, 0x849790u, 0x8497a8u};
    for (std::size_t index = 0; index < factories.size(); ++index) {
        Runtime runtime;
        pal4::CollectionIteratorFactoryState owner{
            0x1234u, 0x7000u, 0x9999u};
        Check(factories[index](owner, 0x3000u, runtime) == 0x5000u &&
                  owner.created == 0x5000u &&
                  owner.source == 0x7000u &&
                  owner.vtable == 0x1234u &&
                  runtime.words.at(0x5000u) == vtables[index] &&
                  runtime.words.at(0x5004u) == 0x3000u &&
                  runtime.words.at(0x5008u) == 0x7000u &&
                  runtime.calls == std::vector<std::string>{
                      "allocate:12", "write:20484:0", "write:20488:0",
                      "write:20480:" + std::to_string(vtables[index]),
                      "write:20484:12288", "write:20488:28672"},
              "basic iterator factory initializes vtable and two fields");

        runtime.allocated = 0u;
        runtime.calls.clear();
        Check(factories[index](owner, 0x3010u, runtime) == 0u &&
                  owner.created == 0u &&
                  runtime.words.at(4u) == 0x3010u &&
                  runtime.words.at(8u) == 0x7000u &&
                  runtime.calls == std::vector<std::string>{
                      "allocate:12", "write:4:12304",
                      "write:8:28672"},
              "basic allocation failure preserves observed low-address writes");
    }
}

void TestRangeFactory() {
    Runtime runtime;
    runtime.words[0x3004u] = 0xaaaau;
    runtime.words[0x3008u] = 0xbbbbu;
    pal4::CollectionIteratorFactoryState owner{
        0x1234u, 0x7000u, 0x9999u};
    Check(pal4::CollectionIteratorFactory::Create6(
              owner, 0x3000u, runtime) == 0x5000u &&
              owner.created == 0x5000u &&
              runtime.words.at(0x5000u) == 0x8497c0u &&
              runtime.words.at(0x5004u) == 0xaaaau &&
              runtime.words.at(0x5008u) == 0xbbbbu &&
              runtime.words.at(0x500cu) == 0x3000u &&
              runtime.words.at(0x5010u) == 0x7000u &&
              runtime.calls.front() == "allocate:20" &&
              runtime.calls[runtime.calls.size() - 2u] == "read:12292",
          "range iterator copies source bounds after owner/source fields");

    runtime.allocated = 0u;
    runtime.calls.clear();
    bool caught = false;
    try {
        (void)pal4::CollectionIteratorFactory::Create6(
            owner, 0x3000u, runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && owner.created == 0x5000u &&
              runtime.calls == std::vector<std::string>{"allocate:20"},
          "range allocation failure throws before null dereference or owner write");

    pal4::CollectionIteratorFactoryRuntimeStub stub;
    Check(stub.Allocate(12u) == 0u && stub.ReadWord(1u) == 0u,
          "native allocator boundary has a linkable stub");
}

}  // namespace

int main() {
    TestBasicFactories();
    TestRangeFactory();
}
