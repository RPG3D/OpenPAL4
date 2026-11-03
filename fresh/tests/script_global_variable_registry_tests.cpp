#include "pal4/script_global_variable_registry.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <map>
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

std::uint32_t Hash(std::string_view bytes) {
    std::uint32_t value = 2166136261u;
    for (unsigned char byte : bytes)
        value = (value ^ byte) * 16777619u;
    return value;
}

struct Runtime final : pal4::ScriptGlobalVariableRuntimeStub {
    std::uint32_t next_address = 0x3000u;
    std::vector<std::string> events;
    std::map<std::uint32_t, std::string> strings;
    std::map<std::uint32_t, pal4::ScriptGlobalVariableDescriptor> registered;
    std::vector<std::uint32_t> cleanup_callbacks;

    std::uint32_t Allocate(std::uint32_t size, std::uint32_t kind,
                           std::uint32_t protection) override {
        Check(kind == 0x1000u && protection == 4u,
              "allocation uses VirtualAlloc commit/read-write flags");
        const auto address = next_address;
        next_address += 0x100u;
        events.push_back("alloc:" + std::to_string(size));
        return address;
    }
    void CopyString(std::uint32_t destination,
                    std::string_view source) override {
        Check(!source.empty() && source.back() == '\0',
              "source copy includes NUL byte");
        strings[destination] = std::string(source);
        events.push_back("copy:" + std::to_string(destination));
    }
    void Release(std::uint32_t address, std::uint32_t size,
                 std::uint32_t free_type) override {
        Check(free_type == 0x8000u && strings.at(address).size() == size,
              "release passes stored size and MEM_RELEASE flag");
        events.push_back("free:" + std::to_string(address));
    }
    void RegisterVariable(
        std::uint32_t address,
        const pal4::ScriptGlobalVariableDescriptor& descriptor) override {
        Check(strings.at(descriptor.description).size() ==
                  descriptor.description_size &&
                  strings.at(descriptor.signature).size() ==
                  descriptor.signature_size,
              "variable registration follows both copies");
        registered[address] = descriptor;
        events.push_back("register:" + std::to_string(address));
    }
    int RegisterAtExit(std::uint32_t callback) override {
        cleanup_callbacks.push_back(callback);
        events.push_back("atexit:" + std::to_string(callback));
        return 19;
    }
};

void CheckEntry(const Runtime& runtime, std::uint32_t descriptor_address,
                std::uint32_t variable_address, const char* signature,
                std::size_t description_size, std::uint32_t description_hash) {
    const auto& descriptor = runtime.registered.at(descriptor_address);
    Check(descriptor.variable_address == variable_address,
          "global variable address lives at descriptor offset +0x30");
    Check(descriptor.description_size == description_size &&
              Hash(runtime.strings.at(descriptor.description)) ==
                  description_hash,
          "GBK description bytes and size match IDA");
    Check(runtime.strings.at(descriptor.signature) ==
              std::string(signature, std::char_traits<char>::length(signature) + 1u),
          "variable declaration includes exact NUL-terminated text");
    for (const auto word : descriptor.reserved)
        Check(word == 0u, "unused descriptor words remain zero");
    Check(descriptor.trailing_word == 0u,
          "trailing descriptor word remains untouched");
}

void TestThreeVariablesAndInitializerOrder() {
    Runtime runtime;
    pal4::ScriptGlobalVariableRegistry registry(runtime);
    Check(registry.InitializeGlobal() == 19, "global initializer returns atexit result");
    Check(registry.InitializeMazeOne() == 19, "maze one initializer returns result");
    Check(registry.InitializeMazeTwo() == 19, "maze two initializer returns result");
    Check(runtime.cleanup_callbacks == std::vector<std::uint32_t>{
              pal4::ScriptGlobalVariableRegistry::kGlobalCleanup,
              pal4::ScriptGlobalVariableRegistry::kMazeOneCleanup,
              pal4::ScriptGlobalVariableRegistry::kMazeTwoCleanup},
          "three CRT callbacks match the registered variables");
    CheckEntry(runtime, pal4::ScriptGlobalVariableRegistry::kGlobalDescriptor,
               pal4::ScriptGlobalVariableRegistry::kGlobalVariable,
               "int global_mvar", 13u, 0xb73177d2u);
    CheckEntry(runtime, pal4::ScriptGlobalVariableRegistry::kMazeOneDescriptor,
               pal4::ScriptGlobalVariableRegistry::kMazeOneVariable,
               "int maze_var1", 14u, 0x96b9515au);
    CheckEntry(runtime, pal4::ScriptGlobalVariableRegistry::kMazeTwoDescriptor,
               pal4::ScriptGlobalVariableRegistry::kMazeTwoVariable,
               "int maze_var2", 14u, 0x70b6d6f1u);
    Check(runtime.events.size() == 18u, "each initializer performs six calls");
    for (std::size_t i = 0; i < 3u; ++i) {
        const auto base = i * 6u;
        Check(runtime.events[base].starts_with("alloc:") &&
                  runtime.events[base + 1u].starts_with("copy:") &&
                  runtime.events[base + 2u].starts_with("alloc:") &&
                  runtime.events[base + 3u].starts_with("copy:") &&
                  runtime.events[base + 4u].starts_with("register:") &&
                  runtime.events[base + 5u].starts_with("atexit:"),
              "two string copies precede variable registration and atexit");
    }
}

