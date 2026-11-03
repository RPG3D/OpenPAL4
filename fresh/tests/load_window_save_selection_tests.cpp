#include "pal4/load_window_save_selection.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::LoadWindowSaveSelectionRuntimeStub {
    std::uint32_t hover_item = 0x2100u;
    std::uint32_t slot = 0x1234u;
    std::uint32_t global_calls{};
    bool existing_type{};
    bool existing_archive{};
    bool throw_on_find{};
    bool throw_on_add{};
    bool throw_on_enter{};
    std::vector<std::string> calls;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> writes;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> set_values;
    std::vector<std::pair<std::string, std::uint32_t>> created;
    std::vector<std::uint32_t> released_keys;
    std::vector<std::uint32_t> destroyed_attributes;
    std::vector<std::uint32_t> destroyed_documents;

    std::uint32_t HoverItem(std::uint32_t list) override {
        Check(list == 0x5400u, "selection queries +0x54 list");
        calls.emplace_back("hover");
        return hover_item;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        if (address == 0x22b8u) return slot;
        Check(address == 0x34c0u,
              "scrollbar position is read at +0x4c0");
        return 0x55u;
    }
    std::uint32_t GlobalData() override {
        ++global_calls;
        calls.emplace_back("global");
        return 0x8000u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        writes.emplace_back(address, value);
        calls.emplace_back("write");
    }
    std::uint32_t ScrollbarOf(std::uint32_t list) override {
        Check(list == 0x5400u, "scrollbar belongs to list");
        calls.emplace_back("scrollbar");
        return 0x3000u;
    }
    void SaveDialogSettings(std::uint32_t global) override {
        Check(global == 0x8000u, "settings receive third global getter");
        calls.emplace_back("save-settings");
    }
    void HideWindow(pal4::LoadWindowState&,
                    std::uint32_t show, std::uint32_t parent) override {
        Check(show == 0u && parent == 0u,
              "confirmed load hides dialog with two zeros");
        calls.emplace_back("hide-window");
    }
    void ProcessUiEvents(std::string_view command,
                         std::uint32_t first,
                         std::uint32_t second) override {
        Check(command == "close_main_ui" &&
                  first == 0u && second == 0u,
              "UI event processing dispatches close_main_ui with two zeros");
        calls.emplace_back("process-ui");
    }
    std::uint32_t InitializeXmlDocument() override {
        calls.emplace_back("init-document");
        return 0x6000u;
    }
    std::uint32_t InitializeKey(const char* name,
                                std::uint8_t low_slot) override {
        Check(low_slot == static_cast<std::uint8_t>(slot),
              "temporary key inherits low byte of selected slot");
        const std::string key{name};
        Check(key == "PALGAMEIV_LOADING_TYPE" ||
                  key == "PALGAMEIV_LOADING_ARCHIVE",
              "attribute names match IDA strings");
        calls.emplace_back("init-key");
        return key == "PALGAMEIV_LOADING_TYPE" ? 0x4000u : 0x5000u;
    }
    pal4::LoadWindowAttributePosition FindAttribute(
        std::uint32_t document, std::uint32_t key) override {
        Check(document == 0x6000u &&
                  (key == 0x4000u || key == 0x5000u),
              "lookup uses document and temporary key");
        calls.emplace_back("find");
        if (throw_on_find) throw std::runtime_error("find");
        const bool exists = key == 0x4000u ?
            existing_type : existing_archive;
        return {exists ? key + 0x3100u : 0x7000u, 0x7000u};
    }
    void ReleaseKey(std::uint32_t key) override {
        released_keys.push_back(key);
        calls.emplace_back("release-key");
    }
    void SetAttributeInt(std::uint32_t field,
                         std::uint32_t value) override {
        set_values.emplace_back(field, value);
        calls.emplace_back("set-int");
    }
    std::uint32_t InitializeAttribute(
        const char* name, std::uint32_t value) override {
        created.emplace_back(name, value);
        calls.emplace_back("init-attribute");
        return created.size() == 1u ? 0x9000u : 0xa000u;
    }
    void AddAttribute(std::uint32_t document,
                      std::uint32_t attribute) override {
        Check(document == 0x6000u &&
                  (attribute == 0x9000u || attribute == 0xa000u),
              "new attribute is added to document");
        calls.emplace_back("add-attribute");
        if (throw_on_add) throw std::runtime_error("add");
    }
    void DestroyAttribute(std::uint32_t attribute) override {
        destroyed_attributes.push_back(attribute);
        calls.emplace_back("destroy-attribute");
    }
    void EnterGameState(std::uint32_t state_id,
                        std::uint32_t document) override {
        Check(state_id == 6u && document == 0x6000u,
              "confirmed save enters state six with XML document");
        calls.emplace_back("enter-state");
        if (throw_on_enter) throw std::runtime_error("enter");
    }
    void DestroyXmlDocument(std::uint32_t document) override {
        destroyed_documents.push_back(document);
        calls.emplace_back("destroy-document");
    }
};

