#include "pal4/npc_manager_creator.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::NpcManagerCreateRuntimeStub {
    std::vector<std::string> events;
    std::unordered_map<std::uint32_t, std::uint8_t> headers;
    bool duplicate{};
    bool has_attributes{true};
    bool has_action{true};
    bool throw_constructor{};
    std::uint32_t action_argument{};

    std::uint32_t CStringLength(std::uint32_t address) override {
        Check(address == 0x12344u, "name address");
        events.emplace_back("length");
        return 4u;
    }
    void SetErrorFlag(pal4::ModeStringBufferRecord& record,
                      std::uint8_t flag) override {
        events.emplace_back(flag == 0u ? "flag0" : "flag1");
        if (flag != 0u && record.buffer.data != 0u)
            FreeTextHeader(record.buffer.data - 1u);
        record.buffer = {};
    }
    bool AllocateText(pal4::ModeStringBufferRecord& record,
                      std::uint32_t length, std::uint8_t mode) override {
        Check(length == 4u && mode == 1u, "name length and mode");
        events.emplace_back("allocate-text");
        record.buffer.data = 0x8001u;
        headers[0x8000u] = 0u;
        return true;
    }
    void CopyBytes(std::uint32_t dst, std::uint32_t src,
                   std::uint32_t size) override {
        Check(dst == 0x8001u && src == 0x12344u && size == 4u,
              "name bytes copied");
    }
    void SetTextSize(pal4::ModeStringBufferRecord& record,
                     std::uint32_t length) override {
        record.buffer.length = length;
    }
    std::uint32_t SearchNode(std::uint32_t tree,
        const pal4::ModeStringBufferRecord& record) override {
        Check(tree == 0x5028u && record.buffer.data == 0x8001u,
              "NPC tree search");
        events.emplace_back("search");
        return duplicate ? 0x7100u : 0x7000u;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        if (address == 0x502cu) return 0x7000u;
        if (address == 0xb03cu) return 0x55u;
        if (address == 0xb050u) return 0xc000u;
        Check(false, "unexpected DWORD read");
        return 0u;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return headers.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        headers[address] = value;
    }
    void FreeTextHeader(std::uint32_t address) override {
        events.emplace_back(address == 0x9000u ? "free-action" : "free-name");
    }
    void LoadScripts() override { events.emplace_back("load-scripts"); }
    void CleanupScripts() override { events.emplace_back("cleanup-scripts"); }
    std::uint32_t AllocateObject(std::uint32_t bytes) override {
        Check(bytes == 0x6ecu, "NPC object size");
        events.emplace_back("allocate-object");
        return 0xa000u;
    }
    void FreeObjectStorage(std::uint32_t address) override {
        Check(address == 0xa000u, "failed constructor storage");
        events.emplace_back("free-object");
    }
    std::uint32_t ConstructObject(std::uint32_t storage) override {
        Check(storage == 0xa000u, "constructor input");
        events.emplace_back("construct");
        if (throw_constructor) throw std::runtime_error("constructor");
        return storage;
    }
    void CallSlot30(std::uint32_t object, std::uint32_t name,
                    std::uint32_t config, std::uint32_t enabled) override {
        Check(object == 0xa000u && name == 0x12344u &&
              config == 0x1111u && enabled == 1u, "slot 0x30 arguments");
        events.emplace_back("slot30");
    }
    void CallSlot4(std::uint32_t, std::uint32_t value,
                   std::uint32_t zero) override {
        Check(value == 0x2222u && zero == 0u, "slot 4 arguments");
        events.emplace_back("slot4");
    }
    void CallSlot8(std::uint32_t, std::uint32_t value,
                   std::uint32_t zero) override {
        Check(value == 0x3333u && zero == 0u, "slot 8 arguments");
        events.emplace_back("slot8");
    }
    void SetStateFlag(std::uint32_t object, std::uint32_t flag) override {
        Check(object == 0xa000u && flag == 0x55u,
              "NPC visibility reads record +0x3c");
        events.emplace_back("state");
    }
    std::uint32_t FindChild(std::uint32_t document,
                            const char* name) override {
        Check(document == 0xc000u &&
              std::string(name) == "NPCINFO_BufferCache_Attr",
              "NPC attributes lookup");
        events.emplace_back("find-child");
        return has_attributes ? 0xd000u : 0u;
    }
    void ReadStringAttribute(std::uint32_t node, const char* name,
        const char* fallback, pal4::ModeStringBufferRecord& result) override {
        Check(node == 0xd000u &&
              std::string(name) == "NPCINFO_BufferCache_Attr_defaultAct" &&
              std::string(fallback) == "C01", "default action lookup");
        events.emplace_back("read-action");
        if (has_action) {
            result.buffer.data = 0x9001u;
            headers[0x9000u] = 0u;
        }
    }
    std::uint32_t EmptyTextAddress() override { return 0xe000u; }
    void SetDefaultAction(std::uint32_t object,
                          std::uint32_t text) override {
        Check(object == 0xa000u, "default action object");
        events.emplace_back("set-action");
        action_argument = text;
    }
    void AssignName(pal4::ModeStringBufferRecord& target,
                    const pal4::ModeStringBufferRecord& name,
                    std::uint32_t start, std::uint32_t count) override {
        Check(name.buffer.data == 0x8001u && start == 0u &&
              count == 0xffffffffu, "full NPC name copy");
        events.emplace_back("assign-key");
        target.buffer = name.buffer;
        headers[0x8000u] = 1u;
    }
    void InsertNode(std::uint32_t tree,
                    const pal4::ModeStringBufferRecord& key,
                    std::uint32_t object) override {
        Check(tree == 0x5028u && key.buffer.data == 0x8001u &&
              object == 0xa000u, "NPC insertion");
        events.emplace_back("insert");
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        Check(address == 0xa204u && value == 0x5038u,
              "NPC grid pointer");
        events.emplace_back("grid-link");
    }
    void SetupBehavior(std::uint32_t object,
                       std::uint32_t record,
                       std::uint32_t enabled) override {
        Check(object == 0xa000u && record == 0xb000u && enabled == 1u,
              "NPC behavior setup");
        events.emplace_back("behavior");
    }
};

