#pragma once
#include "pal4/script_prescription_addition.h"
#include "pal4/native_inventory_prescriptions.h"
#include <array>
#include <map>
#include <string>
namespace pal4 {
class NativeGameDatabase;
class IScriptInventoryChangeRuntime;
// Reuses the existing inventory hint/audio endpoints and shared script handler.
class NativeScriptPrescriptionAddition final : public IScriptPrescriptionAdditionRuntime {
public:
    NativeScriptPrescriptionAddition(NativeInventorySession&,const NativePrescriptionRecordCatalog&,const NativeGameDatabase&,IScriptInventoryChangeRuntime&);
    std::size_t OutstandingTexts()const noexcept{return texts_.size();}
    std::uint32_t FrameManager()override;
    std::uint32_t CreateHintWindow(std::uint32_t)override;
    std::uint32_t GameDatabase()override;
    bool HasRecord(std::uint32_t,std::int32_t)override;
    std::uint32_t FindRecord(std::uint32_t,std::int32_t)override;
    std::uint32_t Inventory()override;
    void AddPrescription(std::uint32_t,std::int32_t)override;
    std::uint32_t InitializeText()override;
    std::uint32_t ReadWord(std::uint32_t)override;
    std::uint32_t EmptyText()override;
    std::uint32_t FindGameString(std::uint32_t,std::uint32_t)override;
    void FormatText(std::uint32_t,std::uint32_t,std::uint32_t)override;
    void AppendText(std::uint32_t,std::uint32_t)override;
    void ShowHint(std::uint32_t,std::uint32_t,float,float,float,std::uint32_t,std::uint32_t)override;
    std::uint32_t AudioManager()override;
    void PlayMusic(std::uint32_t,std::uint8_t,std::string_view,std::uint32_t)override;
    std::uint8_t CleanupText(std::uint32_t)override;
private:
    NativeInventorySession& inventory_;const NativePrescriptionRecordCatalog& recipes_;const NativeGameDatabase& database_;
    IScriptInventoryChangeRuntime& hints_;NativeInventoryPrescriptions learned_;
    std::map<std::uint32_t,std::string> texts_;std::uint32_t next_text_{1u};std::array<std::uint32_t,2> format_{};
};
}
