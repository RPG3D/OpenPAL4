#include "pal4/portable/monster_scene_graphics.h"
#include <stdexcept>

namespace pal4::portable {
struct MonsterSceneGraphics::Impl {
    struct Item {
        SceneMonsterHandle handle;
        std::unique_ptr<AtomicDistanceInstance> distance;
        Scene mesh;
        std::vector<Matrix> frames;
        GlesRenderer gpu;
        ~Item(){gpu.Release();}
    };
    SceneMonsterInstances& actors;
    std::vector<std::unique_ptr<Item>> items;
    bool uploaded{};
    Impl(SceneMonsterInstances& a,DistanceOptimization& engine):actors(a) {
        items.reserve(actors.Size());
        for(std::size_t i=0;i<actors.Size();++i){
            auto item=std::make_unique<Item>();item->handle=actors.Handle(i);
            item->distance=std::make_unique<AtomicDistanceInstance>(actors.Model(item->handle),engine);
            items.push_back(std::move(item));
        }
    }
};
MonsterSceneGraphics::MonsterSceneGraphics(SceneMonsterInstances& actors,DistanceOptimization& engine)
    :impl_(std::make_unique<Impl>(actors,engine)){}
MonsterSceneGraphics::~MonsterSceneGraphics()=default;
bool MonsterSceneGraphics::Uploaded() const noexcept{return impl_->uploaded;}
void MonsterSceneGraphics::Release(){
    for(auto& item:impl_->items){item->gpu.Release();item->mesh={};item->frames.clear();}
    impl_->uploaded=false;
}
void MonsterSceneGraphics::Abandon(){
    for(auto& item:impl_->items){item->gpu.Abandon();item->mesh={};item->frames.clear();}
    impl_->uploaded=false;
}
void MonsterSceneGraphics::Upload(){
    Release();
    try{
        for(auto& item:impl_->items){
            item->mesh=impl_->actors.Model(item->handle).mesh;
            impl_->actors.Playback(item->handle).Sample(item->mesh.vertices,&item->frames);
            item->gpu.Upload(item->mesh);
        }
        impl_->uploaded=true;
    }catch(...){Release();throw;}
}
std::uint32_t MonsterSceneGraphics::Draw(SceneMonsterHandle handle,const Matrix& projection,std::array<float,3> camera){
    // Validate generation before indexing a GPU slot, including after reload.
    (void)impl_->actors.Model(handle);
    if(!impl_->uploaded)throw std::logic_error("monster graphics require upload in the current context");
    auto& item=*impl_->items.at(handle.index);
    impl_->actors.Playback(handle).Sample(item.mesh.vertices,&item.frames);
    item.gpu.UpdateVertices(item.mesh.vertices);
    const auto model=impl_->actors.InstanceTransform(handle);
    const auto appearance=item.distance->Evaluate(item.frames,model,camera);
    return item.gpu.DrawScene(item.mesh,projection,model,appearance);
}
}
