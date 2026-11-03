#pragma once
#include "pal4/native_config_composite_collision_runtime.h"
#include <stdexcept>
#include <unordered_map>

namespace pal4 {
// Primary collision properties contain identities from more than one creating
// system. Route only explicit registrations; fallback keeps the EVF owner's
// existing generation/clump checks. No guessed vtable dispatch is performed.
class NativeCollisionEventRouter final : public INativeCollisionEventCallbacks {
public:
    void BindFallback(INativeCollisionEventCallbacks* fallback){
        if(fallback==this)throw std::invalid_argument("collision callback router cycle");
        fallback_=fallback;
    }
    void Register(std::uint32_t identity,INativeCollisionEventCallbacks& owner){
        if(!identity || &owner==this)throw std::invalid_argument("collision callback owner");
        const auto [it,inserted]=owners_.emplace(identity,&owner);
        if(!inserted)throw std::invalid_argument("collision callback identity already owned");
    }
    void Unregister(std::uint32_t identity,INativeCollisionEventCallbacks& owner){
        const auto it=owners_.find(identity);
        if(it==owners_.end() || it->second!=&owner)
            throw std::invalid_argument("collision callback unregister owner");
        owners_.erase(it);
    }
    std::size_t Count()const noexcept{return owners_.size();}
    INativeCollisionEventCallbacks* Fallback()const noexcept{return fallback_;}
    void Enter(std::uint32_t identity,std::uint32_t previous) override{
        Target(identity).Enter(identity,previous);
    }
    void Leave(std::uint32_t identity) override{Target(identity).Leave(identity);}
private:
    INativeCollisionEventCallbacks& Target(std::uint32_t identity){
        const auto it=owners_.find(identity);
        if(it!=owners_.end())return *it->second;
        if(fallback_)return *fallback_;
        throw std::invalid_argument("collision callback identity has no live owner");
    }
    std::unordered_map<std::uint32_t,INativeCollisionEventCallbacks*> owners_;
    INativeCollisionEventCallbacks* fallback_{};
};
}
