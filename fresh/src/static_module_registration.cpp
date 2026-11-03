#include "pal4/static_module_registration.h"

#include <stdexcept>

namespace pal4 {
namespace {

std::uint32_t InitializerTarget(const std::uint32_t index) {
    if (index <= 15u) return 0x45ad50u + index * 0x160u;
    if (index <= 26u) return 0x45c3a0u + (index - 16u) * 0x160u;
    if (index <= 74u) return 0x45d160u + (index - 26u) * 0x140u;
    return 0x460d60u + (index - 74u) * 0x160u;
}

std::uint32_t FirstEntry(const StaticModuleSeries series) {
    switch (series) {
    case StaticModuleSeries::First: return StaticModuleRegistration::kFirstEntry;
    case StaticModuleSeries::Second: return StaticModuleRegistration::kSecondFirstEntry;
    case StaticModuleSeries::Third: return StaticModuleRegistration::kThirdFirstEntry;
    case StaticModuleSeries::Fourth: return StaticModuleRegistration::kFourthFirstEntry;
    case StaticModuleSeries::Fifth: return StaticModuleRegistration::kFifthFirstEntry;
    case StaticModuleSeries::Sixth: return StaticModuleRegistration::kSixthFirstEntry;
    case StaticModuleSeries::Seventh: return StaticModuleRegistration::kSeventhFirstEntry;
    case StaticModuleSeries::Eighth: return StaticModuleRegistration::kEighthFirstEntry;
    }
    throw std::out_of_range("static module series");
}

std::uint32_t FirstObject(const StaticModuleSeries series) {
    switch (series) {
    case StaticModuleSeries::First: return StaticModuleRegistration::kFirstObject;
    case StaticModuleSeries::Second: return StaticModuleRegistration::kSecondFirstObject;
    case StaticModuleSeries::Third: return StaticModuleRegistration::kThirdFirstObject;
    case StaticModuleSeries::Fourth: return StaticModuleRegistration::kFourthFirstObject;
    case StaticModuleSeries::Fifth: return StaticModuleRegistration::kFifthFirstObject;
    case StaticModuleSeries::Sixth: return StaticModuleRegistration::kSixthFirstObject;
    case StaticModuleSeries::Seventh: return StaticModuleRegistration::kSeventhFirstObject;
    case StaticModuleSeries::Eighth: return StaticModuleRegistration::kEighthFirstObject;
    }
    throw std::out_of_range("static module series");
}

}  // namespace

StaticModuleDescriptor StaticModuleRegistration::Describe(
    const std::uint32_t index) {
    return Describe(StaticModuleSeries::First, index);
}

StaticModuleDescriptor StaticModuleRegistration::Describe(
    const StaticModuleSeries series, const std::uint32_t index) {
    const auto count = series == StaticModuleSeries::Sixth
                           ? kSixthCount : kCount;
    if (index >= count) throw std::out_of_range("static module index");
    const auto first_entry = FirstEntry(series);
    const auto first_object = FirstObject(series);
    const auto registration = first_entry + index * kEntryStride;
    return {registration, registration + 0x10u,
            registration + 0x30u,
            first_object - index * kObjectStride,
            InitializerTarget(index +
                              (series == StaticModuleSeries::Sixth ? 1u : 0u)),
            kCleanupTarget};
}

int StaticModuleRegistration::Register(
    const std::uint32_t index, IStaticModuleRuntime& runtime) {
    return Register(StaticModuleSeries::First, index, runtime);
}

int StaticModuleRegistration::Register(
    const StaticModuleSeries series, const std::uint32_t index,
    IStaticModuleRuntime& runtime) {
    const auto entry = Describe(series, index);
    runtime.Initialize(entry.object_address, entry.initializer_target);
    return runtime.RegisterAtExit(entry.cleanup_entry);
}

void StaticModuleRegistration::Initialize(
    const std::uint32_t index, IStaticModuleRuntime& runtime) {
    Initialize(StaticModuleSeries::First, index, runtime);
}

void StaticModuleRegistration::Initialize(
    const StaticModuleSeries series, const std::uint32_t index,
    IStaticModuleRuntime& runtime) {
    const auto entry = Describe(series, index);
    runtime.Initialize(entry.object_address, entry.initializer_target);
}

void StaticModuleRegistration::Cleanup(
    const std::uint32_t index, IStaticModuleRuntime& runtime) {
    Cleanup(StaticModuleSeries::First, index, runtime);
}

void StaticModuleRegistration::Cleanup(
    const StaticModuleSeries series, const std::uint32_t index,
    IStaticModuleRuntime& runtime) {
    const auto entry = Describe(series, index);
    runtime.Cleanup(entry.object_address, entry.cleanup_target);
}

}  // namespace pal4
