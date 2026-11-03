#include "pal4/ol_info_elements.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Write {
    std::uint32_t address;
    std::uint32_t value;
};

struct Runtime final : pal4::OlInfoElementsRuntimeStub {
    std::vector<std::string> names;
    std::vector<Write> writes;
    std::vector<std::uint32_t> counts;
    std::uint32_t initialized{};
    std::uint32_t cleaned{};
    std::uint32_t subscribed_window{};
    std::uint32_t subscribed_callback{};
    std::uint32_t deleted_reference{};
    std::uint32_t allocation = 0x5000u;
    std::uint32_t throw_lookup_index = 0xffffffffu;

    void InitializeNameBuffer() override { ++initialized; }
    std::string_view LayoutPrefix(const pal4::OlInfoState& state) override {
        Check(state.Read(0x14u) == 0x1234u,
              "control names use dialog layout prefix at +0x14");
        return "olInfo/";
    }
    std::uint32_t FindWindow(std::string_view name) override {
        if (names.size() == throw_lookup_index)
            throw std::runtime_error("lookup");
        names.emplace_back(name);
        return 0x3000u + static_cast<std::uint32_t>(names.size());
    }
    std::uint32_t AllocateCallback(std::size_t bytes) override {
        Check(bytes == 0x18u, "close callback allocates 24 bytes");
        return allocation;
    }
    void WriteCallbackWord(std::uint32_t address,
                           std::uint32_t value) override {
        writes.push_back({address, value});
    }
    std::uint32_t SubscribeClicked(std::uint32_t window,
                                   std::uint32_t callback) override {
        subscribed_window = window;
        subscribed_callback = callback;
        return 0x7000u;
    }
    std::uint32_t ReferenceCount(std::uint32_t reference) override {
        Check(reference == 0x7000u, "subscription returns local reference");
        return 1u;
    }
    void SetReferenceCount(std::uint32_t reference,
                           std::uint32_t count) override {
        Check(reference == 0x7000u, "reference count belongs to subscription");
        counts.push_back(count);
    }
    void DeleteReference(std::uint32_t reference,
                         std::uint32_t flags) override {
        Check(flags == 1u, "zero reference uses deleting flag one");
        deleted_reference = reference;
    }
    std::uint32_t CleanupNameBuffer() override {
        ++cleaned;
        return 0xcafeu;
    }
};

pal4::OlInfoState MakeState() {
    pal4::OlInfoState state{};
    state.Write(0x14u, 0x1234u);
    return state;
}

}  // namespace

int main() {
    auto state = MakeState();
    Runtime runtime;
    Check(pal4::OlInfoElements::Initialize(state, 0x900u, runtime) ==
                  0xcafeu && runtime.initialized == 1u &&
                  runtime.cleaned == 1u && runtime.names.size() == 10u &&
                  runtime.names[0] == "olInfo/BtnClose" &&
                  runtime.names[1] == "olInfo/StaticDesc" &&
                  state.Read(0x50u) == 0x3001u &&
                  state.Read(0x4cu) == 0x3002u &&
                  runtime.subscribed_window == 0x3001u &&
                  runtime.subscribed_callback == 0x5000u &&
                  runtime.counts == std::vector<std::uint32_t>{0u} &&
                  runtime.deleted_reference == 0x7000u,
          "close, description and eight rows are resolved in order");
    Check(runtime.writes.size() == 4u &&
              runtime.writes[0].address == 0x5008u &&
              runtime.writes[0].value == pal4::OlInfoElements::kCloseCallback &&
              runtime.writes[1].address == 0x500cu &&
              runtime.writes[1].value == 0u &&
              runtime.writes[2].address == 0x5010u &&
              runtime.writes[2].value == 0x900u &&
              runtime.writes[3].address == 0x5000u &&
              runtime.writes[3].value == pal4::OlInfoElements::kCallbackVtable,
          "close callback field offsets and write order match IDA");
    for (std::uint32_t index = 0u; index < 8u; ++index) {
        Check(runtime.names[index + 2u] ==
                  "olInfo/StaticInfo" + std::to_string(index + 1u) &&
                  state.Read(0x2cu + index * 4u) == 0x3003u + index,
              "StaticInfo1 through StaticInfo8 use sequential slots");
    }

    runtime = Runtime{};
    runtime.allocation = 0u;
    Check(pal4::OlInfoElements::Initialize(state, 0x900u, runtime) ==
                  0xcafeu && runtime.subscribed_callback == 0u &&
                  runtime.writes.empty(),
          "null callback allocation still subscribes clicked event");
    runtime = Runtime{};
    runtime.throw_lookup_index = 5u;
    try {
        (void)pal4::OlInfoElements::Initialize(state, 0x900u, runtime);
        Check(false, "lookup exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.names.size() == 5u && runtime.cleaned == 1u,
              "lookup exception cleans temporary name buffer");
    }
    std::cout << "olInfo elements tests passed\n";
}
