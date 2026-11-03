#include "pal4/shop_ui_smith.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IShopUiSmithRuntime {
    static constexpr std::uint32_t kFrame = 0x5000u;
    std::map<std::uint32_t, std::uint32_t> words{
        {kFrame, 0x6000u}, {0x6004u, 0x6100u},
        {0x8b94b8u, 0xa000u}, {0xc018u, 0xd000u}};
    std::vector<std::string> calls;
    std::uint32_t found = 0xe000u;
    std::uint32_t set_count = 0u;
    std::uint32_t add_count = 0u;
    std::uint32_t destroyed_attributes = 0u;
    std::uint32_t destroyed_documents = 0u;
    bool throw_find = false;
    bool throw_add = false;

    void GetData(std::uint32_t, std::uint32_t,
                 std::uint32_t) override {
        Check(false, "smith opening does not select shop data");
    }
    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void CallVisibilitySlot(const std::uint32_t function,
                            const std::uint32_t frame,
                            const std::uint32_t visible,
                            const std::uint32_t argument) override {
        Check(function == 0x6100u && frame == kFrame &&
                  visible == 0u && argument == 0u,
              "smith opening hides shop first");
        calls.emplace_back("hide");
    }
    std::uint32_t TradeSystem() override {
        calls.emplace_back("trade system");
        return 0x6200u;
    }
    std::uint32_t SetTradeState(
        const std::uint32_t system) override {
        Check(system == 0x6200u, "smith opening clears trade state");
        calls.emplace_back("trade state");
        return 0u;
    }
    std::uint32_t CreateDocument() override {
        calls.emplace_back("document");
        return 0xc000u;
    }
    std::uint32_t CreateTemporaryText() override {
        calls.emplace_back("create text");
        return 0xb000u;
    }
    void SetTextFlag(const std::uint32_t text,
                     const std::uint8_t flag) override {
        Check(text == 0xb000u && flag <= 1u,
              "temporary attribute search text flag");
        calls.emplace_back(flag == 0u ? "text open" : "text close");
    }
    void AssignText(const std::uint32_t text,
                    const std::uint32_t source) override {
        Check(text == 0xb000u && source == 0xa000u,
              "Smith_OpenWithSystem attribute key");
        calls.emplace_back("assign text");
    }
    std::uint32_t FindAttribute(const std::uint32_t tree,
                                const std::uint32_t text) override {
        Check(tree == 0xc014u && text == 0xb000u,
              "document attribute tree at +0x14");
        calls.emplace_back("find");
        if (throw_find) throw std::runtime_error("find");
        return found;
    }
    void SetAttributeInt(const std::uint32_t value,
                         const std::uint32_t number) override {
        Check(value == found + 0x1cu && number == 0u,
              "existing smith attribute value at +0x1c is reset");
        calls.emplace_back("set int");
        ++set_count;
    }
    std::uint32_t CreateAttribute(const std::uint32_t name,
                                  const std::uint32_t number) override {
        Check(name == 0xa000u && number == 0u,
              "missing smith attribute name and zero value");
        calls.emplace_back("create attribute");
        return 0xb100u;
    }
    void AddAttribute(const std::uint32_t document,
                      const std::uint32_t attribute) override {
        Check(document == 0xc000u && attribute == 0xb100u,
              "add new smith attribute to XML document");
        calls.emplace_back("add attribute");
        ++add_count;
        if (throw_add) throw std::runtime_error("add");
    }
    void DestroyAttribute(const std::uint32_t attribute) override {
        Check(attribute == 0xb100u,
              "temporary smith attribute destruction");
        calls.emplace_back("destroy attribute");
        ++destroyed_attributes;
    }
    std::uint32_t GameInstance() override {
        calls.emplace_back("game");
        return 0xf000u;
    }
    void InitializeStateAndUi(const std::uint32_t game,
                              const std::uint32_t state,
                              const std::uint32_t document) override {
        Check(game == 0xf000u && state == 5u &&
                  document == 0xc000u,
              "smith opening enters game state five with XML document");
        calls.emplace_back("state");
    }
    void DestroyDocument(const std::uint32_t document) override {
        Check(document == 0xc000u, "smith document cleanup receiver");
        calls.emplace_back("destroy document");
        ++destroyed_documents;
    }
};

void CheckClosePrelude(const Runtime& runtime) {
    Check(runtime.calls.size() >= 6u &&
              runtime.calls[0] == "read" &&
              runtime.calls[1] == "read" &&
              runtime.calls[2] == "hide" &&
              runtime.calls[3] == "trade system" &&
              runtime.calls[4] == "trade state" &&
              runtime.calls[5] == "document",
          "smith action closes shop before creating XML document");
}

void TestExistingAndMissingAttribute() {
    Runtime existing;
    Check(pal4::ShopUiSmith::Open(
              Runtime::kFrame, 0xdeadbeefu, existing) == 1u &&
              existing.set_count == 1u && existing.add_count == 0u &&
              existing.destroyed_attributes == 0u &&
              existing.destroyed_documents == 1u &&
              existing.calls[existing.calls.size() - 2u] == "state" &&
              existing.calls.back() == "destroy document",
          "existing attribute resets integer then enters smith state");
    CheckClosePrelude(existing);

    for (const auto missing : {0xd000u, 0xffffffe4u}) {
        Runtime runtime;
        runtime.found = missing;
        Check(pal4::ShopUiSmith::Open(
                  Runtime::kFrame, 0u, runtime) == 1u &&
                  runtime.set_count == 0u && runtime.add_count == 1u &&
                  runtime.destroyed_attributes == 1u &&
                  runtime.destroyed_documents == 1u &&
                  runtime.calls[runtime.calls.size() - 4u] ==
                      "destroy attribute",
              "end iterator and minus-28 sentinel create zero attribute");
        CheckClosePrelude(runtime);
    }
}

void TestExceptionalCleanup() {
    Runtime searching;
    searching.throw_find = true;
    bool caught = false;
    try {
        static_cast<void>(pal4::ShopUiSmith::Open(
            Runtime::kFrame, 0u, searching));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && searching.calls[searching.calls.size() - 2u] ==
                        "text close" &&
              searching.calls.back() == "destroy document" &&
              searching.destroyed_documents == 1u,
          "attribute search exception closes temporary key and document");

    Runtime adding;
    adding.found = 0xd000u;
    adding.throw_add = true;
    caught = false;
    try {
        static_cast<void>(pal4::ShopUiSmith::Open(
            Runtime::kFrame, 0u, adding));
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && adding.destroyed_attributes == 1u &&
              adding.destroyed_documents == 1u &&
              adding.calls[adding.calls.size() - 2u] ==
                  "destroy attribute" &&
              adding.calls.back() == "destroy document",
          "failed insert destroys temporary attribute and XML document");
}

}  // namespace

int main() {
    TestExistingAndMissingAttribute();
    TestExceptionalCleanup();
    return EXIT_SUCCESS;
}
