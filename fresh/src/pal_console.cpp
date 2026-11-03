#include "pal4/pal_console.h"

#include <algorithm>

namespace pal4 {

std::uint32_t PalConsoleAccess::Get(
    PalConsoleAccessState& state, IPalConsoleAccessRuntime& runtime) {
    if ((state.guard & 1u) != 0u) {
        return state.instance;
    }
    state.guard = static_cast<std::uint8_t>(state.guard | 1u);
    state.instance = runtime.Register(
        runtime.GetSingletonManager(), "palConsole", 1008u,
        kFactoryAddress, kDestroyAddress);
    return state.instance;
}

std::uint32_t PalConsoleState::ReadDword(const std::uint32_t offset) const {
    return static_cast<std::uint32_t>(bytes_.at(offset)) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 1u)) << 8u) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 2u)) << 16u) |
           (static_cast<std::uint32_t>(bytes_.at(offset + 3u)) << 24u);
}

void PalConsoleState::WriteDword(const std::uint32_t offset,
                                 const std::uint32_t value) {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        bytes_.at(offset + index) =
            static_cast<std::uint8_t>(value >> (8u * index));
    }
}

void PalConsoleState::Fill(const std::uint32_t offset,
                           const std::uint32_t length,
                           const std::uint8_t value) {
    for (std::uint32_t index = 0u; index < length; ++index) {
        bytes_.at(offset + index) = value;
    }
}

std::uint32_t PalConsole::Create(const std::uint8_t incoming_stack_byte,
                                 IPalConsoleObjectRuntime& runtime) {
    const auto address = runtime.Allocate(PalConsoleState::kBytes);
    if (address == 0u) {
        return 0u;
    }
    try {
        return Construct(address, incoming_stack_byte, runtime);
    } catch (...) {
        static_cast<void>(runtime.Free(address));
        throw;
    }
}

std::uint32_t PalConsole::Construct(const std::uint32_t address,
                                    const std::uint8_t incoming_stack_byte,
                                    IPalConsoleObjectRuntime& runtime) {
    auto& state = runtime.Access(address);
    state.Bytes().at(0x4a70u) = incoming_stack_byte;
    state.WriteDword(PalConsoleState::kHistoryBegin, 0u);
    state.WriteDword(PalConsoleState::kHistoryEnd, 0u);
    state.WriteDword(PalConsoleState::kHistoryCapacity, 0u);
    state.WriteDword(0x08u, 300u);
    state.WriteDword(0x0cu, 400u);
    state.WriteDword(0x20u, 300u);
    state.WriteDword(0x1cu, 400u);
    state.WriteDword(0x04u, 0u);
    state.WriteDword(0x10u, 320u);
    state.WriteDword(0x18u, 0u);
    state.WriteDword(0x14u, 0u);
    state.WriteDword(0x24u, 0xffffff00u);
    state.WriteDword(0x28u, 0xffffffffu);
    state.WriteDword(PalConsoleState::kCommandCount, 0u);
    state.Fill(0x2cu, 0x64u, 0u);
    state.Bytes().at(0x2cu) = 0x5fu;
    state.WriteDword(0x4a80u, 0u);
    state.Fill(0x90u, 0x5dcu, 0u);
    state.Fill(0x66cu, 0x4400u, 0u);
    state.WriteDword(0u, 0u);
    return address;
}

void PalConsole::Destroy(const std::uint32_t address,
                         IPalConsoleObjectRuntime& runtime) {
    if (address != 0u) {
        static_cast<void>(Cleanup(address, runtime));
        static_cast<void>(runtime.Free(address));
    }
}

std::uint32_t PalConsole::Cleanup(const std::uint32_t address,
                                  IPalConsoleObjectRuntime& runtime) {
    auto& state = runtime.Access(address);
    return PalConsoleHistory::Cleanup(state, runtime);
}

std::uint32_t PalConsoleHistory::Cleanup(
    PalConsoleState& state, IPalConsoleObjectRuntime& runtime) {
    const auto begin = state.ReadDword(PalConsoleState::kHistoryBegin);
    const auto end = state.ReadDword(PalConsoleState::kHistoryEnd);
    for (auto record = begin; record != end; record += 0x10u) {
        const auto text = runtime.ReadDword(record + 4u);
        if (text != 0u) {
            const auto header = text - 1u;
            const auto count = runtime.ReadByte(header);
            if (count == 0u || count == 0xffu) {
                static_cast<void>(runtime.Free(header));
            } else {
                runtime.WriteByte(header,
                                  static_cast<std::uint8_t>(count - 1u));
            }
        }
        runtime.WriteDword(record + 4u, 0u);
        runtime.WriteDword(record + 8u, 0u);
        runtime.WriteDword(record + 12u, 0u);
    }
    const auto result = runtime.Free(begin);
    state.WriteDword(PalConsoleState::kHistoryBegin, 0u);
    state.WriteDword(PalConsoleState::kHistoryEnd, 0u);
    state.WriteDword(PalConsoleState::kHistoryCapacity, 0u);
    return result;
}