constexpr pal4::NpcCreationArguments kArguments{
    0x12344u, 0x1111u, 0x2222u, 0x3333u, 0xb000u, 1u};

void SuccessfulCreate() {
    Runtime runtime;
    Check(pal4::NpcManagerCreator::Create(0x5000u, kArguments,
                                          runtime) == 0xa000u,
          "NPC creation result");
    Check(runtime.action_argument == 0x9001u,
          "saved default action used");
    Check(runtime.events == std::vector<std::string>{
        "flag0", "length", "allocate-text", "search", "load-scripts",
        "allocate-object", "construct", "slot30", "slot4", "slot8",
        "cleanup-scripts", "state", "find-child", "read-action",
        "set-action", "flag1", "free-action", "flag0", "assign-key",
        "insert", "grid-link", "behavior", "free-name"},
        "NPC creation and temporary cleanup order");
}

void DuplicateName() {
    Runtime runtime;
    runtime.duplicate = true;
    Check(pal4::NpcManagerCreator::Create(0x5000u, kArguments,
                                          runtime) == 0u &&
          runtime.events == std::vector<std::string>{
              "flag0", "length", "allocate-text", "search",
              "flag1", "free-name"},
          "duplicate exits before script loading");
}

void MissingAction() {
    Runtime runtime;
    runtime.has_action = false;
    Check(pal4::NpcManagerCreator::Create(0x5000u, kArguments,
                                          runtime) == 0xa000u &&
          runtime.action_argument == 0xe000u,
          "empty action uses global empty text");
    Runtime no_attributes;
    no_attributes.has_attributes = false;
    Check(pal4::NpcManagerCreator::Create(0x5000u, kArguments,
                                          no_attributes) == 0xa000u &&
          no_attributes.action_argument == 0u,
          "missing attributes skip action assignment");
}

void ConstructorFailure() {
    Runtime runtime;
    runtime.throw_constructor = true;
    try {
        (void)pal4::NpcManagerCreator::Create(0x5000u, kArguments,
                                               runtime);
        Check(false, "constructor must throw");
    } catch (const std::runtime_error&) {}
    Check(runtime.events.back() == "free-name" &&
          runtime.events[runtime.events.size() - 2u] == "free-object",
          "failed constructor frees storage and temporary name");
}

}  // namespace

int main() {
    SuccessfulCreate();
    DuplicateName();
    MissingAction();
    ConstructorFailure();
    std::cout << "NPC manager creator cases passed\n";
}
