#pragma once
#include "pal4/native_camera_named_collection_cleanup.h"
#include "pal4/native_camera_manager_owner.h"
#include <rpworld.h>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <source_location>
#include <string>

namespace pal4::test {
class NativeNamedCameraCleanupProbe final : public INativeNamedCameraObjects {
    NativeCameraManagerOwner& manager_;
    INativeCameraObjectResources& resources_;
    RpWorld* world_;
    RpClump* observer_{};
    NativeBaseObjectMemory memory_;
    NativeCameraNamedCollectionCleanup cleanup_;
    std::array<std::uint32_t,4> fixed_{};
    std::vector<std::uint32_t> destroyed_;
    unsigned initial_cameras_{};
    unsigned CountCameras() const {
        return static_cast<unsigned>(RpClumpGetNumCameras(observer_));
    }
    static void Check(bool value,std::source_location location=std::source_location::current()) {
        if (!value) throw std::runtime_error("native named-camera cleanup probe line "+
            std::to_string(location.line()));
    }
public:
    NativeNamedCameraCleanupProbe(NativeCameraManagerOwner& manager,
        INativeCameraObjectResources& resources,RpWorld* world)
        :manager_(manager),resources_(resources),world_(world),
          cleanup_(memory_,manager.Address()+16u,*this) {
        Check(world && !manager.State().At(32u));
        observer_=RpClumpCreate();Check(observer_!=nullptr);
        initial_cameras_=CountCameras();
        for (unsigned i=0;i<4;++i) fixed_[i]=manager.State().At(52u+4u*i);
        manager.BindNamedCameraCleanup(&cleanup_);
        std::array<std::uint32_t,3> nodes{};
        for (unsigned i=0;i<3;++i) {
            const auto object=memory_.Allocate(sizeof(CameraSystemObjectState));
            const auto data=memory_.Allocate(sizeof(CameraDataState));
            auto& owner=*reinterpret_cast<CameraSystemObjectState*>(object);
            auto& camera=*reinterpret_cast<CameraDataState*>(data);
            owner={};camera={};owner.camera=data;
            CameraSetupParameters parameters;
            parameters.name="NamedCamera";
            parameters.viewport={0,0,64,64};parameters.depth_raster=true;
            parameters.distance=20.0f;
            Check(resources.Initialize(owner,camera,parameters));
            Check(RpWorldAddCamera(world,reinterpret_cast<RwCamera*>(camera.At(0u)))!=nullptr);
            Check(RpClumpAddCamera(observer_,reinterpret_cast<RwCamera*>(camera.At(0u)))!=nullptr);
            // The resource boundary borrows its setup name; this fixture creates
            // the original object's separate owned StringBuffer before cleanup.
            owner.name={};
            (void)StringBuffer::InitializeEmptyAt(object+4u,memory_);
            std::memcpy(reinterpret_cast<void*>(owner.name.data),"NamedCamera",12u);
            owner.name.length=11u;
            const auto key=memory_.Allocate(2u);
            memory_.WriteByte(key,static_cast<std::uint8_t>('B'+(i==1?-1:i==2?1:0)));
            memory_.WriteByte(key+1u,0u);
            nodes[i]=memory_.Allocate(28u);
            std::memset(reinterpret_cast<void*>(nodes[i]),0,28u);
            memory_.WriteDword(nodes[i]+12u,key);
            memory_.WriteDword(nodes[i]+16u,object);
            memory_.WriteDword(nodes[i]+24u,manager.Address()+16u);
        }
        memory_.WriteDword(nodes[0],nodes[1]);
        memory_.WriteDword(nodes[0]+4u,nodes[2]);
        memory_.WriteDword(nodes[1]+8u,nodes[0]);
        memory_.WriteDword(nodes[2]+8u,nodes[0]);
        manager.State().At(32u)=nodes[0];manager.State().At(36u)=3u;
        Check(CountCameras()==initial_cameras_+3u);
    }
    ~NativeNamedCameraCleanupProbe() override {
        if (manager_.State().At(32u)) manager_.ResetSceneProperties();
        manager_.BindNamedCameraCleanup(nullptr);
        if (observer_) (void)RpClumpDestroy(observer_);
    }
    void DestroyNamedCamera(std::uint32_t object) override {
        auto& owner=*reinterpret_cast<CameraSystemObjectState*>(object);
        const auto data=owner.camera;
        auto& camera=*reinterpret_cast<CameraDataState*>(data);
        Check(RwCameraGetWorld(reinterpret_cast<RwCamera*>(camera.At(0u)))==world_);
        Check(!owner.tasks[1] && !owner.tasks[2]); // no foreign task owner in this fixture
        // The observer clump is an additional test owner. SDK camera destruction
        // does not retire this external membership; its creator must detach it.
        Check(RpClumpRemoveCamera(observer_,reinterpret_cast<RwCamera*>(camera.At(0u)))!=nullptr);
        auto resource_owner=owner;
        resources_.Destroy(resource_owner,camera);
        Check(!camera.At(0u));
        (void)memory_.Free(data);
        (void)StringBuffer::CleanupAt(object+4u,memory_);
        (void)memory_.Free(object);
        destroyed_.push_back(object);
    }
    void VerifyReleased(bool manager_destroyed=false) {
        Check(!manager_.State().At(32u) && !manager_.State().At(36u));
        Check(destroyed_.size()==3u);
        Check(!memory_.OutstandingAllocations());
        Check(CountCameras()==initial_cameras_);
        if (!manager_destroyed) {
            Check(manager_.State().At(48u)==fixed_[0]);
            for (unsigned i=0;i<4;++i) Check(manager_.State().At(52u+4u*i)==fixed_[i]);
            manager_.ResetSceneProperties();
            Check(destroyed_.size()==3u);
        }
        manager_.BindNamedCameraCleanup(nullptr);
    }
};
}
