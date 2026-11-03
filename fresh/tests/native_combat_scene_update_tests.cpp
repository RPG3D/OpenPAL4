#include "pal4/native_combat_scene_update.h"
#include "pal4/native_combat_system_image.h"
#include "pal4/native_event_document_runtime.h"
#include "pal4/native_base_object_memory.h"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void Check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
template<class F> void Reject(F f) {
    bool rejected{}; try { f(); } catch (const std::exception&) { rejected = true; }
    Check(rejected, "expected service failure or reentry rejection");
}
void Run() {
    pal4::NativeBaseObjectMemory memory;
    pal4::NativeEventDocumentRuntime documents(memory);
    pal4::NativeCombatSystemImage image;
    const auto retained = documents.Allocate(60u);
    (void)documents.InitializeEmptyDocument(retained);
    documents.WriteIntAttribute(retained, "CombatResult", -55);
    const auto baseline = documents.DocumentCount();
    std::vector<unsigned> calls;
    unsigned next{}, result{}, fail{};
    bool finish_in_ui{}, recurse{};
    pal4::NativeCombatSceneUpdate* frame{};
    pal4::NativeCombatSceneUpdateBindings bindings;
    bindings.combat = [&](float delta) {
        Check(delta == 0.25f, "original float combat delta"); calls.push_back(1);
        if (fail == 1) throw std::runtime_error("combat failure");
    };
    bindings.ui = [&](double delta) {
        Check(delta == 0.25, "promoted UI delta"); calls.push_back(2);
        if (fail == 2) throw std::runtime_error("UI failure");
        if (finish_in_ui) image.State() = 7u;
    };
    bindings.cleanup_script_cache = [&] {
        calls.push_back(3);
        Check(documents.DocumentCount() == baseline, "failure cache cleanup precedes temporary XML");
        if (fail == 3) throw std::runtime_error("script cleanup failure");
    };
    bindings.transition = [&](unsigned state, unsigned document) {
        calls.push_back(4); next = state;
        Check(documents.DocumentCount() == baseline + 1 && document != retained,
              "transition borrows one actual independent XML document");
        const auto attribute = documents.FindNumericAttribute(document, "CombatResult");
        if (state == 7) Check(!attribute && !documents.ChildCount(document), "alternate loss passes empty XML");
        else {
            Check(attribute && documents.AttributeToInt(attribute, -1) == static_cast<int>(result),
                  "outcome inserts normalized original CombatResult into fresh XML");
            documents.AssignDocument(retained, document);
        }
        if (recurse) Reject([&] { (void)frame->Update(0.25f); });
        if (fail == 4) throw std::runtime_error("transition failure");
    };
    auto missing = bindings; missing.transition = {};
    Reject([&] { pal4::NativeCombatSceneUpdate invalid(image, documents, missing); });
    pal4::NativeCombatSceneUpdate runtime(image, documents, bindings); frame = &runtime;
    image.State() = 1; Check(runtime.Update(0.25f) == 1 && calls == std::vector<unsigned>{1,2}, "nonterminal frame creates no XML");
    finish_in_ui = true; image.session_storage[27] = 2; result = 1; calls.clear(); recurse = true;
    Check(runtime.Update(0.25f) == 0 && next == 1 && calls == std::vector<unsigned>{1,2,4},
          "UI synchronous completion is observed in same frame and supports borrowed transition");
    finish_in_ui = false; recurse = false;
    for (unsigned alternate : {0u,1u}) for (unsigned outcome : {0u,1u,2u}) {
        reinterpret_cast<std::uint8_t*>(&image)[500u] = static_cast<std::uint8_t>(alternate);
        image.Mode() = static_cast<std::uint8_t>(!alternate); //516 is not alternate500.
        image.State() = 7; image.session_storage[27] = outcome; result = outcome != 0;
        calls.clear();
        Check(runtime.Update(0.25f) == 0 && documents.DocumentCount() == baseline &&
              next == ((alternate && !outcome) ? 7u : 1u) &&
              calls == (alternate && !outcome ? std::vector<unsigned>{1,2,3,4} : std::vector<unsigned>{1,2,4}),
              "normal/alternate outcomes preserve frame, cleanup and destination ordering");
    }
    for (fail = 1; fail <= 4; ++fail) {
        reinterpret_cast<std::uint8_t*>(&image)[500u] = 1u;
        image.session_storage[27] = fail == 3 ? 0u : 1u; result = 1; calls.clear();
        Reject([&] { (void)runtime.Update(0.25f); });
        Check(documents.DocumentCount() == baseline, "every service failure releases temporary XML");
        Check(calls.size() == (fail == 1 ? 1u : fail == 2 ? 2u : 3u), "failure stops later phases");
    }
    fail = 0; image.session_storage[27] = 1; result = 1;
    (void)runtime.Update(0.25f);
    Check(documents.AttributeToInt(documents.FindNumericAttribute(retained,"CombatResult"), -1) == 1,
          "failed frames recover; copied result survives source destruction");
    documents.DestroyDocument(retained); (void)documents.Free(retained);
    Check(!documents.DocumentCount() && !memory.OutstandingAllocations(), "all native XML allocations retire");
}
}
int main() { try { Run(); std::cout << "Native Combat scene update: real XML, all outcomes, same-frame UI finish, exceptions and reentry pass; frame/state callbacks are fixtures\n"; }
catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; } }