void TestCleanupAndDirectRegistration() {
    Runtime runtime;
    pal4::ScriptGlobalVariableRegistry registry(runtime);
    registry.RegisterGlobal();
    registry.RegisterMazeOne();
    registry.RegisterMazeTwo();
    Check(runtime.cleanup_callbacks.empty(),
          "direct entry does not install CRT cleanup");
    const auto global = registry.Global();
    const auto maze_one = registry.MazeOne();
    const auto maze_two = registry.MazeTwo();
    runtime.events.clear();
    registry.CleanupMazeTwo();
    registry.CleanupMazeOne();
    registry.CleanupGlobal();
    Check(runtime.events == std::vector<std::string>{
              "free:" + std::to_string(maze_two.signature),
              "free:" + std::to_string(maze_two.description),
              "free:" + std::to_string(maze_one.signature),
              "free:" + std::to_string(maze_one.description),
              "free:" + std::to_string(global.signature),
              "free:" + std::to_string(global.description)},
          "cleanup releases each signature before its description");
    Check(registry.Global().description == 0u &&
              registry.Global().variable_address ==
                  pal4::ScriptGlobalVariableRegistry::kGlobalVariable &&
              registry.MazeOne().signature_size == 0u &&
              registry.MazeTwo().description_size == 0u,
          "cleanup clears strings but retains variable addresses");
    runtime.events.clear();
    registry.CleanupGlobal();
    registry.CleanupMazeOne();
    registry.CleanupMazeTwo();
    Check(runtime.events.empty(), "repeated cleanup skips null pointers");
}

void TestAdditionalMazeVariables() {
    Runtime runtime;
    pal4::ScriptGlobalVariableRegistry registry(runtime);
    constexpr std::array<std::uint32_t, 8> hashes{
        0x4ab45c88u, 0x54c5b567u, 0x2ec33afeu, 0x08c0c095u,
        0xe2be462cu, 0x8ca7f87bu, 0x66a57e12u, 0x7f441cfeu};
    for (std::uint32_t number = 3u; number <= 10u; ++number)
        Check(registry.InitializeMazeNumber(number) == 19,
              "maze initializer propagates CRT atexit result");
    Check(runtime.events.size() == 48u &&
              runtime.cleanup_callbacks.size() == 8u,
          "eight maze variables each allocate, copy, register and install cleanup");
    std::vector<pal4::ScriptGlobalVariableDescriptor> snapshots;
    for (std::uint32_t number = 3u; number <= 10u; ++number) {
        const auto index = number - 3u;
        const auto signature = "int maze_var" + std::to_string(number);
        CheckEntry(runtime,
                   pal4::ScriptGlobalVariableRegistry::kAdditionalMazeDescriptors[index],
                   pal4::ScriptGlobalVariableRegistry::kAdditionalMazeVariables[index],
                   signature.c_str(), number == 10u ? 15u : 14u,
                   hashes[index]);
        Check(runtime.cleanup_callbacks[index] ==
                  pal4::ScriptGlobalVariableRegistry::kAdditionalMazeCleanups[index],
              "maze cleanup address matches IDA");
        const auto base = index * 6u;
        Check(runtime.events[base].starts_with("alloc:") &&
                  runtime.events[base + 1u].starts_with("copy:") &&
                  runtime.events[base + 2u].starts_with("alloc:") &&
                  runtime.events[base + 3u].starts_with("copy:") &&
                  runtime.events[base + 4u] ==
                      "register:" + std::to_string(
                          pal4::ScriptGlobalVariableRegistry::kAdditionalMazeDescriptors[index]) &&
                  runtime.events[base + 5u] ==
                      "atexit:" + std::to_string(
                          pal4::ScriptGlobalVariableRegistry::kAdditionalMazeCleanups[index]),
              "maze variable call order and descriptor identity");
        snapshots.push_back(registry.MazeNumber(number));
    }
    runtime.events.clear();
    for (std::uint32_t number = 10u; number >= 3u; --number)
        registry.CleanupMazeNumber(number);
    Check(runtime.events.size() == 16u,
          "eight maze destructors release two strings each");
    for (std::size_t index = 0; index < snapshots.size(); ++index) {
        const auto& original = snapshots[snapshots.size() - 1u - index];
        Check(runtime.events[2u * index] ==
                  "free:" + std::to_string(original.signature) &&
                  runtime.events[2u * index + 1u] ==
                  "free:" + std::to_string(original.description),
              "maze cleanup releases signature before description");
    }
    for (std::uint32_t number = 3u; number <= 10u; ++number)
        Check(registry.MazeNumber(number).description == 0u &&
                  registry.MazeNumber(number).signature == 0u &&
                  registry.MazeNumber(number).variable_address ==
                      pal4::ScriptGlobalVariableRegistry::kAdditionalMazeVariables[number - 3u],
              "cleanup clears strings but keeps each maze variable address");
    runtime.events.clear();
    for (std::uint32_t number = 3u; number <= 10u; ++number)
        registry.CleanupMazeNumber(number);
    Check(runtime.events.empty(), "repeated maze cleanup skips null pointers");
}

