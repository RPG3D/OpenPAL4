#include "pal4/select_dialog.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISelectDialogCloseRuntime,
                       pal4::ISelectDialogCallbackRuntime {
    static constexpr std::uint32_t kObject = 0x1000u;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::vector<std::pair<std::uint32_t,
                          std::uint32_t>> removals;
    std::vector<std::uint32_t> available;
    std::vector<std::pair<std::uint32_t,
                          std::uint32_t>> freed_strings;
    bool modal_disabled = false;

    Runtime() {
        words[kObject + 36u] = 0x2000u;
        words[0x2000u + 68u] = 0x2100u;
        words[kObject + 72u] = 0x3000u;
        words[kObject + 76u] = 0x3000u;
        words[kObject + 80u] = 0x3100u;
        words[kObject + 92u] = 0x4100u;
        words[kObject + 104u] = 0x4000u;
        words[kObject + 108u] = 0x4000u;
        words[kObject + 152u] = 0x2200u;
        words[kObject + 156u] = 7u;
        words[kObject + 160u] = 3u;
        words[kObject + 64u] = 0u;
    }

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    void SetModalState(std::uint32_t window,
                       bool enabled) override {
        Check(window == 0x2000u && !enabled,
              "modal close arguments");
        modal_disabled = true;
        events.emplace_back("modal-off");
    }
    void RemoveChild(std::uint32_t parent,
                     std::uint32_t child) override {
        events.emplace_back("remove");
        removals.emplace_back(parent, child);
    }
    void AppendAvailableButton(std::uint32_t collection,
                               std::uint32_t storage,
                               std::uint32_t button) override {
        Check(collection == kObject + 84u &&
                  storage == 0x4100u,
              "available collection arguments");
        events.emplace_back("available");
        available.push_back(button);
    }
    void FreeString(std::uint32_t record,
                    std::uint32_t allocation,
                    std::uint32_t size) override {
        events.emplace_back("free-string");
        freed_strings.emplace_back(allocation, size);
        Check(record >= 0x3000u && record < 0x3050u,
              "freed option record");
    }
    void DestroyCallback(std::uint32_t callback,
                         std::uint32_t flags) override {
        Check(callback == 0x6000u && flags == 1u,
              "callback destruction");
        events.emplace_back("destroy-callback");
    }
    std::uint32_t Free(std::uint32_t allocation) override {
        Check(allocation == 0x5000u,
              "callback allocation freed");
        events.emplace_back("free-callback");
        return 0xbeefu;
    }
};

void CheckEmpty() {
    Runtime runtime;
    Check(pal4::SelectDialog::HideAndClearList(
              Runtime::kObject, runtime) == 0x3000u,
          "empty close return");
    Check(runtime.modal_disabled &&
              runtime.removals ==
                  std::vector<std::pair<std::uint32_t,
                                        std::uint32_t>>{{0x2100u,
                                                         0x2000u}} &&
              runtime.available.empty(),
          "empty close detaches root");
    Check(runtime.words.at(Runtime::kObject + 160u) == 0u &&
              runtime.words.at(Runtime::kObject + 156u) == 7u &&
              runtime.words.at(Runtime::kObject + 80u) ==
                  0x3100u &&
              runtime.words.at(Runtime::kObject + 76u) ==
                  0x3000u,
          "close preserves selection and option capacity");
    Check(runtime.events.front() == "modal-off" &&
              runtime.events.back() == "remove",
          "empty close operation order");
}

