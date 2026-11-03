#pragma once
#include "pal4/rump_shahdow_task.h"
#include <memory>
namespace pal4 {
class NativeBaseObjectMemory;
class NativeScriptObjectRuntime;
class INativeEffectTaskFacets;
// Uses the existing task algorithm; host vector owns only the record-pointer
// collection, while accounting memory owns each original twelve-byte record.
class NativeRumpShadowTaskRuntime final : public IRumpShahdowTaskRuntime {
public:
    NativeRumpShadowTaskRuntime(NativeBaseObjectMemory&,NativeScriptObjectRuntime&,INativeEffectTaskFacets&);
    ~NativeRumpShadowTaskRuntime();
    void ConstructBase(RumpShahdowTaskState&) override;
    void DestroyBase(RumpShahdowTaskState&) override;
    void FreeCollectionStorage(RumpShahdowTaskState&) override;
    std::uint32_t ResolveFacet(std::uint32_t,std::string_view) override;
    void PrepareOwner(std::uint32_t) override;
    RumpShahdowColor ReadColor(std::uint32_t) override;
    void WriteColor(std::uint32_t,const RumpShahdowColor&) override;
    bool ValidateCount(std::uint32_t,std::int32_t) override;
    std::uint32_t CreateChild(std::uint32_t,std::int32_t) override;
    std::uint32_t AllocateRecord(std::uint32_t) override;
    void FreeRecord(std::uint32_t) override;
    RumpShahdowRecord ReadRecord(std::uint32_t) override;
    void WriteRecord(std::uint32_t,const RumpShahdowRecord&) override;
    void AppendRecord(RumpShahdowTaskState&,std::uint32_t) override;
    std::uint32_t RecordCount(const RumpShahdowTaskState&) override;
    std::uint32_t RecordAt(const RumpShahdowTaskState&,std::uint32_t) override;
    std::uint32_t MoveLastRecordToFront(RumpShahdowTaskState&) override;
    void ClearRecords(RumpShahdowTaskState&) override;
    void SetShowFacet(std::uint32_t,bool) override;
    void DispatchChildEvent(std::uint32_t,bool) override;
    RumpShahdowTransform ReadOwnerTransform(std::uint32_t) override;
    std::uint32_t ChildFrame(std::uint32_t) override;
    bool FrameHasParent(std::uint32_t) override;
    void RemoveFrameChild(std::uint32_t) override;
    void FrameSetIdentity(std::uint32_t) override;
    void FrameApplyTransform(std::uint32_t,const RumpShahdowTransform&) override;
    void FrameUpdateObjects(std::uint32_t) override;
    std::int32_t FloatToInt(double) override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
