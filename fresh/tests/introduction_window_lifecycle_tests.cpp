#include "pal4/introduction_window_factory.h"
#include "pal4/introduction_window_lifecycle.h"

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

struct InsertRecord {
    std::uint32_t collection{};
    std::uint32_t source{};
    std::uint32_t enabled{};
    std::uint32_t id{};
    bool first{};
};

struct Runtime final : pal4::IntroductionWindowLifecycleRuntimeStub {
    pal4::IntroductionWindowState* state{};
    std::string fail_at;
    std::uint32_t fail_id{};
    bool fail_destroy_collections{};
    bool fail_free_member{};
    std::uint32_t last_freed = 0xffffffffu;
    std::vector<std::string> calls;
    std::vector<InsertRecord> inserts;

    void MaybeFail(const char* stage) {
        if (fail_at == stage) throw std::runtime_error(stage);
    }
    void LoadLayout(pal4::IntroductionWindowState& object,
                    std::uint32_t name, std::uint32_t first,
                    std::uint32_t second) override {
        Check(name == 0x5000u && first == 0u && second == 0u,
              "layout receives name and two zero flags");
        state = &object;
        calls.emplace_back("layout");
        MaybeFail("layout");
    }
    void ConstructMember(pal4::IntroductionWindowState& object,
                         std::uint32_t offset) override {
        Check(&object == state && offset == 0x28u,
              "member constructor uses object +0x28");
        calls.emplace_back("member");
        MaybeFail("member");
    }
    void ConstructCollections(pal4::IntroductionWindowState& object,
                              std::uint32_t offset,
                              std::uint32_t element_size,
                              std::uint32_t count) override {
        Check(&object == state && offset == 0x40u &&
                  element_size == 0x10u && count == 4u,
              "vector constructor creates four 16-byte collection members");
        calls.emplace_back("collections");
        MaybeFail("collections");
        object.Write(0x48u, 0xa000u);
        object.Write(0x58u, 0xb000u);
        object.Write(0x68u, 0xc000u);
        object.Write(0x78u, 0xd000u);
    }
    void InitializePanelElements(
        pal4::IntroductionWindowState& object) override {
        Check(object.Read(0xb0u) ==
                  pal4::IntroductionWindowLifecycle::kTailVtable &&
                  object.Read(0u) ==
                  pal4::IntroductionWindowLifecycle::kObjectVtable &&
                  object.Read(0x28u) ==
                  pal4::IntroductionWindowLifecycle::kMemberVtable,
              "three vtables are written before panel initialization");
        calls.emplace_back("elements");
        MaybeFail("elements");
    }
    void InsertFirst(std::uint32_t collection,
                     std::uint32_t source,
                     std::uint32_t id) override {
        Check(collection == 0x1040u && source == 0xa000u && id == 1u,
              "first element uses collection +0x40 and id one");
        calls.emplace_back("first");
        inserts.push_back({collection, source, 0u, id, true});
        state->Write(0x48u, source + 1u);
        if (fail_id == id) throw std::runtime_error("insert");
    }
    void InsertNext(std::uint32_t collection,
                    std::uint32_t source,
                    std::uint32_t enabled,
                    std::uint32_t id) override {
        Check(enabled == 1u, "following insertions set flag one");
        inserts.push_back({collection, source, enabled, id, false});
        calls.emplace_back("insert");
        const auto source_offset = collection - 0x1000u + 8u;
        state->Write(source_offset, source + 1u);
        if (fail_id == id) throw std::runtime_error("insert");
    }
    void DestroyCollections(pal4::IntroductionWindowState&,
                            std::uint32_t offset,
                            std::uint32_t element_size,
                            std::uint32_t count) override {
        Check(offset == 0x40u && element_size == 0x10u && count == 4u,
              "unwind destroys constructed collection array");
        calls.emplace_back("destroy-collections");
        if (fail_destroy_collections) {
            throw std::runtime_error("destroy-collections");
        }
    }
    void CleanupMember(pal4::IntroductionWindowState&,
                       std::uint32_t offset) override {
        Check(offset == 0x28u, "unwind cleans constructed member");
        calls.emplace_back("cleanup-member");
    }
    void FreeMemberStorage(std::uint32_t address) override {
        last_freed = address;
        calls.emplace_back("free-member");
        if (fail_free_member) throw std::runtime_error("free-member");
    }
    std::uint32_t DestroyDialog(
        pal4::IntroductionWindowState&) override {
        calls.emplace_back("destroy-dialog");
        return 0x9000u;
    }
};

