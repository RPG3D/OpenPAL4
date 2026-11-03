#include "pal4/atomic_vertex_color.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IAtomicVertexColorRuntime {
    static constexpr std::uint32_t clump = 0x1000u;
    static constexpr std::uint32_t atomic = 0x2000u;
    static constexpr std::uint32_t geometry = 0x3000u;
    static constexpr std::uint32_t locked = 0x4000u;
    static constexpr std::uint32_t colors = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {atomic + 24u, geometry}, {geometry + 8u, 0x18u},
        {locked + 48u, colors}, {locked + 20u, 3u}};
    std::vector<std::string> events;

    [[nodiscard]] std::uint32_t ReadWord(const std::uint32_t address) override {
        events.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        events.emplace_back("write:" + std::to_string(address));
        words[address] = value;
    }
    [[nodiscard]] std::uint32_t LockGeometry(
        const std::uint32_t value,
        const std::uint32_t flags) override {
        Check(value == geometry && flags == 8u,
              "locks original geometry for prelight colors");
        events.emplace_back("lock");
        return locked;
    }
    void UnlockGeometry(const std::uint32_t value) override {
        Check(value == geometry, "unlocks original geometry pointer");
        events.emplace_back("unlock");
    }
    [[nodiscard]] std::uint32_t ForEachAtomic(
        const std::uint32_t value, const pal4::VertexColorPass pass,
        const std::uint32_t* argument) override {
        Check(value == clump && argument != nullptr,
              "clump passes address of color or mask");
        if (pass == pal4::VertexColorPass::set) {
            events.emplace_back("traverse:set");
            Check(pal4::AtomicVertexColor::SetAtomicColor(
                      atomic, *argument, *this) == atomic,
                  "set callback returns atomic");
        } else {
            events.emplace_back("traverse:clear");
            Check(pal4::AtomicVertexColor::ClearAtomicColor(
                      atomic, *argument, *this) == atomic,
                  "clear callback returns atomic");
        }
        return 0x7777u;
    }
};

}  // namespace

int main() {
    using Color = pal4::AtomicVertexColor;
    Runtime runtime;
    Check(Color::SetClumpColor(Runtime::clump, 0xaabbccddu, runtime) ==
              0x7777u &&
              runtime.words[Runtime::colors] == 0xaabbccddu &&
              runtime.words[Runtime::colors + 4u] == 0xaabbccddu &&
              runtime.words[Runtime::colors + 8u] == 0xaabbccddu &&
              runtime.events.front() == "traverse:set" &&
              runtime.events.back() == "unlock",
          "clump callback writes every vertex color and forwards traversal result");
    runtime.events.clear();
    runtime.words[Runtime::geometry + 8u] = 0x10u;
    Check(Color::SetAtomicColor(Runtime::atomic, runtime.words[Runtime::colors],
                                runtime) == Runtime::atomic &&
              runtime.events.size() == 2u,
          "geometry without prelight flag skips lock and writes");
    runtime.events.clear();
    runtime.words[Runtime::geometry + 8u] = 0x18u;
    runtime.words[Runtime::locked + 48u] = 0u;
    Check(Color::SetAtomicColor(Runtime::atomic, runtime.words[Runtime::colors],
                                runtime) == Runtime::atomic &&
              runtime.words[Runtime::locked + 8u] == 0x10u &&
              runtime.events.back() ==
                  "write:" + std::to_string(Runtime::locked + 8u),
          "missing color array clears locked flags without unlocking");
    runtime.events.clear();
    runtime.words[Runtime::locked + 48u] = Runtime::colors;
    runtime.words[Runtime::locked + 20u] = 0u;
    Check(Color::SetAtomicColor(Runtime::atomic, runtime.words[Runtime::colors],
                                runtime) == Runtime::atomic &&
              runtime.events.back() == "unlock",
          "zero vertex count still unlocks geometry");
    runtime.events.clear();
    runtime.words[Runtime::locked + 20u] = 0xffffffffu;
    Check(Color::SetAtomicColor(Runtime::atomic, runtime.words[Runtime::colors],
                                runtime) == Runtime::atomic &&
              runtime.events.back() == "unlock",
          "signed negative vertex count skips color writes");
    runtime.events.clear();
    runtime.words[Runtime::geometry + 8u] = 0x1fu;
    Check(Color::ClearClumpColor(Runtime::clump, 0x0au, runtime) ==
              0x7777u &&
              runtime.words[Runtime::geometry + 8u] == 0x15u &&
              runtime.events.front() == "traverse:clear" &&
              runtime.events.back() ==
                  "write:" + std::to_string(Runtime::geometry + 8u),
          "clump clear masks geometry flags and forwards traversal result");
    pal4::AtomicVertexColorRuntimeStub stub;
    Check(Color::SetClumpColor(1u, 2u, stub) == 0u &&
              Color::ClearClumpColor(1u, 2u, stub) == 0u,
          "RenderWare traversal stub links");
    return EXIT_SUCCESS;
}