void TestMazeNumberBounds() {
    Runtime runtime;
    pal4::ScriptGlobalVariableRegistry registry(runtime);
    for (const auto number : {0u, 2u, 11u, 0xffffffffu}) {
        bool threw = false;
        try {
            (void)registry.InitializeMazeNumber(number);
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw && runtime.events.empty(),
              "invalid helper index cannot register another descriptor");
    }
}

void TestNamedVariables() {
    Runtime runtime;
    pal4::ScriptGlobalVariableRegistry registry(runtime);
    constexpr std::array<const char*, 37> signatures{
        "int var_shu", "int var_xia", "int var_wu", "int var_feng",
        "int var_zi", "int var_mo", "int var_ming", "int var_yan",
        "int var_cha", "int var_star", "int npc_a", "int npc_b",
        "int npc_c", "int npc_d", "int npc_e", "int npc_f",
        "int npc_g", "int var_zhu", "int var_shop031", "int var_shop010",
        "int var_shop032", "int var_shop021", "int var_shop012",
        "int var_shop034", "int var_shop037", "int var_shop004",
        "int var_shop024", "int var_shop025", "int var_shop005",
        "int var_shop035", "int var_shop026", "int var_shop006",
        "int var_shop027", "int var_shop007", "int var_shop029",
        "int var_shop009", "int var_shop030"};
    constexpr std::array<std::size_t, 37> description_sizes{
        9u, 7u, 7u, 9u, 9u, 5u, 9u, 5u, 9u, 5u,
        20u, 18u, 18u, 18u, 16u, 16u, 20u, 17u,
        15u, 15u, 15u, 13u, 13u, 15u, 13u, 16u, 16u,
        13u, 13u, 15u, 13u, 15u, 13u, 11u, 11u, 13u, 13u};
    constexpr std::array<std::uint32_t, 37> description_hashes{
        0xd10654a1u, 0x1d8fdcbfu, 0x81d5af00u, 0x2892d461u,
        0x75217316u, 0xcd266190u, 0x8cc2a566u, 0xc95bfc7cu,
        0x725b3b87u, 0x53001186u, 0xf8c5ddcdu, 0x632d54bbu,
        0x39a69cafu, 0x413106ebu, 0xf20f7e26u, 0x931dda46u,
        0x96eefe2bu, 0xed872f6eu, 0x52fd3ed9u, 0x79f5ae8du,
        0xae62d9f6u, 0x6fe935e3u, 0x1e1838cbu, 0xff045436u,
        0xc4f88bafu, 0x1c6d6267u, 0xae4c841au,
        0xbf5ab2e1u, 0xe2f80e4eu, 0x6d4cd43au, 0x4b774667u,
        0x00001333u, 0x2c094372u, 0xb621e22eu, 0x5ecac798u,
        0x7f9ff478u, 0xbb661e99u};
    for (std::size_t index = 0; index < signatures.size(); ++index) {
        const auto variable = static_cast<pal4::ScriptNamedVariable>(index);
        Check(registry.InitializeNamed(variable) == 19,
              "named initializer propagates CRT atexit result");
        CheckEntry(runtime,
                   pal4::ScriptGlobalVariableRegistry::kNamedDescriptors[index],
                   pal4::ScriptGlobalVariableRegistry::kNamedVariables[index],
                   signatures[index], description_sizes[index],
                   description_hashes[index]);
        Check(runtime.cleanup_callbacks[index] ==
                  pal4::ScriptGlobalVariableRegistry::kNamedCleanups[index],
              "named cleanup callback matches IDA");
        const auto base = index * 6u;
        Check(runtime.events[base] ==
                  "alloc:" + std::to_string(description_sizes[index]) &&
                  runtime.events[base + 1u].starts_with("copy:") &&
                  runtime.events[base + 2u] ==
                      "alloc:" + std::to_string(
                          std::char_traits<char>::length(signatures[index]) + 1u) &&
                  runtime.events[base + 3u].starts_with("copy:") &&
                  runtime.events[base + 4u] ==
                      "register:" + std::to_string(
                          pal4::ScriptGlobalVariableRegistry::kNamedDescriptors[index]) &&
                  runtime.events[base + 5u] ==
                      "atexit:" + std::to_string(
                          pal4::ScriptGlobalVariableRegistry::kNamedCleanups[index]),
              "named variable allocation, copy, registration and atexit order");
    }
    Check(runtime.events.size() == signatures.size() * 6u,
          "named initializers each perform six runtime calls");

    std::array<pal4::ScriptGlobalVariableDescriptor, 37> snapshots{};
    for (std::size_t index = 0; index < snapshots.size(); ++index)
        snapshots[index] = registry.Named(
            static_cast<pal4::ScriptNamedVariable>(index));
    runtime.events.clear();
    for (std::size_t index = snapshots.size(); index > 0u; --index)
        registry.CleanupNamed(
            static_cast<pal4::ScriptNamedVariable>(index - 1u));
    Check(runtime.events.size() == snapshots.size() * 2u,
          "each named cleanup releases two allocated strings");
    for (std::size_t index = 0; index < snapshots.size(); ++index) {
        const auto& original = snapshots[snapshots.size() - 1u - index];
        Check(runtime.events[2u * index] ==
                  "free:" + std::to_string(original.signature) &&
                  runtime.events[2u * index + 1u] ==
                  "free:" + std::to_string(original.description),
              "reverse CRT cleanup releases signature before description");
    }
    for (std::size_t index = 0; index < snapshots.size(); ++index) {
        const auto& descriptor = registry.Named(
            static_cast<pal4::ScriptNamedVariable>(index));
        Check(descriptor.description == 0u &&
                  descriptor.description_size == 0u &&
                  descriptor.signature == 0u &&
                  descriptor.signature_size == 0u &&
                  descriptor.variable_address ==
                      pal4::ScriptGlobalVariableRegistry::kNamedVariables[index],
              "named cleanup clears string fields and retains variable pointer");
    }
    runtime.events.clear();
    for (std::size_t index = 0; index < snapshots.size(); ++index)
        registry.CleanupNamed(static_cast<pal4::ScriptNamedVariable>(index));
    Check(runtime.events.empty(), "repeated named cleanup skips null pointers");
}

void TestNamedVariableBoundsAndDirectRegistration() {
    Runtime runtime;
    pal4::ScriptGlobalVariableRegistry registry(runtime);
    constexpr auto shu = pal4::ScriptNamedVariable::Shu;
    registry.RegisterNamed(shu);
    Check(runtime.cleanup_callbacks.empty() && runtime.events.size() == 5u,
          "direct named registration does not install atexit callback");
    registry.CleanupNamed(shu);
    runtime.events.clear();
    for (const auto invalid : {37u, 0xffu}) {
        const auto variable = static_cast<pal4::ScriptNamedVariable>(invalid);
        bool threw = false;
        try {
            (void)registry.InitializeNamed(variable);
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw && runtime.events.empty(),
              "invalid named initializer has no runtime side effects");
        threw = false;
        try {
            (void)registry.Named(variable);
        } catch (const std::out_of_range&) {
            threw = true;
        }
        Check(threw, "invalid named variable lookup is rejected");
    }
}

}  // namespace

int main() {
    TestThreeVariablesAndInitializerOrder();
    TestCleanupAndDirectRegistration();
    TestAdditionalMazeVariables();
    TestMazeNumberBounds();
    TestNamedVariables();
    TestNamedVariableBoundsAndDirectRegistration();
}