void CheckSuccessAndFactoryIntegration() {
    pal4::IntroductionWindowState state{};
    Runtime runtime;
    Check(&pal4::IntroductionWindowLifecycle::Construct(
              state, 0x1000u, 0x5000u, runtime) == &state &&
              runtime.calls[0] == "layout" &&
              runtime.calls[1] == "member" &&
              runtime.calls[2] == "collections" &&
              runtime.calls[3] == "elements" &&
              runtime.inserts.size() == 16u &&
              state.Read(0xb0u) ==
                  pal4::IntroductionWindowLifecycle::kTailVtable,
          "constructor initializes members before sixteen associations");

    const std::vector<std::uint32_t> expected_ids{
        1u, 2u, 8u, 9u, 10u, 11u, 3u, 4u,
        5u, 6u, 7u, 12u, 13u, 14u, 15u, 16u};
    for (std::size_t i = 0; i < expected_ids.size(); ++i) {
        const auto& item = runtime.inserts[i];
        const auto expected_collection =
            i < 2u ? 0x1040u : i < 6u ? 0x1050u :
            i < 11u ? 0x1060u : 0x1070u;
        const auto base_source =
            i < 2u ? 0xa000u : i < 6u ? 0xb000u :
            i < 11u ? 0xc000u : 0xd000u;
        const auto group_start =
            i < 2u ? 0u : i < 6u ? 2u : i < 11u ? 6u : 11u;
        Check(item.id == expected_ids[i] &&
                  item.collection == expected_collection &&
                  item.source == base_source +
                                     static_cast<std::uint32_t>(i - group_start) &&
                  item.first == (i == 0u),
              "each ID uses its collection and freshly read source pointer");
    }

    struct FactoryRuntime final : pal4::IntroductionWindowFactoryRuntimeStub {
        pal4::IntroductionWindowState object{};
        Runtime lifecycle;
        std::uint32_t Allocate(std::size_t bytes) override {
            Check(bytes == 0xb8u, "factory requests constructor object size");
            return 0x1000u;
        }
        std::uint32_t Construct(std::uint32_t storage,
                                std::uint32_t name) override {
            (void)pal4::IntroductionWindowLifecycle::Construct(
                object, storage, name, lifecycle);
            return storage;
        }
    } factory;
    Check(pal4::IntroductionWindowFactory::CreateObject(
              0x5000u, factory) == 0x1000u &&
              factory.lifecycle.inserts.size() == 16u,
          "registered factory can reach the reconstructed constructor");
}

void CheckUnwind() {
    for (const auto& scenario : std::vector<std::string>{
             "layout", "member", "collections", "elements", "insert"}) {
        pal4::IntroductionWindowState state{};
        Runtime runtime;
        if (scenario == "insert") {
            runtime.fail_id = 9u;
        } else {
            runtime.fail_at = scenario;
        }
        try {
            (void)pal4::IntroductionWindowLifecycle::Construct(
                state, 0x1000u, 0x5000u, runtime);
            Check(false, "configured constructor stage must throw");
        } catch (const std::runtime_error&) {
            const std::vector<std::string> expected_tail =
                scenario == "layout" ? std::vector<std::string>{} :
                scenario == "member" ?
                    std::vector<std::string>{"destroy-dialog"} :
                scenario == "collections" ?
                    std::vector<std::string>{
                        "cleanup-member", "destroy-dialog"} :
                    std::vector<std::string>{
                        "destroy-collections", "cleanup-member",
                        "destroy-dialog"};
            Check(runtime.calls.size() >= expected_tail.size(),
                  "cleanup stage count is present");
            for (std::size_t i = 0; i < expected_tail.size(); ++i) {
                Check(runtime.calls[runtime.calls.size() -
                                    expected_tail.size() + i] ==
                          expected_tail[i],
                      "constructor unwind follows completed stages in reverse");
            }
            if (scenario == "layout") {
                Check(runtime.calls == std::vector<std::string>{"layout"},
                      "failed layout has no constructed base to destroy");
            }
        }
    }
}

void CheckDestruction() {
    pal4::IntroductionWindowState state{};
    state.Write(0x30u, 0x3000u);
    state.Write(0x34u, 9u);
    state.Write(0x38u, 11u);
    state.Write(0x3cu, 13u);
    Runtime runtime;
    Check(pal4::IntroductionWindowLifecycle::Destroy(state, runtime) ==
              0x9000u &&
              runtime.last_freed == 0x3000u &&
              state.Read(0x30u) == 0u && state.Read(0x34u) == 0u &&
              state.Read(0x38u) == 0u && state.Read(0x3cu) == 13u &&
              runtime.calls == std::vector<std::string>{
                  "destroy-collections", "free-member", "destroy-dialog"},
          "destructor releases collections then member storage and dialog");

    runtime = Runtime{};
    runtime.fail_destroy_collections = true;
    state.Write(0x30u, 0x3000u);
    try {
        (void)pal4::IntroductionWindowLifecycle::Destroy(state, runtime);
        Check(false, "collection destructor failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "destroy-collections", "cleanup-member", "destroy-dialog"},
              "destructor exception unwinds member and dialog");
    }

    runtime = Runtime{};
    runtime.fail_free_member = true;
    try {
        (void)pal4::IntroductionWindowLifecycle::Destroy(state, runtime);
        Check(false, "member free failure must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.calls == std::vector<std::string>{
                  "destroy-collections", "free-member", "cleanup-member",
                  "destroy-dialog"},
              "member free exception also unwinds member and dialog");
    }
}

}  // namespace

int main() {
    CheckSuccessAndFactoryIntegration();
    CheckUnwind();
    CheckDestruction();
    std::cout << "Introduction window lifecycle tests passed\n";
}
