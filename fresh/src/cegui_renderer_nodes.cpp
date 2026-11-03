#include "pal4/cegui_renderer_nodes.h"
#include "pal4/cegui_packed_color.h"

#include <bit>
#include <new>
#include <stdexcept>

namespace pal4 {

std::uint32_t CeguiRendererNodeStub::Allocate(const std::uint32_t bytes) {
    const auto address = next_address_;
    next_address_ += bytes + 0x10u;
    return address;
}

void CeguiRendererNodeStub::Free(std::uint32_t /*address*/) {}

std::uint32_t CeguiRendererNodeStub::ConstructObject36(
    const std::uint32_t address, CeguiRendererState& /*owner*/) {
    return address;
}

std::uint32_t CeguiRendererNodeStub::ReadDword(
    const std::uint32_t address) {
    return words_.at(address);
}

void CeguiRendererNodeStub::WriteDword(
    const std::uint32_t address, const std::uint32_t value) {
    words_[address] = value;
}

void CeguiRendererNodeStub::ConfigureObject(
    std::uint32_t /*object*/, std::uint32_t /*first*/,
    std::uint32_t /*second*/) {}

std::int32_t CeguiRendererNodeStub::FloatToInt(const float value) {
    CeguiHostFloatToInt host;
    return host.Convert(value);
}

void CeguiRendererNodeStub::ZeroBuffer(
    std::uint32_t /*address*/, std::uint32_t /*bytes*/) {}

void CeguiRendererNodeStub::ApplyGrid(
    std::uint32_t /*object*/, std::uint32_t /*buffer*/,
    std::int32_t /*width*/, std::int32_t /*height*/) {}

std::uint32_t CeguiRendererNodeStub::DestroyObject(
    std::uint32_t /*object*/, std::uint32_t /*flags*/) { return 0u; }

std::uint32_t CeguiRendererNodes::Create(
    CeguiRendererState& state, ICeguiRendererNodeRuntime& runtime) {
    const auto storage = runtime.Allocate(36u);
    std::uint32_t object = 0u;
    if (storage != 0u) {
        try {
            object = runtime.ConstructObject36(storage, state);
        } catch (...) {
            runtime.Free(storage);
            throw;
        }
    }

    const auto sentinel = state.ReadDword(236u);
    auto previous = runtime.ReadDword(sentinel + 4u);
    const auto node = runtime.Allocate(12u);
    if (node == 0u) {
        throw std::bad_alloc();
    }
    runtime.WriteDword(node, sentinel);
    if (previous == 0u) {
        previous = node;
    }
    runtime.WriteDword(node + 4u, previous);
    runtime.WriteDword(sentinel + 4u, node);
    runtime.WriteDword(previous, node);
    runtime.WriteDword(node + 8u, object);
    state.WriteDword(240u, state.ReadDword(240u) + 1u);
    return object;
}

std::uint32_t CeguiRendererNodes::CreateAndConfigure(
    CeguiRendererState& state, const std::uint32_t first,
    const std::uint32_t second, ICeguiRendererNodeRuntime& runtime) {
    const auto object = Create(state, runtime);
    runtime.ConfigureObject(object, first, second);
    return object;
}

std::uint32_t CeguiRendererNodes::CreateZeroGrid(
    CeguiRendererState& state, const float side,
    ICeguiRendererNodeRuntime& runtime) {
    const auto object = Create(state, runtime);
    const auto dimension = runtime.FloatToInt(side);
    const auto square = static_cast<std::uint32_t>(dimension) *
                        static_cast<std::uint32_t>(dimension);
    if (std::bit_cast<std::int32_t>(square) < 0) {
        throw std::overflow_error("grid size wraps to a negative count");
    }
    const auto bytes = 4u * square;
    const auto buffer = runtime.Allocate(bytes);
    if (buffer == 0u && bytes != 0u) {
        throw std::bad_alloc();
    }
    for (std::uint32_t index = 0u; index < square; ++index) {
        if (buffer != 0u) {
            runtime.WriteDword(buffer + 4u * index, 0u);
        }
    }
    runtime.ZeroBuffer(buffer, bytes);
    try {
        runtime.ApplyGrid(object, buffer, dimension, dimension);
    } catch (...) {
        runtime.Free(buffer);
        throw;
    }
    runtime.Free(buffer);
    return object;
}

std::optional<std::uint32_t> CeguiRendererNodes::Remove(
    CeguiRendererState& state, const std::uint32_t object,
    ICeguiRendererNodeRuntime& runtime) {
    if (object == 0u) {
        return std::nullopt;
    }
    const auto sentinel = state.ReadDword(236u);
    auto node = runtime.ReadDword(sentinel);
    while (node != sentinel) {
        const auto next = runtime.ReadDword(node);
        if (runtime.ReadDword(node + 8u) == object) {
            const auto previous = runtime.ReadDword(node + 4u);
            runtime.WriteDword(previous, next);
            runtime.WriteDword(next + 4u, previous);
            runtime.Free(node);
            state.WriteDword(240u, state.ReadDword(240u) - 1u);
        }
        node = next;
    }
    return runtime.DestroyObject(object, 1u);
}

std::uint32_t CeguiRendererNodes::Drain(
    CeguiRendererState& state, ICeguiRendererNodeRuntime& runtime) {
    while (state.ReadDword(240u) != 0u) {
        const auto sentinel = state.ReadDword(236u);
        const auto first = runtime.ReadDword(sentinel);
        const auto object = runtime.ReadDword(first + 8u);
        const auto before = state.ReadDword(240u);
        static_cast<void>(Remove(state, object, runtime));
        if (state.ReadDword(240u) == before) {
            throw std::logic_error("renderer node drain made no progress");
        }
    }
    return state.ReadDword(240u);
}

}  // namespace pal4