void CheckButtonsAndStrings() {
    Runtime runtime;
    runtime.words[Runtime::kObject + 108u] = 0x4008u;
    runtime.words[0x4000u] = 0x7000u;
    runtime.words[0x4004u] = 0x7100u;
    runtime.words[Runtime::kObject + 76u] = 0x3040u;
    runtime.words[0x3004u] = 0x8001u;
    runtime.words[0x300cu] = 4u;
    runtime.bytes[0x8000u] = 0u;
    runtime.words[0x3014u] = 0x8101u;
    runtime.words[0x301cu] = 5u;
    runtime.bytes[0x8100u] = 3u;
    runtime.words[0x3024u] = 0x8201u;
    runtime.words[0x302cu] = 6u;
    runtime.bytes[0x8200u] = 0xffu;
    runtime.words[0x3034u] = 0u;

    Check(pal4::SelectDialog::HideAndClearList(
              Runtime::kObject, runtime) == 0x3030u,
          "nonempty close returns final record");
    Check(runtime.available ==
              std::vector<std::uint32_t>{0x7000u, 0x7100u} &&
              runtime.removals ==
                  std::vector<std::pair<std::uint32_t,
                                        std::uint32_t>>{
                      {0x2100u, 0x2000u},
                      {0x2200u, 0x7000u},
                      {0x2200u, 0x7100u}} &&
              runtime.words.at(Runtime::kObject + 108u) ==
                  0x4000u,
          "buttons returned to spare collection");
    Check(runtime.freed_strings ==
              std::vector<std::pair<std::uint32_t,
                                    std::uint32_t>>{
                  {0x8000u, 6u}, {0x8200u, 8u}} &&
              runtime.bytes.at(0x8100u) == 2u,
          "option buffer reference handling");
    for (auto record = 0x3000u; record < 0x3040u;
         record += 16u) {
        Check(runtime.words.at(record + 4u) == 0u &&
                  runtime.words.at(record + 8u) == 0u &&
                  runtime.words.at(record + 12u) == 0u,
              "option buffer record cleared");
    }
    Check(runtime.words.at(Runtime::kObject + 76u) ==
              0x3000u &&
              runtime.words.at(Runtime::kObject + 80u) ==
                  0x3100u &&
              runtime.words.at(Runtime::kObject + 156u) == 7u,
          "option end reset while capacity and selected index remain");
}

void CheckNoParent() {
    Runtime runtime;
    runtime.words[0x2000u + 68u] = 0u;
    (void)pal4::SelectDialog::HideAndClearList(
        Runtime::kObject, runtime);
    Check(runtime.removals.empty(),
          "unattached root needs no removal");
}

void CheckCallback() {
    Runtime runtime;
    Check(pal4::SelectDialog::CleanupCallback(
              Runtime::kObject, runtime) == 0u &&
              runtime.events.empty(),
          "empty callback cleanup");
    runtime.words[Runtime::kObject + 64u] = 0x5000u;
    runtime.words[0x5000u] = 0x6000u;
    Check(pal4::SelectDialog::CleanupCallback(
              Runtime::kObject, runtime) == 0xbeefu &&
              runtime.words.at(Runtime::kObject + 64u) == 0u &&
              runtime.events ==
                  std::vector<std::string>{
                      "destroy-callback", "free-callback"},
          "owned callback destruction and free");

    Runtime no_callback;
    no_callback.words[Runtime::kObject + 64u] = 0x5000u;
    no_callback.words[0x5000u] = 0u;
    Check(pal4::SelectDialog::CleanupCallback(
              Runtime::kObject, no_callback) == 0xbeefu &&
              no_callback.events ==
                  std::vector<std::string>{"free-callback"},
          "null callback allocation still freed");

    Runtime constructed;
    Check(pal4::SelectDialog::ConstructCallback(
              0x5000u, 0x1234u, 9u,
              Runtime::kObject, constructed) == 0x5000u &&
              constructed.words.at(0x5000u) ==
                  pal4::SelectDialog::kCallbackVtable &&
              constructed.words.at(0x5008u) == 0x1234u &&
              constructed.words.at(0x500cu) == 9u &&
              constructed.words.at(0x5010u) ==
                  Runtime::kObject,
          "callback constructor writes exact fields");
}

