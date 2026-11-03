#include "pal4/script_random_talk.h"

#include <stdexcept>

namespace pal4 {

std::uint32_t ScriptRandomTalkServiceStub::ScriptExecutor() { return 0u; }
ScenarioDialogWaitState* ScriptRandomTalkServiceStub::AllocateHandler(
    std::uint32_t) { return nullptr; }
void ScriptRandomTalkServiceStub::ConstructBase(
    ScenarioDialogWaitState&, std::uint32_t, std::uint32_t) {}
void ScriptRandomTalkServiceStub::Flush(ScenarioDialogWaitState&) {}
void ScriptRandomTalkServiceStub::AddHandler(
    std::uint32_t, ScenarioDialogWaitState*) {}
std::int32_t ScriptRandomTalkServiceStub::Random() { return 0; }
std::uint32_t ScriptRandomTalkServiceStub::UiFrameManager() { return 0u; }
std::uint32_t ScriptRandomTalkServiceStub::CreateScenarioDialog(
    std::uint32_t) { return 0u; }
std::uint8_t ScriptRandomTalkServiceStub::IsVisible(std::uint32_t) {
    return 0u;
}
std::uint8_t ScriptRandomTalkServiceStub::SetTextRecord(
    std::uint32_t, std::uint32_t, std::uint32_t) { return 0u; }

std::uint32_t ScriptRandomTalk::Push(std::uint32_t source_record) {
    const auto array = service_.ScriptExecutor() + kArrayOffset;
    const auto begin = records_.ReadWord(array + 4u);
    const auto end = records_.ReadWord(array + 8u);
    const auto capacity = records_.ReadWord(array + 12u);
    static_cast<void>(SelectDialogOptionArray::Insert(
        array, end, source_record, records_));
    return static_cast<std::int64_t>(capacity) - end >= kRecordSize
               ? end + kRecordSize : begin;
}

std::uint8_t ScriptRandomTalk::Show() {
    class LegacyPool final : public IScriptRandomTalkPoolView {
    public:
        LegacyPool(ISelectDialogOptionArrayRuntime& records, IScriptRandomTalkService& service)
            : records_(records), service_(service) {}
        std::uint32_t Count() override {
            const auto executor = service_.ScriptExecutor();
            const auto begin = records_.ReadWord(executor + kArrayOffset + 4u);
            const auto end = begin == 0u ? begin :
                records_.ReadWord(executor + kArrayOffset + 8u);
            return (end - begin) / kRecordSize;
        }
        std::uint32_t Record(std::uint32_t index) override {
            return records_.ReadWord(service_.ScriptExecutor() + kArrayOffset + 4u) +
                kRecordSize * index;
        }
    private:
        ISelectDialogOptionArrayRuntime& records_;
        IScriptRandomTalkService& service_;
    } pool(records_, service_);
    return Show(pool, service_);
}

std::uint32_t ScriptRandomTalk::Release() {
    const auto array = service_.ScriptExecutor() + kArrayOffset;
    const auto begin = records_.ReadWord(array + 4u);
    const auto end = records_.ReadWord(array + 8u);
    SelectDialogOptionRecords::Cleanup(begin, end, records_);
    records_.WriteWord(array + 8u, begin);
    return end;
}

}  // namespace pal4
