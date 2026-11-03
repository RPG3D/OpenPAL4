#include "pal4/script_global_variable_registry.h"

#include <stdexcept>
#include <string>

namespace pal4 {
namespace {

constexpr char kGlobalDescription[] =
    "\xd6\xf7\xcf\xdf\xbe\xe7\xc7\xe9\xb1\xe4\xc1\xbf";
constexpr char kMazeOneDescription[] =
    "\xc3\xd4\xb9\xac\xd7\xa8\xd3\xc3\xb1\xe4\xc1\xbf\x31";
constexpr char kMazeTwoDescription[] =
    "\xc3\xd4\xb9\xac\xd7\xa8\xd3\xc3\xb1\xe4\xc1\xbf\x32";
constexpr char kMazeDescriptionPrefix[] =
    "\xc3\xd4\xb9\xac\xd7\xa8\xd3\xc3\xb1\xe4\xc1\xbf";
constexpr char kShuDescription[] = "\xca\xf1\xd6\xd0\xbe\xde\xb8\xbb";
constexpr char kXiaDescription[] = "\xcf\xc4\xea\xd4\xb3\xe6";
constexpr char kWuDescription[] = "\xce\xe5\xb6\xbe\xca\xde";
constexpr char kFengDescription[] = "\xb7\xef\xbb\xcb\xb3\xa4\xc0\xeb";
constexpr char kZiDescription[] = "\xd7\xd3\xc3\xac\xd6\xae\xca\xc4";
constexpr char kMoDescription[] = "\xc4\xa7\xbd\xa3";
constexpr char kMingDescription[] = "\xc3\xf7\xd6\xe9\xd3\xd0\xc0\xe1";
constexpr char kYanDescription[] = "\xd9\xc8\xca\xa6";
constexpr char kChaDescription[] = "\xc9\xb2\xc4\xc7\xb7\xbc\xbb\xaa";
constexpr char kStarDescription[] = "\xc9\xcf\xc8\xed";
constexpr char kNpcADescription[] =
    "\xcc\xab\xc6\xbd\xb4\xe5\x20\xd5\xd4\xd4\xaa\xbc\xaa\xd0\xa1\xbe\xe7\xc7\xe9";
constexpr char kNpcBDescription[] =
    "\xca\xd9\xd1\xf4\x20\xb8\xf0\xb4\xf3\xc2\xb7\xd0\xa1\xbe\xe7\xc7\xe9";
constexpr char kNpcCDescription[] =
    "\xb3\xc2\xd6\xdd\x20\xba\xfa\xb5\xc3\xcb\xb7\xd0\xa1\xbe\xe7\xc7\xe9";
constexpr char kNpcDDescription[] =
    "\xb2\xa5\xcf\xc9\xd5\xf2\x20\xb0\xa2\xbf\xcb\xd0\xa1\xbe\xe7\xc7\xe9";
constexpr char kNpcEDescription[] =
    "\xbc\xb4\xc4\xab\x20\xc7\xef\xc0\xbc\xd0\xa1\xbe\xe7\xc7\xe9";
constexpr char kNpcFDescription[] =
    "\xb9\xed\xbd\xe7\x20\xd0\xa1\xc1\xaf\xd0\xa1\xbe\xe7\xc7\xe9";
constexpr char kNpcGDescription[] =
    "\xc7\xed\xbb\xaa\xc5\xc9\x20\xbe\xed\xd4\xc6\xcc\xa8\xd0\xa1\xbe\xe7\xc7\xe9";
constexpr char kZhuDescription[] =
    "\xd3\xeb\xb0\xd9\xf4\xe1\xd6\xde\x62\x6f\x73\x73\xd5\xbd\xd3\xc3";
constexpr char kShop031Description[] =
    "\xcc\xab\xc6\xbd\xb4\xe5\x09\x09\xca\xb3\xc6\xb7\xb5\xea";
constexpr char kShop010Description[] =
    "\xcc\xab\xc6\xbd\xb4\xe5\x09\x09\xd4\xd3\xbb\xf5\xc6\xcc";
constexpr char kShop032Description[] =
    "\xca\xd9\xd1\xf4\xb3\xc7\x09\x09\xca\xb3\xc6\xb7\xb5\xea";
constexpr char kShop021Description[] =
    "\xca\xd9\xd1\xf4\xb3\xc7\x09\x09\xcc\xfa\xc6\xcc";
constexpr char kShop012Description[] =
    "\xb3\xc2\xd6\xdd\x09\x09\xd4\xd3\xbb\xf5\xb5\xea";
constexpr char kShop034Description[] =
    "\xb2\xa5\xcf\xc9\xd5\xf2\x09\x09\xca\xb3\xc6\xb7\xb5\xea";
constexpr char kShop037Description[] =
    "\xb2\xa5\xcf\xc9\xd5\xf2\x09\x09\xbf\xcd\xd5\xbb";
constexpr char kShop004Description[] =
    "\xc0\xa5\xc2\xd8\xc7\xed\xbb\xaa\xc5\xc9\x09\xd2\xa9\xc6\xcc";
constexpr char kShop024Description[] =
    "\xc0\xa5\xc2\xd8\xc7\xed\xbb\xaa\xc5\xc9\x09\xcc\xfa\xc6\xcc";
constexpr char kShop025Description[] =
    "\xd4\xc2\xd1\xc0\xb4\xe5\x09\x09\xcc\xfa\xc6\xcc";
constexpr char kShop005Description[] =
    "\xbc\xb4\xc4\xab\xcf\xd8\x09\x09\xd2\xa9\xc6\xcc";
constexpr char kShop035Description[] =
    "\xbc\xb4\xc4\xab\xcf\xd8\x09\x09\xca\xb3\xc6\xb7\xb5\xea";
constexpr char kShop026Description[] =
    "\xbc\xb4\xc4\xab\xcf\xd8\x09\x09\xcc\xfa\xc6\xcc";
constexpr char kShop006Description[] =
    "\xbe\xd3\xb3\xb2\xb9\xfa\x09\x09\xd4\xd3\xbb\xf5\xc6\xcc";
constexpr char kShop027Description[] =
    "\xbe\xd3\xb3\xb2\xb9\xfa\x09\x09\xcc\xfa\xc6\xcc";
constexpr char kShop007Description[] =
    "\xb9\xed\xbd\xe7\x09\x09\xd2\xa9\xc6\xcc";
constexpr char kShop029Description[] =
    "\xdb\xba\xb6\xbc\x09\x09\xcc\xfa\xc6\xcc";
constexpr char kShop009Description[] =
    "\xbb\xc3\xda\xa4\xbd\xe7\x09\x09\xd2\xa9\xc6\xcc";
constexpr char kShop030Description[] =
    "\xbb\xc3\xda\xa4\xbd\xe7\x09\x09\xcc\xfa\xc6\xcc";

std::size_t AdditionalMazeIndex(std::uint32_t number) {
    if (number < 3u || number > 10u)
        throw std::out_of_range("maze variable number must be 3..10");
    return number - 3u;
}

template <std::size_t N>
constexpr std::string_view BytesWithNul(const char (&text)[N]) {
    return {text, N};
}

struct NamedVariableText {
    std::string_view description;
    std::string_view signature;
};

constexpr std::array<NamedVariableText, 37> kNamedText{{
    {BytesWithNul(kShuDescription), BytesWithNul("int var_shu")},
    {BytesWithNul(kXiaDescription), BytesWithNul("int var_xia")},
    {BytesWithNul(kWuDescription), BytesWithNul("int var_wu")},
    {BytesWithNul(kFengDescription), BytesWithNul("int var_feng")},
    {BytesWithNul(kZiDescription), BytesWithNul("int var_zi")},
    {BytesWithNul(kMoDescription), BytesWithNul("int var_mo")},
    {BytesWithNul(kMingDescription), BytesWithNul("int var_ming")},
    {BytesWithNul(kYanDescription), BytesWithNul("int var_yan")},
    {BytesWithNul(kChaDescription), BytesWithNul("int var_cha")},
    {BytesWithNul(kStarDescription), BytesWithNul("int var_star")},
    {BytesWithNul(kNpcADescription), BytesWithNul("int npc_a")},
    {BytesWithNul(kNpcBDescription), BytesWithNul("int npc_b")},
    {BytesWithNul(kNpcCDescription), BytesWithNul("int npc_c")},
    {BytesWithNul(kNpcDDescription), BytesWithNul("int npc_d")},
    {BytesWithNul(kNpcEDescription), BytesWithNul("int npc_e")},
    {BytesWithNul(kNpcFDescription), BytesWithNul("int npc_f")},
    {BytesWithNul(kNpcGDescription), BytesWithNul("int npc_g")},
    {BytesWithNul(kZhuDescription), BytesWithNul("int var_zhu")},
    {BytesWithNul(kShop031Description), BytesWithNul("int var_shop031")},
    {BytesWithNul(kShop010Description), BytesWithNul("int var_shop010")},
    {BytesWithNul(kShop032Description), BytesWithNul("int var_shop032")},
    {BytesWithNul(kShop021Description), BytesWithNul("int var_shop021")},
    {BytesWithNul(kShop012Description), BytesWithNul("int var_shop012")},
    {BytesWithNul(kShop034Description), BytesWithNul("int var_shop034")},
    {BytesWithNul(kShop037Description), BytesWithNul("int var_shop037")},
    {BytesWithNul(kShop004Description), BytesWithNul("int var_shop004")},
    {BytesWithNul(kShop024Description), BytesWithNul("int var_shop024")},
    {BytesWithNul(kShop025Description), BytesWithNul("int var_shop025")},
    {BytesWithNul(kShop005Description), BytesWithNul("int var_shop005")},
    {BytesWithNul(kShop035Description), BytesWithNul("int var_shop035")},
    {BytesWithNul(kShop026Description), BytesWithNul("int var_shop026")},
    {BytesWithNul(kShop006Description), BytesWithNul("int var_shop006")},
    {BytesWithNul(kShop027Description), BytesWithNul("int var_shop027")},
    {BytesWithNul(kShop007Description), BytesWithNul("int var_shop007")},
    {BytesWithNul(kShop029Description), BytesWithNul("int var_shop029")},
    {BytesWithNul(kShop009Description), BytesWithNul("int var_shop009")},
    {BytesWithNul(kShop030Description), BytesWithNul("int var_shop030")},
}};

std::size_t NamedIndex(ScriptNamedVariable variable) {
    const auto index = static_cast<std::size_t>(variable);
    if (index >= kNamedText.size())
        throw std::out_of_range("unknown named script variable");
    return index;
}

}  // namespace

std::string_view ScriptGlobalVariableRegistry::NamedSignature(ScriptNamedVariable variable) {
    const auto text=kNamedText[NamedIndex(variable)].signature;return text.substr(0,text.size()-1u);
}

std::uint32_t ScriptGlobalVariableRuntimeStub::Allocate(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }
void ScriptGlobalVariableRuntimeStub::CopyString(
    std::uint32_t, std::string_view) {}
void ScriptGlobalVariableRuntimeStub::Release(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void ScriptGlobalVariableRuntimeStub::RegisterVariable(
    std::uint32_t, const ScriptGlobalVariableDescriptor&) {}
int ScriptGlobalVariableRuntimeStub::RegisterAtExit(std::uint32_t) { return 0; }

void ScriptGlobalVariableRegistry::Register(
    ScriptGlobalVariableDescriptor& descriptor,
    std::uint32_t descriptor_address, std::string_view description,
    std::string_view signature, std::uint32_t variable_address) {
    descriptor.description = 0u;
    descriptor.description_size = 0u;
    descriptor.signature = 0u;
    descriptor.signature_size = 0u;
    descriptor.description_size = static_cast<std::uint32_t>(description.size());
    descriptor.description = runtime_.Allocate(descriptor.description_size,
                                                0x1000u, 4u);
    runtime_.CopyString(descriptor.description, description);
    descriptor.signature_size = static_cast<std::uint32_t>(signature.size());
    descriptor.signature = runtime_.Allocate(descriptor.signature_size,
                                              0x1000u, 4u);
    runtime_.CopyString(descriptor.signature, signature);
    descriptor.variable_address = variable_address;
    runtime_.RegisterVariable(descriptor_address, descriptor);
}

void ScriptGlobalVariableRegistry::Cleanup(
    ScriptGlobalVariableDescriptor& descriptor) {
    if (descriptor.signature != 0u)
        runtime_.Release(descriptor.signature, descriptor.signature_size,
                         0x8000u);
    descriptor.signature = 0u;
    descriptor.signature_size = 0u;
    if (descriptor.description != 0u)
        runtime_.Release(descriptor.description, descriptor.description_size,
                         0x8000u);
    descriptor.description = 0u;
    descriptor.description_size = 0u;
}

int ScriptGlobalVariableRegistry::InitializeGlobal() {
    RegisterGlobal();
    return runtime_.RegisterAtExit(kGlobalCleanup);
}
void ScriptGlobalVariableRegistry::RegisterGlobal() {
    Register(global_, kGlobalDescriptor, BytesWithNul(kGlobalDescription),
             BytesWithNul("int global_mvar"), kGlobalVariable);
}
void ScriptGlobalVariableRegistry::CleanupGlobal() { Cleanup(global_); }

int ScriptGlobalVariableRegistry::InitializeMazeOne() {
    RegisterMazeOne();
    return runtime_.RegisterAtExit(kMazeOneCleanup);
}
void ScriptGlobalVariableRegistry::RegisterMazeOne() {
    Register(maze_one_, kMazeOneDescriptor, BytesWithNul(kMazeOneDescription),
             BytesWithNul("int maze_var1"), kMazeOneVariable);
}
void ScriptGlobalVariableRegistry::CleanupMazeOne() { Cleanup(maze_one_); }

int ScriptGlobalVariableRegistry::InitializeMazeTwo() {
    RegisterMazeTwo();
    return runtime_.RegisterAtExit(kMazeTwoCleanup);
}
void ScriptGlobalVariableRegistry::RegisterMazeTwo() {
    Register(maze_two_, kMazeTwoDescriptor, BytesWithNul(kMazeTwoDescription),
             BytesWithNul("int maze_var2"), kMazeTwoVariable);
}
void ScriptGlobalVariableRegistry::CleanupMazeTwo() { Cleanup(maze_two_); }

int ScriptGlobalVariableRegistry::InitializeMazeNumber(std::uint32_t number) {
    const auto index = AdditionalMazeIndex(number);
    RegisterMazeNumber(number);
    return runtime_.RegisterAtExit(kAdditionalMazeCleanups[index]);
}

void ScriptGlobalVariableRegistry::RegisterMazeNumber(std::uint32_t number) {
    const auto index = AdditionalMazeIndex(number);
    std::string description(kMazeDescriptionPrefix,
                            sizeof(kMazeDescriptionPrefix) - 1u);
    description += std::to_string(number);
    description.push_back('\0');
    std::string signature = "int maze_var" + std::to_string(number);
    signature.push_back('\0');
    Register(additional_maze_[index], kAdditionalMazeDescriptors[index],
             description, signature, kAdditionalMazeVariables[index]);
}

void ScriptGlobalVariableRegistry::CleanupMazeNumber(std::uint32_t number) {
    Cleanup(additional_maze_[AdditionalMazeIndex(number)]);
}

const ScriptGlobalVariableDescriptor& ScriptGlobalVariableRegistry::MazeNumber(
    std::uint32_t number) const {
    return additional_maze_[AdditionalMazeIndex(number)];
}

int ScriptGlobalVariableRegistry::InitializeNamed(
    ScriptNamedVariable variable) {
    const auto index = NamedIndex(variable);
    RegisterNamed(variable);
    return runtime_.RegisterAtExit(kNamedCleanups[index]);
}

void ScriptGlobalVariableRegistry::RegisterNamed(
    ScriptNamedVariable variable) {
    const auto index = NamedIndex(variable);
    Register(named_[index], kNamedDescriptors[index],
             kNamedText[index].description, kNamedText[index].signature,
             kNamedVariables[index]);
}

void ScriptGlobalVariableRegistry::CleanupNamed(
    ScriptNamedVariable variable) {
    Cleanup(named_[NamedIndex(variable)]);
}

const ScriptGlobalVariableDescriptor& ScriptGlobalVariableRegistry::Named(
    ScriptNamedVariable variable) const {
    return named_[NamedIndex(variable)];
}

}  // namespace pal4
