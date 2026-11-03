#include "pal4/window_message_processor.h"
#include "pal4/comparison_flee_task_trace.h"

#include <bit>

namespace pal4 {
namespace {

constexpr std::uint32_t kDisplayFlags = 0x40000009u;

constexpr std::array<std::uint8_t, 256> MakeScanCodeMap() {
    std::array<std::uint8_t, 256> map{};
    map.fill(0xffu);
    constexpr std::array<std::uint8_t, 96> prefix{
        0xff, 0x80, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
        0x37, 0x38, 0x39, 0x30, 0x2d, 0x3d, 0xa8, 0xa9,
        0x71, 0x77, 0x65, 0x72, 0x74, 0x79, 0x75, 0x69,
        0x6f, 0x70, 0x5b, 0x5d, 0xab, 0xae, 0x61, 0x73,
        0x64, 0x66, 0x67, 0x68, 0x6a, 0x6b, 0x6c, 0x3b,
        0x27, 0x60, 0xac, 0x23, 0x7a, 0x78, 0x63, 0x76,
        0x62, 0x6e, 0x6d, 0x2c, 0x2e, 0x2f, 0xad, 0xa3,
        0xb0, 0x20, 0xaa, 0x81, 0x82, 0x83, 0x84, 0x85,
        0x86, 0x87, 0x88, 0x89, 0x8a, 0xa1, 0xff, 0x8f,
        0x9d, 0x9b, 0xa4, 0x9f, 0xa7, 0xa0, 0xa5, 0x9a,
        0x9e, 0x9c, 0x97, 0x98, 0xff, 0xff, 0x5c, 0x8b,
        0x8c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    };
    for (std::size_t i = 0; i < prefix.size(); ++i) map[i] = prefix[i];
    map[156] = 0xa6;
    map[157] = 0xaf;
    map[181] = 0xa2;
    map[184] = 0xb1;
    map[197] = 0xa1;
    map[199] = 0x8f;
    map[200] = 0x93;
    map[201] = 0x91;
    map[203] = 0x95;
    map[205] = 0x96;
    map[207] = 0x90;
    map[208] = 0x94;
    map[209] = 0x92;
    map[210] = 0x8d;
    map[211] = 0x8e;
    return map;
}

constexpr auto kScanCodeMap = MakeScanCodeMap();

std::int32_t Coordinate(const std::uint32_t value, const bool high) {
    return static_cast<std::int16_t>(high ? value >> 16u : value);
}

std::uint32_t FloatBits(const float value) {
    return std::bit_cast<std::uint32_t>(value);
}

}  // namespace

std::uint8_t WindowMessageProcessor::DecodeKeyboardScanCode(
    const std::uint32_t ignored, const std::uint32_t key_lparam) {
    (void)ignored;
    auto scan = static_cast<std::uint8_t>(key_lparam >> 16u);
    if ((key_lparam & 0x1000000u) != 0u) {
        scan = static_cast<std::uint8_t>(scan | 0x80u);
    }
    return scan;
}

std::uint32_t WindowMessageProcessor::MapScanCode(
    const std::uint8_t scan_code) {
    return kScanCodeMap[scan_code];
}

void WindowMessageProcessor::UpdateModifier(const std::uint32_t code,
                                             const std::uint32_t value) {
    if (code >= 172u && code <= 175u) {
        config_[27u + code - 172u] = value;
    }
}

void WindowMessageProcessor::ToggleMusic(IWindowMessageRuntime& runtime) {
    if (runtime.CurrentAudioActive()) {
        runtime.ReplayCurrentSound();
    }
    auto path = runtime.ComposeMusicBasePath();
    const bool enabled = runtime.MusicEnabled();
    runtime.SetMusicEnabled(!enabled);
    path += enabled ? "HSOff.mp3" : "HSOn.mp3";
    runtime.StoreMusicHandle(runtime.PlayMusic(path, 1u));
}

void WindowMessageProcessor::HandleKey(
    const std::uint32_t message, const std::uint32_t wparam,
    const std::uint32_t lparam, IWindowMessageRuntime& runtime) {
    if (message == 0x102u) {
        runtime.HandleKeyboardInput(0x18u, wparam);
        return;
    }
    const bool active = game_.Read(0x20u) != 0u;
    if (message == 0x104u) {
        if (active) {
            config_[31] = (lparam & 0x1du) != 0u ? 1u : 0u;
        }
        if ((lparam & 0x40000000u) != 0u) {
            return;
        }
    } else if (message == 0x105u && (lparam & 0x1du) != 0u) {
        config_[31] = 0u;
    }

    if ((message == 0x100u || message == 0x101u) && !active) {
        return;
    }
    const auto scan = DecodeKeyboardScanCode(wparam, lparam);
    const auto code = MapScanCode(scan);
    config_[25] = scan;
    config_[26] = code;
    if (message == 0x100u || message == 0x101u) {
        UpdateModifier(code, message == 0x100u ? 1u : 0u);
    }
    runtime.HandleKeyboardInput(message == 0x100u || message == 0x104u
                                    ? 0x16u : 0x17u,
                                config_.data() + 25u);
    if (message == 0x101u && (code == 86u || code == 118u)) {
        ToggleMusic(runtime);
    }
}

void WindowMessageProcessor::SetMouseSnapshot(const std::uint32_t lparam) {
    config_[12] = FloatBits(static_cast<float>(Coordinate(lparam, false)));
    config_[13] = FloatBits(static_cast<float>(Coordinate(lparam, true)));
    config_[14] = 0u;
    config_[15] = 0u;
    for (std::uint32_t i = 0; i < 5u; ++i) {
        config_[20u + i] = config_[27u + i];
    }
}

void WindowMessageProcessor::HandleMouse(
    const std::uint32_t window, const std::uint32_t message,
    const std::uint32_t wparam, const std::uint32_t lparam,
    IWindowMessageRuntime& runtime) {
    if (game_.Read(0x20u) == 0u) {
        return;
    }
    if (message == 0x200u) {
        const auto x = static_cast<float>(Coordinate(lparam, false));
        const auto y = static_cast<float>(Coordinate(lparam, true));
        if (game_.Read(0x2cu) != 0u) {
            config_[12] = FloatBits(x);
            config_[13] = FloatBits(y);
            config_[14] = FloatBits(x - std::bit_cast<float>(game_.Read(0x10u)));
            config_[15] = FloatBits(y - std::bit_cast<float>(game_.Read(0x14u)));
            for (std::uint32_t i = 0; i < 5u; ++i) {
                config_[20u + i] = config_[27u + i];
            }
            config_[17] = wparam & 1u;
            config_[18] = (wparam >> 1u) & 1u;
            config_[19] = (wparam >> 4u) & 1u;
            runtime.HandleMouseInput(0x0cu, config_.data() + 12u);
        } else {
            game_.Write(0x2cu, 1u);
        }
        game_.Write(0x10u, FloatBits(x));
        game_.Write(0x14u, FloatBits(y));
        return;
    }

    if (message == 0x202u || message == 0x205u ||
        message == 0x207u || message == 0x208u) {
        runtime.ReleaseCapture();
        if (message == 0x207u || message == 0x208u) {
            runtime.ReleaseCursorClip();
        }
    }
    SetMouseSnapshot(lparam);
    config_[17] = 0u;
    config_[18] = 0u;
    config_[19] = 0u;
    if (message == 0x201u || message == 0x203u) {
        config_[17] = 1u;
    } else if (message == 0x204u) {
        config_[18] = 1u;
    } else if (message == 0x207u) {
        config_[19] = 1u;
    } else if (message == 0x20au) {
        config_[17] = wparam & 1u;
        config_[18] = (wparam >> 1u) & 1u;
        config_[16] = (wparam & 0x80000000u) == 0u ? 1u : 0u;
    }
    if (message == 0x201u) {
        game_.Write(0x10u, config_[12]);
        game_.Write(0x14u, config_[13]);
    }
    if (message == 0x201u || message == 0x203u || message == 0x204u) {
        runtime.SetCapture(window);
    }
    std::uint32_t type = 0u;
    switch (message) {
    case 0x201u: type = 5u; break;
    case 0x202u: type = 6u; break;
    case 0x203u: type = 0x0bu; break;
    case 0x204u: type = 7u; break;
    case 0x205u: type = 8u; break;
    case 0x207u: type = 9u; break;
    case 0x208u: type = 0x0au; break;
    case 0x20au: type = 0x0du; break;
    default: break;
    }
    runtime.HandleMouseInput(type, config_.data() + 12u);
}

void WindowMessageProcessor::HandleActivation(
    const std::uint32_t window, const std::uint32_t wparam,
    IWindowMessageRuntime& runtime) {
    if ((wparam & 0xffffu) == 0u) {
        runtime.SetInputMode(0u, 0u, 0u);
        runtime.InitializeInputState();
        runtime.ReleaseCursorClip();
        if (!keep_running_when_inactive_) {
            game_.WriteShared(0x28u, 0u);
            runtime.ShowWindow(window, 2u);
            if (game_.Read(8u) != 0u)
                runtime.ChangeDisplayMode(prior_mode_, kDisplayFlags);
            game_.Write(0x20u, 0u);
        }
        runtime.SetAudioActive(false);
        return;
    }

    if (game_.Read(8u) != 0u) {
        DisplayModeRecord requested;
        requested.WriteWord(36u, 156u);
        requested.WriteDword(40u, 0x180000u);
        requested.WriteDword(108u, config_[0]);
        requested.WriteDword(112u, config_[1]);
        runtime.ChangeDisplayMode(requested, kDisplayFlags);
    }
    runtime.SetInputMode(1u, 1u, 0u);
    runtime.InitializeInputState();
    if (game_.Read(0x548u) != 0u) {
        runtime.ClipCursorToWindow(window);
    }
    game_.Write(0x20u, 1u);
    for (std::uint32_t i = 12u; i <= 31u; ++i) {
        config_[i] = 0u;
    }
    const auto tick = runtime.TimeGetTime();
    game_.Write(0x56cu, tick);
    game_.Write(0x574u, tick);
    game_.Write(0x570u, tick);
    runtime.ShowWindow(window, 9u);
    runtime.ShowWindow(window, 5u);
    runtime.SetForegroundWindow(window);
    runtime.SetFocus(window);
    runtime.SetAudioActive(true);
    runtime.Sleep(500u);
}

void WindowMessageProcessor::HandleDrop(
    const std::uint32_t drop_handle, IWindowMessageRuntime& runtime) {
    const auto saved = runtime.GetFilesystemCallback();
    const auto count = runtime.DroppedFileCount(drop_handle);
    for (auto remaining = count; remaining > 0u; --remaining) {
        const auto path = runtime.DroppedFile(drop_handle, remaining - 1u);
        std::uint32_t slashes = 0u;
        bool needs_uncd = false;
        for (const char character : path) {
            if (character == ':') {
                needs_uncd = true;
                break;
            }
            if (character == '\\' && ++slashes == 3u) {
                needs_uncd = true;
                break;
            }
        }
        if (!needs_uncd) {
            continue;
        }
        auto callback = runtime.CreateUncdFilesystemCallback(20u, "UNCD");
        if (callback != 0u) {
            runtime.RegisterFilesystemCallback(callback);
        } else {
            callback = runtime.FindFilesystemCallback("UNCD");
        }
        runtime.SetFilesystemCallback(callback);
    }
    runtime.SetFilesystemCallback(saved);
    runtime.FinishDrop(drop_handle);
}

std::uint32_t WindowMessageProcessor::Process(
    const std::uint32_t window, const std::uint32_t message,
    const std::uint32_t wparam, const std::uint32_t lparam,
    IWindowMessageRuntime& runtime) {
    const bool trace_message=message==0x200u || message==0x201u || message==0x202u ||
        ((message==0x100u || message==0x101u || message==0x102u) && (wparam=='W' || wparam=='w'));
    auto& trace=GetComparisonFleeTaskTrace();
    if(trace_message)trace.Mark(9u,message,wparam);
    struct MessageEnd {ComparisonFleeTaskTrace& trace;bool enabled;std::uint32_t message,key;
        ~MessageEnd(){if(enabled)trace.Mark(10u,message,key);}} message_end{trace,trace_message,message,wparam};
    handled_flag_ = 0u;
    switch (message) {
    case 1u:
        runtime.AcceptDroppedFiles(window);
        runtime.EnumCurrentDisplayMode(prior_mode_);
        handled_flag_ = 1u;
        break;
    case 2u:
    case 0x10u:
        if (game_.Read(8u) != 0u) {
            runtime.ChangeDisplayMode(prior_mode_, kDisplayFlags);
        }
        runtime.ReleaseCursorClip();
        config_[3] = (config_[3] & 0xffffff00u) | 1u;
        handled_flag_ = 1u;
        break;
    case 3u:
        if (game_.Read(0x548u) != 0u) {
            runtime.ClipCursorToWindow(window);
        }
        handled_flag_ = 1u;
        break;
    case 5u:
        if (game_.Read(0x548u) != 0u) {
            runtime.ClipCursorToWindow(window);
        }
        if (game_.Read(0x24u) != 0u &&
            (lparam & 0xffffu) != 0u && (lparam >> 16u) != 0u) {
            resize_marker_ = 0u;
        }
        handled_flag_ = 1u;
        break;
    case 6u:
        HandleActivation(window, wparam, runtime);
        handled_flag_ = 1u;
        break;
    case 0x20u:
        runtime.SetUICursorFromTable(0x8b1c70u);
        handled_flag_ = 1u;
        break;
    case 0x47u:
        if (game_.ReadShared(0x28u) != 0u) {
            const auto [keyboard_ready, mouse_ready] =
                runtime.InputDevicesReady();
            if ((!keyboard_ready || !mouse_ready) &&
                game_.Read(0x20u) != 0u) {
                runtime.SetInputMode(1u, 1u, 0u);
                runtime.InitializeInputState();
                runtime.ShowWindow(window, 5u);
                runtime.SetForegroundWindow(window);
                runtime.SetFocus(window);
            }
        }
        break;
    case 0x100u:
    case 0x101u:
    case 0x102u:
    case 0x104u:
    case 0x105u:
        HandleKey(message, wparam, lparam, runtime);
        handled_flag_ = 1u;
        break;
    case 0x113u:
        handled_flag_ = 1u;
        break;
    case 0x200u:
    case 0x201u:
    case 0x202u:
    case 0x203u:
    case 0x204u:
    case 0x205u:
    case 0x207u:
    case 0x208u:
    case 0x20au:
        HandleMouse(window, message, wparam, lparam, runtime);
        handled_flag_ = 1u;
        break;
    case 0x214u: {
        const auto rect = runtime.ReadSizingRect(lparam);
        const auto width = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(rect.right) -
            static_cast<std::uint32_t>(rect.left));
        const auto height = static_cast<std::int32_t>(
            static_cast<std::uint32_t>(rect.bottom) -
            static_cast<std::uint32_t>(rect.top));
        runtime.SetWindowPos(window, width, height, 2u);
        runtime.GetRenderSystem();
        runtime.ResizeRenderScreen(width, height, runtime.GetActiveCamera());
        handled_flag_ = 1u;
        break;
    }
    case 0x233u:
        HandleDrop(wparam, runtime);
        handled_flag_ = 1u;
        break;
    default:
        break;
    }

    if (!runtime.ConsoleActive() && game_.ReadShared(0x28u) != 0u) {
        runtime.HandleUIMessage(window, message, wparam, lparam);
    }
    if (handled_flag_ != 0u) {
        return 0u;
    }
    return runtime.DefWindowProc(window, message, wparam, lparam);
}

}  // namespace pal4
