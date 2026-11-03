#include "pal4/physical_attack_task.h"
#include <stdexcept>

namespace pal4 {
namespace {

constexpr std::uint32_t kEmptyText = 0x841964u;
constexpr std::uint32_t kTaskTextGlobal = 0x8f9680u;
constexpr std::uint32_t kSharedTextHeader = 0x8fa38cu;

std::uint32_t TextOrEmpty(IPhysicalAttackStartRuntime& runtime) {
    const auto text = runtime.ReadWord(kSharedTextHeader + 4u);
    return text == 0u ? kEmptyText : text;
}

}  // namespace

void IPhysicalAttackStartRuntime::AssignActionText(
    const std::uint32_t header, const std::uint32_t source_header) {
    if (source_header != kSharedTextHeader)
        throw std::invalid_argument("physical attack action global");
    const auto length = this->ReadWord(kSharedTextHeader + 8u);
    if (header == kSharedTextHeader) {
        this->EraseText(header, length, 0xffffffffu);
        this->EraseText(header, 0u, 0u);
    } else {
        bool share = false;
        if (length != 0u) {
            const auto text = TextOrEmpty(*this);
            const auto count = this->ReadByte(text - 1u);
            if (count < 0xfeu) {
                this->SetTextErrorFlag(header, 1u);
                const auto shared_text = TextOrEmpty(*this);
                this->WriteWord(header + 4u, shared_text);
                this->WriteWord(header + 8u, length);
                this->WriteWord(header + 12u,
                                  this->ReadWord(kSharedTextHeader + 12u));
                this->WriteByte(shared_text - 1u,
                                  static_cast<std::uint8_t>(count + 1u));
                share = true;
            }
        }
        if (!share && this->AllocateText(header, length, 1u)) {
            this->CopyText(header, TextOrEmpty(*this), length);
            this->WriteWord(header + 8u, length);
        }
    }
}

PhysicalAttackChildFactorySlot* PhysicalAttackChildFactory::Initialize(
    PhysicalAttackChildFactorySlot* const slot) {
    slot->vtable = kVtable;
    return slot;
}

std::uint8_t PhysicalAttackTask::Start(
    const std::uint32_t task, IPhysicalAttackStartRuntime& runtime) {
    const auto source_text = runtime.ReadWord(kTaskTextGlobal);
    constexpr std::array<std::uint32_t, 3> timing{1u, 0u, 0u};
    PhysicalAttackChildFactorySlot factory;
    (void)PhysicalAttackChildFactory::Initialize(&factory);
    const auto child = runtime.CreateChildTask(factory);
    if (child == 0u) return 0u;
    runtime.AddTaskAndInitialize(task + 4u, child,
                                 source_text == 0u ? kEmptyText : source_text,
                                 timing);

    const auto mode = runtime.ReadByte(task + 54u);
    const auto flag = runtime.ReadByte(task + 53u);
    const auto protector = runtime.ReadWord(task + 48u);
    const auto source = runtime.ReadWord(task + 44u);
    runtime.WriteWord(child + 60u, source);
    runtime.AssignActionText(child + 64u, kSharedTextHeader);
    runtime.WriteWord(child + 280u, protector);
    runtime.WriteWord(child + 276u, source);
    runtime.WriteByte(child + 288u, flag);
    runtime.WriteByte(child + 289u, mode);
    return flag;
}

}  // namespace pal4
