#include "pal4/pal_texture_dictionary.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class AccessRuntime final
    : public pal4::IPalTextureDictionaryAccessRuntime {
public:
    std::uint32_t GetSingletonManager() override {
        ++manager_calls;
        return 0x6000u;
    }
    std::uint32_t Register(std::uint32_t manager, std::string_view name,
                           std::uint32_t id, std::uint32_t factory,
                           std::uint32_t destroy) override {
        ++register_calls;
        Check(manager == 0x6000u && name == "palTextureDict" &&
                  id == 1000u && factory == 0x41a490u &&
                  destroy == 0x7de5d0u,
              "texture singleton registration matches IDA");
        return result;
    }
    std::uint32_t result{0x7000u};
    int manager_calls{};
    int register_calls{};
};

class ObjectRuntime final : public pal4::IPalTextureDictionaryRuntime {
public:
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.push_back("allocate");
        Check(bytes == 12u, "texture dictionary allocates 12 bytes");
        return allocation_result;
    }
    void Free(std::uint32_t address) override {
        Check(address == 0x1000u, "texture dictionary frees object address");
        events.push_back("free");
    }
    pal4::PalTextureDictionaryState& Access(
        std::uint32_t address) override {
        Check(address == 0x1000u, "texture object state address");
        events.push_back("access");
        if (throw_on_access) {
            throw std::runtime_error("access");
        }
        return state;
    }
    void BaseConstruct(std::uint32_t address) override {
        Check(address == 0x1000u, "base constructor object address");
        events.push_back("base construct");
        if (throw_on_construct) {
            throw std::runtime_error("construct");
        }
        state = {0x84b940u, 0u, 1u};
    }
    void BaseDestruct(std::uint32_t address) override {
        Check(address == 0x1000u &&
                  state.vtable == 0x842174u,
              "base destructor sees derived vtable write");
        events.push_back("base destruct");
        state.vtable = 0x84b940u;
    }
    std::uint32_t BaseAddTexture(std::uint32_t dictionary,
                                 std::uint32_t texture) override {
        Check(dictionary == 0x1000u && texture == 0x5000u,
              "add-texture thunk forwards both arguments");
        events.push_back("add texture");
        return 0x9999u;
    }

    pal4::PalTextureDictionaryState state{0xaaaaaaaau, 0xbbbbbbbbu,
                                          0xccccccccu};
    std::uint32_t allocation_result{0x1000u};
    bool throw_on_construct{};
    bool throw_on_access{};
    std::vector<std::string> events;
};

void SingletonCachesRegistration() {
    AccessRuntime runtime;
    pal4::PalTextureDictionaryAccessState state;
    Check(pal4::PalTextureDictionaryAccess::Get(state, runtime) ==
              0x7000u && state.instance == 0x7000u &&
              state.guard == 1u,
          "cold singleton registers and caches texture dictionary");
    runtime.result = 0x8000u;
    Check(pal4::PalTextureDictionaryAccess::Get(state, runtime) ==
              0x7000u && runtime.manager_calls == 1 &&
              runtime.register_calls == 1,
          "warm singleton returns cached result");
    pal4::PalTextureDictionaryAccessState failed{0x80u, 0x9999u};
    runtime.result = 0u;
    Check(pal4::PalTextureDictionaryAccess::Get(failed, runtime) ==
              0u && failed.guard == 0x81u && failed.instance == 0u,
          "failed registration preserves other guard bits and caches null");
    Check(pal4::PalTextureDictionaryAccess::Get(failed, runtime) ==
              0u && runtime.register_calls == 2,
          "null singleton result is not retried");
}

void FactoryAndConstructionOrder() {
    ObjectRuntime runtime;
    Check(pal4::PalTextureDictionary::Create(runtime) == 0x1000u &&
              runtime.events == std::vector<std::string>({
                  "allocate", "base construct", "access"}),
          "factory invokes base constructor before derived vtable write");
    Check(runtime.state.vtable == 0x842174u &&
              runtime.state.texture_head == 0u &&
              runtime.state.flag == 1u,
          "derived constructor retains base-initialized fields");
    runtime.events.clear();
    Check(pal4::PalTextureDictionary::Construct(
              0x1000u, runtime) == 0x1000u &&
              runtime.events == std::vector<std::string>({
                  "base construct", "access"}),
          "direct constructor returns this and preserves call order");
}

void FactoryFailurePaths() {
    ObjectRuntime runtime;
    runtime.allocation_result = 0u;
    Check(pal4::PalTextureDictionary::Create(runtime) == 0u &&
              runtime.events == std::vector<std::string>{"allocate"},
          "allocation failure returns null");
    runtime.allocation_result = 0x1000u;
    runtime.events.clear();
    runtime.throw_on_construct = true;
    try {
        static_cast<void>(pal4::PalTextureDictionary::Create(runtime));
        Check(false, "base constructor exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.events == std::vector<std::string>({
                  "allocate", "base construct", "free"}),
              "factory frees allocation after base construction throws");
    }
    runtime.events.clear();
    runtime.throw_on_construct = false;
    runtime.throw_on_access = true;
    try {
        static_cast<void>(pal4::PalTextureDictionary::Create(runtime));
        Check(false, "derived state exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.events == std::vector<std::string>({
                  "allocate", "base construct", "access", "free"}),
              "factory frees allocation after derived construction throws");
    }
}

void DeletingDestructorHonorsFlagBit() {
    ObjectRuntime runtime;
    runtime.state = {0x842174u, 0x5000u, 1u};
    Check(pal4::PalTextureDictionary::Delete(
              0x1000u, 0x80u, runtime) == 0x1000u &&
              runtime.events == std::vector<std::string>({
                  "access", "base destruct"}),
          "flag bit clear destroys without freeing and returns this");
    Check(runtime.state.vtable == 0x84b940u &&
              runtime.state.texture_head == 0x5000u,
          "base destructor can replace vtable without touching other fields");
    runtime.state.vtable = 0x842174u;
    runtime.events.clear();
    Check(pal4::PalTextureDictionary::Delete(
              0x1000u, 0x81u, runtime) == 0x1000u &&
              runtime.events == std::vector<std::string>({
                  "access", "base destruct", "free"}),
          "flag bit set frees only after destruction");
}

void TextureThunkAndLinkableStub() {
    ObjectRuntime runtime;
    Check(pal4::PalTextureDictionary::AddTexture(
              0x1000u, 0x5000u, runtime) == 0x9999u &&
              runtime.events == std::vector<std::string>{"add texture"},
          "texture thunk forwards result unchanged");
    pal4::PalTextureDictionaryRuntimeStub stub;
    const auto address = pal4::PalTextureDictionary::Create(stub);
    Check(address != 0u &&
              stub.Access(address).vtable == 0x842174u &&
              pal4::PalTextureDictionary::AddTexture(
                  address, 0x5000u, stub) == 0u,
          "RenderWare empty stub links and constructs texture dictionary");
    static_cast<void>(pal4::PalTextureDictionary::Delete(
        address, 1u, stub));
}

}  // namespace

int main() {
    SingletonCachesRegistration();
    FactoryAndConstructionOrder();
    FactoryFailurePaths();
    DeletingDestructorHonorsFlagBit();
    TextureThunkAndLinkableStub();
    std::cout << "5 PAL texture dictionary test cases passed\n";
}
