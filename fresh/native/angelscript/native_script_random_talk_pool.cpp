#include "pal4/native_crt_random.h"
#include "pal4/native_script_random_talk_pool.h"
#include <cstdlib>
#include <limits>
#include <stdexcept>

static_assert(sizeof(void*) == sizeof(std::uint32_t));
namespace pal4 {
namespace {
class DialogService final : public IScriptRandomTalkService {
public:
    explicit DialogService(IScriptTalkHandlerRuntime& talk) : talk_(talk) {}
    std::uint32_t ScriptExecutor() override { return talk_.ScriptExecutor(); }
    ScenarioDialogWaitState* AllocateHandler(std::uint32_t size) override { return talk_.Allocate(size); }
    void ConstructBase(ScenarioDialogWaitState& h, std::uint32_t mode, std::uint32_t queue) override {
        talk_.ConstructBase(h,mode,queue);
    }
    void Flush(ScenarioDialogWaitState& h) override { talk_.Flush(h); }
    void AddHandler(std::uint32_t queue, ScenarioDialogWaitState* h) override {
        (void)talk_.AddHandler(queue,h);
    }
    std::int32_t Random() override { return pal4::NativeCrtRandom(__FILE__); }
    std::uint32_t UiFrameManager() override { return talk_.UiFrameManager(); }
    std::uint32_t CreateScenarioDialog(std::uint32_t manager) override { return talk_.CreateScenarioDialog(manager); }
    std::uint8_t IsVisible(std::uint32_t dialog) override { return talk_.IsVisible(dialog); }
    std::uint8_t SetTextRecord(std::uint32_t dialog, std::uint32_t record, std::uint32_t display) override {
        const auto& words = *reinterpret_cast<const std::array<std::uint32_t,4>*>(record);
        return talk_.SetText(dialog, {reinterpret_cast<const char*>(words[1]),words[2]},display);
    }
private:
    IScriptTalkHandlerRuntime& talk_;
};
}
void NativeScriptRandomTalkPool::Push(std::string_view text) {
    if (texts_.size() >= static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) ||
        text.size() > std::numeric_limits<std::uint32_t>::max())
        throw std::length_error("PAL4 random dialogue pool exceeds original range");
    texts_.emplace_back(text);
    selected_ = {};
}
void NativeScriptRandomTalkPool::Clear() noexcept {
    selected_ = {};
    texts_.clear();
}
std::uint32_t NativeScriptRandomTalkPool::Count() { return static_cast<std::uint32_t>(texts_.size()); }
std::uint32_t NativeScriptRandomTalkPool::Record(std::uint32_t index) {
    const auto& text = texts_.at(index);
    selected_ = {0u,reinterpret_cast<std::uint32_t>(text.c_str()),
        static_cast<std::uint32_t>(text.size()),static_cast<std::uint32_t>(text.size())};
    return reinterpret_cast<std::uint32_t>(selected_.data());
}
std::uint8_t NativeScriptRandomTalkPool::Show(IScriptTalkHandlerRuntime& talk) {
    DialogService service(talk);
    return ScriptRandomTalk::Show(*this,service);
}
} // namespace pal4