void PalConsole::Render(const std::uint32_t address,
                        const std::uint32_t context, PalConsoleState& state,
                        IPalConsoleRenderRuntime& runtime) {
    if (state.ReadDword(0u) == 0u) {
        return;
    }
    const auto signed_field = [&state](const std::uint32_t offset) {
        return static_cast<std::int32_t>(state.ReadDword(offset));
    };
    const auto wrapped_subtract = [](const std::uint32_t left,
                                     const std::uint32_t right) {
        return static_cast<std::int32_t>(left - right);
    };
    runtime.DrawFilledRectangle(
        context,
        {signed_field(0x04u), signed_field(0x18u),
         wrapped_subtract(state.ReadDword(0x0cu), state.ReadDword(0x04u)),
         wrapped_subtract(state.ReadDword(0x10u), state.ReadDword(0x18u))},
        {90u, 90u, 90u, 155u});
    runtime.DrawFontRectangle(
        runtime.GetFontManager(), address + 0x2cu,
        {signed_field(0x04u), signed_field(0x08u),
         signed_field(0x0cu), signed_field(0x10u)},
        1u, state.ReadDword(0x24u), 0u);
    for (std::int32_t line = 0; line >= -252; line -= 18) {
        const auto text = address + 0x608u +
                          static_cast<std::uint32_t>(line / 18 * 100);
        if (runtime.StringLength(text) == 0u) {
            continue;
        }
        const auto bottom = static_cast<std::int32_t>(
            state.ReadDword(0x20u) + static_cast<std::uint32_t>(line));
        const auto top = std::max(
            static_cast<std::int32_t>(
                static_cast<std::uint32_t>(bottom) - 18u),
            signed_field(0x18u));
        runtime.DrawFontRectangle(
            runtime.GetFontManager(), text,
            {signed_field(0x14u), top, signed_field(0x1cu), bottom},
            1u, state.ReadDword(0x28u), 8u);
    }
}

bool PalConsole::AddCommand(const std::uint32_t address,
                            const std::uint32_t callback, const char* name,
                            const char* description,
                            IPalConsoleObjectRuntime& runtime) {
    auto& state = runtime.Access(address);
    const auto index = static_cast<std::int32_t>(
        state.ReadDword(PalConsoleState::kCommandCount));
    if (index >= 64) {
        return false;
    }
    const auto stride = static_cast<std::uint32_t>(
        static_cast<std::int64_t>(index) * 136);
    runtime.WriteDword(address + 0x66cu + stride, callback);
    runtime.CopyString(address + 0x670u + stride, name, 0x20u);
    runtime.CopyString(address + 0x690u + stride, description, 0x64u);
    state.WriteDword(PalConsoleState::kCommandCount,
                     static_cast<std::uint32_t>(index) + 1u);
    return true;
}

std::uint32_t PalConsole::AddOutputLine(
    const std::uint32_t address, IPalConsoleOutputRuntime& runtime,
    const char* format, ...) {
    char buffer[256]{};
    std::va_list args;
    va_start(args, format);
    try {
        runtime.FormatToBuffer(buffer, sizeof(buffer), format, args);
    } catch (...) {
        va_end(args);
        throw;
    }
    va_end(args);
    runtime.CopyBytes(address + 0x90u, address + 0xf4u, 0x578u);
    return runtime.CopyString(address + 0x608u, buffer, 0x63u);
}

std::uint32_t PalConsole::CopyPromptRectangle(
    PalConsoleState& state,
    const std::array<std::uint32_t, 4>& rectangle) {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        state.WriteDword(0x04u + 4u * index, rectangle[index]);
    }
    return rectangle[2];
}

std::uint32_t PalConsole::CopyBackgroundRectangle(
    PalConsoleState& state,
    const std::array<std::uint32_t, 4>& rectangle) {
    for (std::uint32_t index = 0u; index < 4u; ++index) {
        state.WriteDword(0x14u + 4u * index, rectangle[index]);
    }
    return rectangle[2];
}

std::uint32_t PalConsole::InitializeRectangles(
    PalConsoleState& state, const std::int32_t width,
    const std::int32_t height) {
    const auto half_height = height / 2;
    static_cast<void>(CopyBackgroundRectangle(
        state, {0u, 0u, static_cast<std::uint32_t>(width),
                static_cast<std::uint32_t>(half_height)}));
    const auto result = CopyPromptRectangle(
        state, {0u, static_cast<std::uint32_t>(half_height + 2),
                static_cast<std::uint32_t>(width),
                static_cast<std::uint32_t>(half_height + 20)});
    state.WriteDword(0x24u, 0xff20ee20u);
    state.WriteDword(0x28u, 0xffeeeeeeu);
    return result;
}

}  // namespace pal4
