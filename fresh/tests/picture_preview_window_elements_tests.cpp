#include "pal4/picture_preview_window_elements.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Subscription {
    std::uint32_t window{};
    std::string event;
    std::uint32_t callback{};
};

struct Runtime final : pal4::PicturePreviewWindowElementsRuntimeStub {
    std::vector<std::string> names;
    std::vector<Subscription> subscriptions;
    std::vector<std::uint32_t> top;
    std::vector<std::uint32_t> deleted;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t next_callback{0xa000u};
    std::uint32_t fail_lookup_index{0xffffffffu};
    bool null_callbacks{};
    bool mask_hidden{};

    std::string ReadPrefix(std::uint32_t address) override {
        Check(address == 0x9000u, "layout prefix stored at +0x18");
        return "picturePreviewWindow/";
    }
    std::uint32_t FindWindow(std::string_view name) override {
        if (names.size() == fail_lookup_index)
            throw std::runtime_error("lookup");
        names.emplace_back(name);
        return 0x2000u + static_cast<std::uint32_t>(names.size()) * 0x100u;
    }
    void SetAlwaysOnTop(std::uint32_t window, bool enabled) override {
        Check(enabled, "every preview image is always on top");
        top.push_back(window);
    }
    std::uint32_t AllocateCallback(std::size_t bytes) override {
        Check(bytes == 0x18u, "event callback is 0x18 bytes");
        if (null_callbacks) return 0u;
        const auto value = next_callback;
        next_callback += 0x20u;
        return value;
    }
    void WriteCallbackWord(std::uint32_t address,
                           std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Subscribe(std::uint32_t window,
                            std::string_view event,
                            std::uint32_t callback) override {
        subscriptions.push_back({window, std::string(event), callback});
        return 0xc000u + static_cast<std::uint32_t>(subscriptions.size()) * 4u;
    }
    std::uint32_t ReferenceCount(std::uint32_t) override { return 1u; }
    void SetReferenceCount(std::uint32_t,
                           std::uint32_t count) override {
        Check(count == 0u, "local event owner reference decremented");
    }
    void DeleteReference(std::uint32_t reference,
                         std::uint32_t flags) override {
        Check(flags == 1u, "zero-ref event owner deleted");
        deleted.push_back(reference);
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(window == 0x2d00u && !visible,
              "select mask initially hidden");
        mask_hidden = true;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x27b0u,
              "first image field +0x6b0 supplies disabled image");
        return 0xd000u;
    }
};

void CheckElements() {
    pal4::PicturePreviewWindowState state{};
    state.Write(0x18u, 0x9000u);
    Runtime runtime;
    pal4::PicturePreviewWindowElements::Initialize(state, 0x6000u, runtime);
    Check(runtime.names.size() == 13u && runtime.top.size() == 10u &&
              runtime.subscriptions.size() == 21u &&
              runtime.deleted.size() == 21u && runtime.mask_hidden,
          "ten images and three auxiliary controls initialized");
    for (std::uint32_t i = 0u; i < 10u; ++i) {
        Check(runtime.names[i] ==
                  "picturePreviewWindow/Img" + std::to_string(i + 1u) &&
                  state.Read(0x2cu + i * 4u) == 0x2100u + i * 0x100u &&
                  runtime.top[i] == state.Read(0x2cu + i * 4u),
              "image names, slots and always-on-top order");
        const auto move = runtime.subscriptions[i * 2u];
        const auto click = runtime.subscriptions[i * 2u + 1u];
        Check(move.window == state.Read(0x2cu + i * 4u) &&
                  move.event == "MouseMove" && click.window == move.window &&
                  click.event == "MouseClick" &&
                  runtime.words.at(move.callback + 8u) == 0x44c4b0u &&
                  runtime.words.at(click.callback + 8u) == 0x44c4d0u,
              "image move and click subscriptions");
    }
    Check(runtime.names[10] == "picturePreviewWindow/StaticInfo" &&
              runtime.names[11] == "picturePreviewWindow/BtnClose" &&
              runtime.names[12] == "picturePreviewWindow/ImgSelectMask" &&
              state.Read(0x54u) == 0x2b00u &&
              state.Read(0x58u) == 0x2c00u &&
              state.Read(0x5cu) == 0x2d00u &&
              state.Read(0x60u) == 0xd000u,
          "auxiliary names, slots and disabled-image pointer");
    const auto close = runtime.subscriptions.back();
    Check(close.event == "Clicked" && close.window == state.Read(0x58u) &&
              runtime.words.at(close.callback + 8u) == 0x44a150u,
          "close button uses observed shared close handler");
    for (const auto& subscription : runtime.subscriptions) {
        const auto callback = subscription.callback;
        Check(runtime.words.at(callback) == 0x843778u &&
                  runtime.words.at(callback + 0xcu) == 0u &&
                  runtime.words.at(callback + 0x10u) == 0x6000u,
              "callback vtable, zero field and owner");
    }
}

void CheckNullAndFailure() {
    pal4::PicturePreviewWindowState state{};
    state.Write(0x18u, 0x9000u);
    Runtime runtime;
    runtime.null_callbacks = true;
    pal4::PicturePreviewWindowElements::Initialize(state, 0x6000u, runtime);
    Check(runtime.words.empty() && runtime.subscriptions.size() == 21u &&
              runtime.subscriptions.front().callback == 0u,
          "null allocation still subscribes empty callback owner");
    state = {};
    state.Write(0x18u, 0x9000u);
    runtime = Runtime{};
    runtime.fail_lookup_index = 1u;
    try {
        pal4::PicturePreviewWindowElements::Initialize(state, 0x6000u, runtime);
        Check(false, "lookup exception must propagate");
    } catch (const std::runtime_error&) {
        Check(runtime.names.size() == 1u &&
                  runtime.subscriptions.size() == 2u &&
                  state.Read(0x2cu) == 0x2100u &&
                  state.Read(0x30u) == 0u,
              "failed second lookup leaves first image initialized");
    }
}

}  // namespace

int main() {
    CheckElements();
    CheckNullAndFailure();
    std::cout << "Picture preview window element tests passed\n";
}
