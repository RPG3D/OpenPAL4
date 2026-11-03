#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeMagicWindowLayout;class NativeMagicWindowRecords;class NativeSkillRecordCatalog;class NativeGameDatabase;
struct PlayerManagerObjectState;
// Read-only presentation of live party collections through the original DLL.
// Layout, players, database and catalogs must outlive this borrowed adapter.
class NativeMagicWindowPresentation final {
public:
    NativeMagicWindowPresentation(NativeMagicWindowLayout&,PlayerManagerObjectState&,
        const NativeGameDatabase&,const NativeMagicWindowRecords&,const NativeSkillRecordCatalog&);
    ~NativeMagicWindowPresentation();
    NativeMagicWindowPresentation(const NativeMagicWindowPresentation&)=delete;
    NativeMagicWindowPresentation& operator=(const NativeMagicWindowPresentation&)=delete;
    std::size_t UpdateMagic();
    std::size_t UpdateStunts();
    void UpdateCharacter(std::int32_t index);
    void SetStuntPanelVisible(bool);
    void MagicDetails(std::int32_t row);
    void StuntDetails(std::int32_t row);
private:
    class Runtime;std::unique_ptr<Runtime> runtime_;
};
}
