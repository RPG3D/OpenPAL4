#include "pal4/pal_floor_attribute.h"
#include "pal4/floor_sound_lookup.h"
#include "pal4/floor_color_tree.h"
#include "pal4/floor_color_sample.h"

#include <cmath>
#include <stdexcept>

namespace pal4 {

std::uint8_t PalFloorAttributeRuntimeStub::ReadByte(std::uint32_t) { return 0u; }
std::uint32_t PalFloorAttributeRuntimeStub::ReadDword(std::uint32_t) { return 0u; }
void PalFloorAttributeRuntimeStub::WriteByte(std::uint32_t, std::uint8_t) {}
void PalFloorAttributeRuntimeStub::WriteDword(std::uint32_t, std::uint32_t) {}
std::uint32_t PalFloorAttributeRuntimeStub::EmptyTextAddress() { return 0x841964u; }
void PalFloorAttributeRuntimeStub::SetErrorFlag(std::uint32_t, std::uint8_t) {}
bool PalFloorAttributeRuntimeStub::AllocateText(
    std::uint32_t, std::uint32_t, std::uint8_t) { return false; }
void PalFloorAttributeRuntimeStub::DetachAndReset(std::uint32_t) {}
void PalFloorAttributeRuntimeStub::SetTextSize(std::uint32_t, std::uint32_t) {}
void PalFloorAttributeRuntimeStub::CopyBytes(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
void PalFloorAttributeRuntimeStub::ThrowRange() {
    throw std::out_of_range("floor text self-copy");
}
std::int32_t PalFloorAttributeRuntimeStub::FloatToLong(long double value) {
    return static_cast<std::int32_t>(std::nearbyint(value));
}
std::int32_t PalFloorAttributeRuntimeStub::CompareCaseInsensitive(
    std::uint32_t, std::uint32_t) { return 0; }
std::uint32_t PalFloorAttributeRuntimeStub::GameDatabase() { return 0u; }
std::uint32_t PalFloorAttributeRuntimeStub::GetSingletonManager() { return 0u; }
std::uint32_t PalFloorAttributeRuntimeStub::Register(
    std::uint32_t, std::string_view, std::uint32_t,
    std::uint32_t, std::uint32_t) { return 0u; }
PalFloorAttributeState* PalFloorAttributeRuntimeStub::Allocate(
    std::uint32_t) { return nullptr; }
std::uint32_t PalFloorAttributeRuntimeStub::Free(PalFloorAttributeState*) {
    return 0u;
}
std::uint32_t PalFloorAttributeRuntimeStub::CreateEmptyList() { return 0u; }
FloorColorTreeGlobals& PalFloorAttributeRuntimeStub::ColorTreeGlobals() {
    return color_tree_globals_;
}
std::uint32_t PalFloorAttributeRuntimeStub::RgbaToPixel(
    std::uint32_t rgba, std::uint32_t) { return rgba; }
std::uint32_t PalFloorAttributeRuntimeStub::PrepareColorValue(
    std::string_view, std::uint32_t) { return 0u; }
void PalFloorAttributeRuntimeStub::ReleaseColorValue(std::uint32_t) {}
void PalFloorAttributeRuntimeStub::ReleaseText(FloorTextRecord&) {}
void PalFloorAttributeRuntimeStub::ReleaseList(PalFloorAttributeState&) {}
std::uint32_t PalFloorAttributeRuntimeStub::MovementAdapter() { return 0u; }
std::uint32_t PalFloorAttributeRuntimeStub::CallMovementVirtual(
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t PalFloorAttributeRuntimeStub::AllocateListNode(
    std::uint32_t) { return 0u; }
std::uint32_t PalFloorAttributeRuntimeStub::AllocateColorNode(
    std::uint32_t) { return 0u; }
std::uint8_t PalFloorAttributeRuntimeStub::ConstructorStackByte(
    std::uint32_t) { return 0u; }
void PalFloorAttributeRuntimeStub::AssignSoundPath(
    FloorTextRecord&, std::uint32_t) {}
void PalFloorAttributeRuntimeStub::AllocateEmptySoundPath(FloorTextRecord&) {}
void PalFloorAttributeRuntimeStub::InvokeCallback(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
std::uint32_t PalFloorAttributeRuntimeStub::NextColorNode(
    std::uint32_t) { return 0u; }
void PalFloorAttributeRuntimeStub::DeleteColorNode(
    std::uint32_t, FloorColorTreeState&) {}
std::uint32_t PalFloorAttributeRuntimeStub::FreeAddress(std::uint32_t) {
    return 0u;
}

std::uint32_t FloorCallbackNode::CopyPayload(
    std::uint32_t destination, const FloorCallbackPayload& source,
    IPalFloorAttributeRuntime& runtime) {
    if (destination != 0u) {
        runtime.WriteDword(destination, source.pixel);
        runtime.WriteDword(destination + 4u, 0u);
        runtime.WriteDword(destination + 8u, 0u);
        runtime.WriteDword(destination + 8u, source.callback);
        runtime.WriteDword(destination + 4u, source.context);
    }
    return destination;
}

std::int32_t FloorSoundRecordCollection::Count(
    std::uint32_t collection, IPalFloorAttributeRuntime& runtime) {
    const auto begin = runtime.ReadDword(collection + 4u);
    if (begin == 0u) return 0;
    const auto end = runtime.ReadDword(collection + 8u);
    return static_cast<std::int32_t>(end - begin) / 40;
}

const std::array<FloorColorSound, 15>& PalFloorAttribute::ColorSounds() {
    return kColorSounds;
}

std::int32_t PalFloorAttribute::ScaleValue(
    std::int32_t value, IPalFloorAttributeRuntime& runtime) {
    return runtime.FloatToLong(FloorSoundLookup::ScaleInput(value));
}

bool PalFloorAttribute::Matches(std::uint32_t query_text,
                                std::int32_t scaled_value,
                                std::uint32_t record,
                                IPalFloorAttributeRuntime& runtime) {
    if (static_cast<std::int32_t>(runtime.ReadDword(record + 20u)) !=
        scaled_value) return false;
    auto query_data = runtime.ReadDword(query_text + 4u);
    auto record_data = runtime.ReadDword(record + 8u);
    if (query_data == 0u) query_data = runtime.EmptyTextAddress();
    if (record_data == 0u) record_data = runtime.EmptyTextAddress();
    return runtime.CompareCaseInsensitive(record_data, query_data) == 0;
}

std::uint32_t PalFloorAttribute::GameDatabaseData(
    std::uint32_t game_db_member, IPalFloorAttributeRuntime& runtime) {
    return runtime.ReadDword(game_db_member + 24u);
}

std::uint32_t PalFloorAttribute::FindSoundRecord(
    std::uint32_t query_text, std::int32_t value,
    IPalFloorAttributeRuntime& runtime) {
    const auto scaled = ScaleValue(value, runtime);
    const auto database = runtime.GameDatabase();
    const auto data = GameDatabaseData(database + 52u, runtime);
    const auto records = runtime.ReadDword(data + 96u);
    const auto count = FloorSoundRecordCollection::Count(data + 92u,
                                                         runtime);
    for (std::uint32_t index = 0;
         index < static_cast<std::uint32_t>(count); ++index) {
        const auto record = records + index * 40u;
        if (Matches(query_text, scaled, record, runtime)) return record;
    }
    return 0u;
}

void PalFloorAttribute::GetSoundPath(
    std::int32_t current, std::int32_t previous,
    std::uint32_t query_text, std::uint32_t output_text,
    IPalFloorAttributeRuntime& runtime) {
    const auto candidates = FloorSoundLookup::Candidates(current, previous);
    for (const auto value : candidates) {
        const auto record = FindSoundRecord(query_text, value, runtime);
        if (record != 0u) {
            CeguiTextRecord::Copy(output_text, record + 24u, runtime);
            return;
        }
    }
    runtime.WriteByte(output_text,
                      static_cast<std::uint8_t>(output_text));
    runtime.WriteDword(output_text + 4u, 0u);
    runtime.WriteDword(output_text + 8u, 0u);
    runtime.WriteDword(output_text + 12u, 0u);
    if (runtime.AllocateText(output_text, 0u, 1u)) {
        const auto data = runtime.ReadDword(output_text + 4u);
        runtime.WriteByte(data, 0u);
    }
}

std::uint32_t PalFloorAttribute::GetSingleton(
    PalFloorAttributeAccessState& access,
    IPalFloorAttributeRuntime& runtime) {
    if ((access.guard & 1u) != 0u) return access.instance;
    access.guard = static_cast<std::uint8_t>(access.guard | 1u);
    access.instance = runtime.Register(runtime.GetSingletonManager(),
        "palFloorAttributeProcessor", 1015u,
        kFactoryAddress, kDestroyAddress);
    return access.instance;
}

PalFloorAttributeState* PalFloorAttribute::Create(
    IPalFloorAttributeRuntime& runtime) {
    auto* state = runtime.Allocate(60u);
    if (state == nullptr) return nullptr;
    try {
        Construct(*state, runtime);
    } catch (...) {
        static_cast<void>(runtime.Free(state));
        throw;
    }
    return state;
}

std::uint32_t PalFloorAttribute::Destroy(
    PalFloorAttributeState* state, std::uint32_t incoming_eax,
    IPalFloorAttributeRuntime& runtime) {
    if (state == nullptr) return incoming_eax;
    Destruct(*state, runtime);
    return runtime.Free(state);
}

void PalFloorAttribute::Construct(
    PalFloorAttributeState& state, IPalFloorAttributeRuntime& runtime) {
    state.current_alpha = 0u;
    state.fallback_alpha = 0u;
    state.list_head = runtime.CreateEmptyList();
    state.list_size = 0u;
    FloorColorTree::Initialize(state,
        runtime.ConstructorStackByte(0u),
        runtime.ConstructorStackByte(1u), false, runtime);
    state.text.data = 0u;
    state.text.length = 0u;
    state.text.capacity = 0u;
    try {
        for (const auto& entry : kColorSounds) {
            const auto rgba = entry.rgb | 0xff000000u;
            const auto pixel = runtime.RgbaToPixel(rgba, 1280u);
            const auto value = runtime.PrepareColorValue(entry.path, pixel);
            try {
                static_cast<void>(FloorColorTree::Insert(
                    state.color_tree, value, runtime));
            } catch (...) {
                runtime.ReleaseColorValue(value);
                throw;
            }
            runtime.ReleaseColorValue(value);
        }
        state.previous_color = 0u;
        state.current_color = runtime.RgbaToPixel(0xff000000u, 1280u);
    } catch (...) {
        runtime.ReleaseText(state.text);
        DestroyColorTree(state.color_tree, runtime);
        runtime.ReleaseList(state);
        throw;
    }
}

void PalFloorAttribute::Destruct(
    PalFloorAttributeState& state, IPalFloorAttributeRuntime& runtime) {
    runtime.ReleaseText(state.text);
    auto& tree = state.color_tree;
    const auto header = tree.header;
    static_cast<void>(FloorColorTree::EraseRange(
        tree, runtime.ReadDword(header), header, runtime));
    static_cast<void>(runtime.FreeAddress(header));
    tree.header = 0u;
    tree.size = 0u;
    FloorColorTree::ReleaseSharedNil(runtime);
    runtime.ReleaseList(state);
}

void PalFloorAttribute::DestroyColorTree(
    FloorColorTreeState& tree, IPalFloorAttributeRuntime& runtime) {
    const auto header = tree.header;
    auto node = runtime.ReadDword(header);
    if (tree.size != 0u) {
        static_cast<void>(FloorColorTree::DestroySubtree(
            tree, runtime.ReadDword(header + 4u), runtime));
        runtime.WriteDword(header + 4u, runtime.ColorTreeGlobals().nil);
        tree.size = 0u;
        runtime.WriteDword(header, header);
        runtime.WriteDword(header + 8u, header);
    } else {
        while (node != header) {
            const auto next = runtime.NextColorNode(node);
            runtime.DeleteColorNode(node, tree);
            node = next;
        }
    }
    static_cast<void>(runtime.FreeAddress(header));
    tree.header = 0u;
    tree.size = 0u;
    FloorColorTree::ReleaseSharedNil(runtime);
}

std::uint32_t PalFloorAttribute::UpdateColorFromMovement(
    PalFloorAttributeState& state, IPalFloorAttributeRuntime& runtime) {
    const auto adapter = runtime.MovementAdapter();
    const auto movement = runtime.ReadDword(adapter + 8u);
    if (movement == 0u) return adapter;
    const auto clump = runtime.CallMovementVirtual(movement, 40u);
    if (clump == 0u) return clump;
    const auto triangle = runtime.CallMovementVirtual(movement, 36u);
    const auto mesh = runtime.ReadDword(clump + 24u);
    const auto colors = runtime.ReadDword(mesh + 48u);
    if (colors == 0u) return triangle;
    const auto triangles = runtime.ReadDword(mesh + 44u);
    const auto first_pair = runtime.ReadDword(triangles + triangle * 8u);
    const auto second_pair = runtime.ReadDword(triangles + triangle * 8u + 4u);
    const std::array<std::uint16_t, 3> indices{
        static_cast<std::uint16_t>(first_pair),
        static_cast<std::uint16_t>(first_pair >> 16u),
        static_cast<std::uint16_t>(second_pair)};
    std::array<std::uint32_t, 3> raw{};
    for (std::size_t index = 0; index < 3u; ++index)
        raw[index] = runtime.ReadDword(colors + indices[index] * 4u);
    return UpdateFloorColorSample(raw, state.current_color, state.fallback_alpha,
        state.current_alpha,
        [&](std::uint32_t rgba) { return runtime.RgbaToPixel(rgba, 1280u); },
        [&](std::uint32_t pixel) { ApplyColor(state, pixel, runtime); });
}

std::uint32_t PalFloorAttribute::AddFloorAttribute(
    PalFloorAttributeState& state, std::uint32_t pixel,
    std::uint32_t context, std::uint32_t callback,
    IPalFloorAttributeRuntime& runtime) {
    const auto head = state.list_head;
    auto prior = runtime.ReadDword(head + 4u);
    const auto node = runtime.AllocateListNode(20u);
    runtime.WriteDword(node, head);
    if (prior == 0u) prior = node;
    runtime.WriteDword(node + 4u, prior);
    runtime.WriteDword(head + 4u, node);
    runtime.WriteDword(prior, node);
    static_cast<void>(FloorCallbackNode::CopyPayload(
        node + 8u, {pixel, context, callback}, runtime));
    return ++state.list_size;
}

void PalFloorAttribute::ApplyColor(
    PalFloorAttributeState& state, std::uint32_t pixel,
    IPalFloorAttributeRuntime& runtime) {
    const auto found = FloorColorTree::Find(state.color_tree, pixel, runtime);
    if (found == state.color_tree.header) {
        runtime.AllocateEmptySoundPath(state.text);
    } else {
        runtime.AssignSoundPath(state.text, found + 16u);
    }
    const auto head = state.list_head;
    bool matched = false;
    for (auto node = runtime.ReadDword(head); node != head;
         node = runtime.ReadDword(node)) {
        if (runtime.ReadDword(node + 8u) == pixel) {
            runtime.InvokeCallback(runtime.ReadDword(node + 16u),
                                   runtime.ReadDword(node + 12u), pixel);
            matched = true;
        }
    }
    if (!matched) {
        for (auto node = runtime.ReadDword(head); node != head;
             node = runtime.ReadDword(node)) {
            if (runtime.ReadDword(node + 8u) == state.current_color) {
                runtime.InvokeCallback(runtime.ReadDword(node + 16u),
                                       runtime.ReadDword(node + 12u),
                                       state.current_color);
            }
        }
    }
}

void PalFloorAttribute::CopyData(
    std::uint32_t destination, std::uint32_t source,
    IPalFloorAttributeRuntime& runtime) {
    const auto flag = runtime.ReadByte(source);
    runtime.WriteDword(destination + 4u, 0u);
    runtime.WriteByte(destination, flag);
    runtime.WriteDword(destination + 8u, 0u);
    runtime.WriteDword(destination + 12u, 0u);
    const auto size = runtime.ReadDword(source + 8u);
    if (destination == source) {
        if (size != 0u) runtime.ThrowRange();
        runtime.DetachAndReset(destination);
        runtime.DetachAndReset(destination);
        return;
    }
    if (size != 0u) {
        auto data = runtime.ReadDword(source + 4u);
        if (data == 0u) data = runtime.EmptyTextAddress();
        if (runtime.ReadByte(data - 1u) < 0xfeu) {
            runtime.WriteDword(destination + 4u, data);
            runtime.WriteDword(destination + 8u, size);
            runtime.WriteDword(destination + 12u,
                               runtime.ReadDword(source + 12u));
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(
                                  runtime.ReadByte(data - 1u) + 1u));
            return;
        }
    }
    if (!runtime.AllocateText(destination, size, 1u)) return;
    auto source_data = runtime.ReadDword(source + 4u);
    if (source_data == 0u) source_data = runtime.EmptyTextAddress();
    runtime.CopyBytes(runtime.ReadDword(destination + 4u), source_data, size);
    runtime.SetTextSize(destination, size);
}

}  // namespace pal4