struct EventRuntime final : pal4::ISelectDialogEventRuntime, pal4::IUiNavigationInputRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t forwarded_event = 0u;
    std::uint32_t invoked_index = 0xffffffffu;
    std::uint32_t expected_selection=3u, centered=0u;
    void WriteByte(std::uint32_t,std::uint8_t)override{}
    std::uint8_t ReadByte(std::uint32_t)override{return 0u;}
    void RequestRedraw(std::uint32_t)override{}
    void SetButtonSelected(std::uint32_t,bool)override{}
    bool IsDisabled(std::uint32_t)override{return false;}
    bool IsVisible(std::uint32_t)override{return true;}
    std::uint32_t InvokeCycle(std::uint32_t list,std::uint32_t slot,std::uint32_t previous)override{
        return slot==24u?pal4::UiNavigationList::CycleNext(list,previous,*this):pal4::UiNavigationList::CyclePrevious(list,previous,*this);
    }
    void MouseAtCenter(std::uint32_t window)override{centered=window;}

    EventRuntime() {
        words[0x8000u + 8u] = 0x9000u;
        words[0x9000u + 280u] = 3u;
        words[Runtime::kObject + 64u] = 0x5000u;
        words[0x5000u] = 0x6000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        events.emplace_back("write");
    }
    void ForwardNavigationKey(std::uint32_t navigation,
                              std::uint32_t event) override {
        Check(navigation == Runtime::kObject + 40u,
              "key navigation address");
        forwarded_event = event;
        events.emplace_back("key");
    }
    void HideAndClearList(std::uint32_t object) override {
        Check(object == Runtime::kObject &&
                  words.at(object + 156u) == expected_selection,
              "selection saved before close");
        events.emplace_back("hide");
    }
    void InvokeSelectionCallback(std::uint32_t callback,
                                 std::uint32_t index) override {
        Check(callback == 0x6000u,
              "selection callback target");
        invoked_index = index;
        events.emplace_back("invoke");
    }
    void CleanupCallback(std::uint32_t object) override {
        Check(object == Runtime::kObject,
              "event callback cleanup owner");
        events.emplace_back("cleanup");
    }
};

void CheckNavigation() {
    for(const auto default_selection:{0u,1u,0xffffffffu})for(const bool callback:{false,true}){
        EventRuntime r;r.words[0x8010u]=1u;r.words[Runtime::kObject+160u]=default_selection;
        r.expected_selection=default_selection;if(!callback)r.words[Runtime::kObject+64u]=0u;
        Check(pal4::SelectDialog::HandleNavigationKey(Runtime::kObject,0x8000u,r,r)==1u &&
            r.words[Runtime::kObject+156u]==default_selection &&
            r.events==(callback?std::vector<std::string>{"write","hide","invoke","cleanup"}:std::vector<std::string>{"write","hide","cleanup"}),
            "Esc uses exact signed default result and close/callback ordering without navigation");
        if(callback)Check(r.invoked_index==default_selection,"Esc callback receives original default including -1");
    }
    EventRuntime r;const auto list=Runtime::kObject+40u;
    r.words[list+8u]=0xa000u;r.words[list+12u]=0xa008u;r.words[list+20u]=0u;
    r.words[0xa000u]=0xb000u;r.words[0xa004u]=0xb100u;r.words[0x8010u]=208u;
    Check(pal4::SelectDialog::HandleNavigationKey(Runtime::kObject,0x8000u,r,r)==1u && r.words[list+20u]==1u,"SelectDialog delegates Down to actual shared vertical cycle");
    r.words[0x8010u]=203u;
    Check(pal4::SelectDialog::HandleNavigationKey(Runtime::kObject,0x8000u,r,r)==0u && r.words[list+20u]==1u,"SelectDialog preserves vertical navigation rejection of Left");
}

void CheckEvents() {
    EventRuntime runtime;
    Check(pal4::SelectDialog::HandleKeyDown(
              Runtime::kObject, 0x8000u, runtime) == 1u &&
              runtime.forwarded_event == 0x8000u &&
              runtime.events ==
                  std::vector<std::string>{"key"},
          "key event delegated to navigation");
    runtime.events.clear();
    Check(pal4::SelectDialog::HandleClick(
              Runtime::kObject, 0x8000u, runtime) == 1u &&
              runtime.invoked_index == 3u &&
              runtime.events ==
                  std::vector<std::string>{
                      "write", "hide", "invoke", "cleanup"},
          "click stores selection, closes, calls and cleans");

    EventRuntime without_callback;
    without_callback.words[Runtime::kObject + 64u] = 0u;
    Check(pal4::SelectDialog::HandleClick(
              Runtime::kObject, 0x8000u,
              without_callback) == 1u &&
              without_callback.invoked_index ==
                  0xffffffffu &&
              without_callback.events ==
                  std::vector<std::string>{
                      "write", "hide", "cleanup"},
          "click without callback still closes and cleans");
}

}  // namespace

int main() {
    try {
        CheckEmpty();
        CheckButtonsAndStrings();
        CheckNoParent();
        CheckCallback();
        CheckEvents();
        CheckNavigation();
        std::cout << "select_dialog_close_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