pal4::LoadWindowState MakeState() {
    pal4::LoadWindowState state{};
    state.Write(0x54u, 0x5400u);
    return state;
}

void CheckGates() {
    auto state = MakeState();
    Runtime runtime;
    Check(pal4::LoadWindowSaveSelection::Confirm(state, 0u, runtime) == 1u &&
              runtime.calls.empty(),
          "non-confirmation returns without side effects");
    runtime.hover_item = 0u;
    Check(pal4::LoadWindowSaveSelection::Confirm(state, 1u, runtime) == 1u &&
              runtime.calls == std::vector<std::string>{"hover"},
          "missing hover item returns before global changes");
}

void CheckInsertedAttributes() {
    auto state = MakeState();
    Runtime runtime;
    Check(pal4::LoadWindowSaveSelection::Confirm(state, 1u, runtime) == 1u &&
              runtime.global_calls == 3u &&
              runtime.writes ==
                  (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                      {0x802cu, 0x1234u}, {0x8030u, 0x55u}}) &&
              runtime.created ==
                  (std::vector<std::pair<std::string, std::uint32_t>>{
                      {"PALGAMEIV_LOADING_TYPE", 1u},
                      {"PALGAMEIV_LOADING_ARCHIVE", 0x1234u}}) &&
              runtime.released_keys ==
                  (std::vector<std::uint32_t>{0x4000u, 0x5000u}) &&
              runtime.destroyed_attributes ==
                  (std::vector<std::uint32_t>{0x9000u, 0xa000u}) &&
              runtime.destroyed_documents ==
                  (std::vector<std::uint32_t>{0x6000u}),
          "confirmed save records slot, scroll and two loading attributes");
    Check(runtime.calls[0] == "hover" &&
              runtime.calls[1] == "read" &&
              runtime.calls[2] == "global" &&
              runtime.calls[3] == "write" &&
              runtime.calls[4] == "scrollbar" &&
              runtime.calls[5] == "read" &&
              runtime.calls[6] == "global" &&
              runtime.calls[7] == "write" &&
              runtime.calls[8] == "global" &&
              runtime.calls[9] == "save-settings" &&
              runtime.calls[10] == "hide-window" &&
              runtime.calls[11] == "process-ui" &&
              runtime.calls[12] == "init-document" &&
              runtime.calls[runtime.calls.size() - 2u] == "enter-state" &&
              runtime.calls.back() == "destroy-document",
          "save confirmation follows IDA call order");
}

void CheckUpdatedAttributes() {
    auto state = MakeState();
    Runtime runtime;
    runtime.existing_type = true;
    runtime.existing_archive = true;
    (void)pal4::LoadWindowSaveSelection::Confirm(state, 1u, runtime);
    Check(runtime.set_values ==
              (std::vector<std::pair<std::uint32_t, std::uint32_t>>{
                  {0x711cu, 1u}, {0x811cu, 0x1234u}}) &&
              runtime.created.empty() &&
              runtime.destroyed_documents.size() == 1u,
          "existing loading attributes are updated at node +0x1c");
}

void CheckExceptionCleanup() {
    auto state = MakeState();
    Runtime find_failure;
    find_failure.throw_on_find = true;
    try {
        (void)pal4::LoadWindowSaveSelection::Confirm(
            state, 1u, find_failure);
        Check(false, "find must throw");
    } catch (const std::runtime_error&) {
        Check(find_failure.released_keys ==
                  (std::vector<std::uint32_t>{0x4000u}) &&
                  find_failure.destroyed_documents ==
                  (std::vector<std::uint32_t>{0x6000u}),
              "failed find releases key and document");
    }
    Runtime add_failure;
    add_failure.throw_on_add = true;
    try {
        (void)pal4::LoadWindowSaveSelection::Confirm(
            state, 1u, add_failure);
        Check(false, "add must throw");
    } catch (const std::runtime_error&) {
        Check(add_failure.destroyed_attributes ==
                  (std::vector<std::uint32_t>{0x9000u}) &&
                  add_failure.destroyed_documents ==
                  (std::vector<std::uint32_t>{0x6000u}),
              "failed insertion releases attribute and document");
    }
    Runtime state_failure;
    state_failure.throw_on_enter = true;
    try {
        (void)pal4::LoadWindowSaveSelection::Confirm(
            state, 1u, state_failure);
        Check(false, "state entry must throw");
    } catch (const std::runtime_error&) {
        Check(state_failure.destroyed_attributes.size() == 2u &&
                  state_failure.destroyed_documents ==
                  (std::vector<std::uint32_t>{0x6000u}),
              "failed state entry releases XML document");
    }
}

}  // namespace

int main() {
    CheckGates();
    CheckInsertedAttributes();
    CheckUpdatedAttributes();
    CheckExceptionCleanup();
    std::cout << "Load window save selection tests passed\n";
}
