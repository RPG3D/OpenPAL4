#pragma once
#include "pal4/track_task.h"
#include <memory>
namespace pal4 {
class NativeBaseObjectMemory;
class NativeEventDocumentRuntime;
class NativeScriptObjectRuntime;
class INativeEffectTaskFacets;
// Reuses TrackTask; owns point arrays in modern STL and calls public RpSpline.
class NativeTrackTaskRuntime final : public ITrackTaskRuntime, public ITrackTaskConfigRuntime {
public:
    NativeTrackTaskRuntime(NativeBaseObjectMemory&,NativeEventDocumentRuntime&,NativeScriptObjectRuntime&,INativeEffectTaskFacets&);
    ~NativeTrackTaskRuntime();
    TrackPointArray& Points(TrackTaskState&);
    void ConstructBase(TrackTaskState&) override;
    void DestroyBase(TrackTaskState&) override;
    void FreeNativePointBuffer(std::uint32_t) override;
    std::uint8_t ConstructorStackByte() override;
    void TransformPoints(std::uint32_t,TrackPointArray&) override;
    TrackPoint Normalize(const TrackPoint&) override;
    std::uint32_t ResolveFacet(std::uint32_t,std::string_view) override;
    void SetPosition(std::uint32_t,const TrackPoint&) override;
    void FacePoint(std::uint32_t,const TrackPointArray&,std::size_t) override;
    std::vector<std::uint32_t> Children(std::uint32_t,std::string_view) override;
    std::int32_t ReadInt(std::uint32_t,std::string_view,std::int32_t) override;
    std::int32_t ReadTreeInt(std::uint32_t,std::string_view,std::int32_t) override;
    float ReadFloat(std::uint32_t,std::string_view,float) override;
    std::uint32_t CreateSpline(const TrackPointArray&,std::int32_t) override;
    TrackPoint SampleSpline(std::uint32_t,std::int32_t,float) override;
    void DestroySpline(std::uint32_t) override;
    void SynchronizePointStorage(TrackTaskState&,const TrackPointArray&) override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
