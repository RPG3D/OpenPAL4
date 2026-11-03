#include "pal4/script_dialog_registration.h"

#include <array>
#include <string_view>

namespace pal4 {
namespace {

constexpr char kDescription[] =
    "\x5b\xcb\xb5\xc3\xf7\x5d\x0a\xbd\xc5\xb1\xbe\xcf\xb5\xcd\xb3\xd1\xd3\xca\xb1"
    "\xb5\xc8\xb4\xfd\xd6\xb8\xc1\xee\xa3\xac\xb4\xcb\xd6\xb8\xc1\xee\xba\xf3"
    "\xd0\xf8\xb5\xc4\xd6\xb8\xc1\xee\xbd\xab\xd6\xd0\xb6\xcf\xd6\xb4\xd0\xd0"
    "\xa3\xac\xd6\xb1\xd6\xc1\xbc\xc6\xca\xb1\xbd\xe1\xca\xf8\xa1\xa3\x0a"
    "\xcd\xac\xb2\xbd\xb7\xbd\xca\xbd\x3a\x09\x09\xd6\xa7\xb3\xd6\xa3\xac"
    "\xc7\xbf\xd0\xd0\xd6\xd0\xb6\xcf\xa1\xa3\x0a\x70\x61\x72\x61\x6d"
    "\x5b\x69\x6e\x5d\x09\x09\x64\x54\x69\x6d\x65\x09\x09\xb5\xc8"
    "\xb4\xfd\xca\xb1\xbc\xe4";
constexpr char kSignature[] = "void giWait( double )";
constexpr char kTalkDescription[] =
    "\x5b\xcb\xb5\xc3\xf7\x5d\x0a\xc9\xe8\xd6\xc3\xb6\xd4\xbb\xb0\xc4\xda"
    "\xc8\xdd\x0a\xcd\xac\xb2\xbd\xb7\xbd\xca\xbd\x3a\x09\x09\xd6\xa7"
    "\xb3\xd6\xa1\xa3\x0a\x70\x61\x72\x61\x6d\x5b\x69\x6e\x5d\x09"
    "\x09\x73\x74\x72\x69\x6e\x67\x09\x61\x72\x67\x31\x09\xb6\xd4"
    "\xbb\xb0\xc4\xda\xc8\xdd\x0a\x70\x61\x72\x61\x6d\x5b\x69\x6e"
    "\x5d\x09\x09\x73\x74\x72\x69\x6e\x67\x09\x61\x72\x67\x32\x09"
    "\xd2\xf4\xd0\xa7\xc3\xfb\xb3\xc6\x0a";
constexpr char kTalkSignature[] = "void giTalk( string &in, string &in )";
constexpr char kRandTalkPushDescription[] =
    "\x5b\xcb\xb5\xc3\xf7\x5d\x0a\xbc\xd3\xc8\xeb\xcb\xe6\xbb\xfa\xb6"
    "\xd4\xbb\xb0\xc4\xda\xc8\xdd\x0a\xcd\xac\xb2\xbd\xb7\xbd\xca"
    "\xbd\x3a\x09\x09\xc1\xa2\xbc\xb4\xa1\xa3\x0a\x70\x61\x72\x61"
    "\x6d\x5b\x69\x6e\x5d\x09\x09\x73\x74\x72\x69\x6e\x67\x09\x61"
    "\x72\x67\x31\x09\xb6\xd4\xbb\xb0\xc4\xda\xc8\xdd\x0a";
constexpr char kRandTalkPushSignature[] = "void giRandTalkPush( string &in )";
constexpr char kRandTalkDescription[] =
    "\x5b\xcb\xb5\xc3\xf7\x5d\x0a\xcb\xe6\xbb\xfa\xb6\xd4\xbb\xb0"
    "\xa3\xac\xc7\xb0\xcc\xe2\xb1\xd8\xd0\xeb\xd2\xaa\xbc\xd3\xc8\xeb"
    "\xbf\xc9\xd1\xa1\xd4\xf1\xb5\xc4\xb6\xd4\xbb\xb0\xc4\xda\xc8\xdd\x0a";
constexpr char kRandTalkSignature[] = "void giRandTalk()";
constexpr char kRandTalkReleaseDescription[] =
    "\x5b\xcb\xb5\xc3\xf7\x5d\x0a\xc7\xe5\xbf\xd5\xcb\xe6\xbb\xfa"
    "\xb6\xd4\xbb\xb0\xc4\xda\xc8\xdd\x0a";
constexpr char kRandTalkReleaseSignature[] = "void giRandTalkRelease()";
constexpr std::uint32_t kCommit = 0x1000u;
constexpr std::uint32_t kReadWrite = 4u;
constexpr std::uint32_t kRelease = 0x8000u;

const std::array<ScriptDialogLateSpec, ScriptDialogLateRegistry::kCount>
    kLateSpecs{{
#include "pal4/detail/script_dialog_late_specs.inc"
    }};

void ConstructDescriptor(IScriptBuiltinRuntime& runtime,
                         ScriptBuiltinDescriptor& descriptor,
                         std::uint32_t address, std::string_view description,
                         std::string_view signature, std::uint32_t handler) {
    descriptor = {};
    descriptor.description_size = static_cast<std::uint32_t>(description.size());
    descriptor.description = runtime.Allocate(descriptor.description_size,
                                              kCommit, kReadWrite);
    runtime.CopyString(descriptor.description, description);
    descriptor.signature_size = static_cast<std::uint32_t>(signature.size());
    descriptor.signature = runtime.Allocate(descriptor.signature_size,
                                            kCommit, kReadWrite);
    runtime.CopyString(descriptor.signature, signature);
    descriptor.callback_data[0] = handler;
    runtime.Register(address, descriptor);
}

void CleanupDescriptor(IScriptBuiltinRuntime& runtime,
                       ScriptBuiltinDescriptor& descriptor) {
    if (descriptor.signature != 0u)
        runtime.Release(descriptor.signature, descriptor.signature_size,
                        kRelease);
    descriptor.signature = 0u;
    descriptor.signature_size = 0u;
    if (descriptor.description != 0u)
        runtime.Release(descriptor.description, descriptor.description_size,
                        kRelease);
    descriptor.description = 0u;
    descriptor.description_size = 0u;
}

}  // namespace

int ScriptWaitRegistration::Initialize() {
    static_cast<void>(Construct());
    return runtime_.RegisterAtExit(kCleanupCallback);
}

std::uint32_t ScriptWaitRegistration::Construct() {
    ConstructDescriptor(runtime_, descriptor_, kDescriptorAddress,
                        {kDescription, sizeof(kDescription)},
                        {kSignature, sizeof(kSignature)}, kHandlerAddress);
    return kDescriptorAddress;
}

void ScriptWaitRegistration::Cleanup() {
    CleanupDescriptor(runtime_, descriptor_);
}

int ScriptTalkRegistration::Initialize() {
    static_cast<void>(Construct());
    return runtime_.RegisterAtExit(kCleanupCallback);
}

std::uint32_t ScriptTalkRegistration::Construct() {
    ConstructDescriptor(runtime_, descriptor_, kDescriptorAddress,
                        {kTalkDescription, sizeof(kTalkDescription)},
                        {kTalkSignature, sizeof(kTalkSignature)}, kHandlerAddress);
    return kDescriptorAddress;
}

void ScriptTalkRegistration::Cleanup() {
    CleanupDescriptor(runtime_, descriptor_);
}

int ScriptRandTalkPushRegistration::Initialize() {
    static_cast<void>(Construct());
    return runtime_.RegisterAtExit(kCleanupCallback);
}

std::uint32_t ScriptRandTalkPushRegistration::Construct() {
    ConstructDescriptor(runtime_, descriptor_, kDescriptorAddress,
                        {kRandTalkPushDescription,
                         sizeof(kRandTalkPushDescription)},
                        {kRandTalkPushSignature,
                         sizeof(kRandTalkPushSignature)},
                        kHandlerAddress);
    return kDescriptorAddress;
}

void ScriptRandTalkPushRegistration::Cleanup() {
    CleanupDescriptor(runtime_, descriptor_);
}

int ScriptRandTalkRegistration::Initialize() {
    static_cast<void>(Construct());
    return runtime_.RegisterAtExit(kCleanupCallback);
}

std::uint32_t ScriptRandTalkRegistration::Construct() {
    ConstructDescriptor(runtime_, descriptor_, kDescriptorAddress,
                        {kRandTalkDescription, sizeof(kRandTalkDescription)},
                        {kRandTalkSignature, sizeof(kRandTalkSignature)},
                        kHandlerAddress);
    return kDescriptorAddress;
}

void ScriptRandTalkRegistration::Cleanup() {
    CleanupDescriptor(runtime_, descriptor_);
}

int ScriptRandTalkReleaseRegistration::Initialize() {
    static_cast<void>(Construct());
    return runtime_.RegisterAtExit(kCleanupCallback);
}

std::uint32_t ScriptRandTalkReleaseRegistration::Construct() {
    ConstructDescriptor(runtime_, descriptor_, kDescriptorAddress,
                        {kRandTalkReleaseDescription,
                         sizeof(kRandTalkReleaseDescription)},
                        {kRandTalkReleaseSignature,
                         sizeof(kRandTalkReleaseSignature)},
                        kHandlerAddress);
    return kDescriptorAddress;
}

void ScriptRandTalkReleaseRegistration::Cleanup() {
    CleanupDescriptor(runtime_, descriptor_);
}

const ScriptDialogLateSpec& ScriptDialogLateRegistry::Describe(
    std::size_t index) {
    return kLateSpecs.at(index);
}

int ScriptDialogLateRegistry::Initialize(std::size_t index) {
    static_cast<void>(Construct(index));
    return runtime_.RegisterAtExit(Describe(index).cleanup_entry);
}

std::uint32_t ScriptDialogLateRegistry::Construct(std::size_t index) {
    const auto& spec = Describe(index);
    ConstructDescriptor(runtime_, descriptors_.at(index),
                        spec.descriptor_address, spec.description_with_nul,
                        spec.signature_with_nul, spec.handler_address);
    return spec.descriptor_address;
}

void ScriptDialogLateRegistry::Cleanup(std::size_t index) {
    static_cast<void>(Describe(index));
    CleanupDescriptor(runtime_, descriptors_.at(index));
}

const ScriptBuiltinDescriptor& ScriptDialogLateRegistry::Descriptor(
    std::size_t index) const {
    return descriptors_.at(index);
}

}  // namespace pal4
