#include "pal4/utilities_table_binding_refresh.h"

namespace pal4 {
namespace {

void Refresh(std::uint32_t object, UtilitiesBindingSchema schema,
             IUtilitiesTableBindingRuntime& runtime) {
    const auto bindings = runtime.ReadWord(object + 128);
    const auto binding_count = bindings
        ? (runtime.ReadWord(object + 132) - bindings) / 4 : 0;
    const auto fields = runtime.ReadWord(object + 24);
    const auto field_count = fields
        ? (runtime.ReadWord(object + 28) - fields) / 60 : 0;
    if (binding_count == field_count) return;

    runtime.LogBind("beginBindField, table:", object + 52);
    runtime.AssignEffects(object + 108, object + 20);
    runtime.RegisterFields(object + 108, schema);
    runtime.BindEffects(object + 108);
    runtime.LogBind("endBindField, table:", object + 52);
}

}  // namespace

void UtilitiesTableBindingRefresh::IdString(
    std::uint32_t object, IUtilitiesTableBindingRuntime& runtime) {
    Refresh(object, UtilitiesBindingSchema::IdString, runtime);
}

void UtilitiesTableBindingRefresh::PreviewInfo(
    std::uint32_t object, IUtilitiesTableBindingRuntime& runtime) {
    Refresh(object, UtilitiesBindingSchema::PreviewInfo, runtime);
}

void UtilitiesTableBindingRefresh::Introduction(
    std::uint32_t object, IUtilitiesTableBindingRuntime& runtime) {
    Refresh(object, UtilitiesBindingSchema::Introduction, runtime);
}

void UtilitiesTableBindingRefresh::Scene(
    std::uint32_t object, IUtilitiesTableBindingRuntime& runtime) {
    Refresh(object, UtilitiesBindingSchema::Scene, runtime);
}

void UtilitiesTableBindingRefresh::Window(
    std::uint32_t object, IUtilitiesTableBindingRuntime& runtime) {
    Refresh(object, UtilitiesBindingSchema::Window, runtime);
}

void UtilitiesTableBindingRefresh::StringId(
    std::uint32_t object, IUtilitiesTableBindingRuntime& runtime) {
    Refresh(object, UtilitiesBindingSchema::StringId, runtime);
}

void UtilitiesTableBindingRefresh::World(
    std::uint32_t object, IUtilitiesTableBindingRuntime& runtime) {
    Refresh(object, UtilitiesBindingSchema::World, runtime);
}

}  // namespace pal4
