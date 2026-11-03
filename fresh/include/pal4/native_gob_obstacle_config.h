#pragma once
#include "pal4/gob_obstacle_config.h"
#include "pal4/native_gob_local_float_memory.h"
#include "pal4/native_gob_random_delay.h"
#include "pal4/native_scene_gob_catalog.h"
#include <cmath>
#include <cstring>
#include <functional>
#include <stdexcept>

namespace pal4 {
// Only reads the retained original XND. Package/model loading is owned by the
// scene's staged loader; it must not be initiated again by a config reset.
class NativeGobObstacleConfig final : public IGobObstacleConfigRuntime {
public:
    NativeGobObstacleConfig(NativeBaseObjectMemory& memory,const BinaryXmlNode& root,
        std::uint32_t object,std::function<void()> machine)
        :memory_(memory),root_(root),object_(object),machine_(std::move(machine)){}
    void LoadMachineConfig(std::uint32_t p) override{Require(p);machine_();}
    std::uint32_t ReadWord(std::uint32_t p) override{
        return p==object_+0x1c0u?reinterpret_cast<std::uint32_t>(&root_):memory_.ReadDword(p);
    }
    void WriteWord(std::uint32_t p,std::uint32_t v) override{memory_.WriteDword(p,v);}
    void WriteFloat(std::uint32_t p,float v) override{WriteWord(p,std::bit_cast<std::uint32_t>(v));}
    std::uint32_t FindXmlChild(std::uint32_t p,std::string_view name) override{
        if(!p)return 0u;
        for(const auto& n:reinterpret_cast<const BinaryXmlNode*>(p)->children)
            if(n.name==name)return reinterpret_cast<std::uint32_t>(&n);
        return 0u;
    }
    std::int32_t ReadConfigInt(std::uint32_t p,std::string_view key,std::int32_t fallback) override{
        const auto* a=Node(p).FindAttribute(key);return a?std::bit_cast<std::int32_t>(a->ToInt(static_cast<std::uint32_t>(fallback))):fallback;
    }
    float ReadConfigFloat(std::uint32_t p,std::string_view key,float fallback) override{
        const auto* a=Node(p).FindAttribute(key);return a?a->ToFloat(fallback):fallback;
    }
    std::string ReadConfigString(std::uint32_t p,std::string_view key,std::string_view fallback) override{
        const auto* a=Node(p).FindAttribute(key);const auto* s=a?std::get_if<std::string>(&a->value):nullptr;return s?*s:std::string(fallback);
    }
    void AssignStringRecord(std::uint32_t p,std::string_view value) override{
        if(p!=object_+0x308u && p!=object_+0x320u)throw std::invalid_argument("Obstacle launch string");
        const auto old=ReadWord(p);
        const auto n=static_cast<std::uint32_t>(value.size());
        if(!n){if(old)(void)memory_.Free(old);for(auto o:{0u,4u,8u})WriteWord(p+o,0u);return;}
        const auto data=memory_.Allocate(n+1u);
        std::memcpy(reinterpret_cast<void*>(data),value.data(),n);memory_.WriteByte(data+n,0u);
        if(old)(void)memory_.Free(old);WriteWord(p,data);WriteWord(p+4u,n);WriteWord(p+8u,n+1u);
    }
    std::uint32_t TimeMilliseconds() override{return timeGetTime();}
    float RandomDelay(float a,float b,float seed) override{return NativeGobRandomDelay(a,b,seed);}
private:
    void Require(std::uint32_t p){if(p!=object_)throw std::invalid_argument("Obstacle config owner");}
    const BinaryXmlNode& Node(std::uint32_t p){if(!p)throw std::invalid_argument("Obstacle config node");return *reinterpret_cast<const BinaryXmlNode*>(p);}
    [[noreturn]] static void LoadBoundary(){throw std::logic_error("Obstacle config reset cannot initiate a scene load");}
    void CleanupBeforeSceneLoad(std::uint32_t) override{LoadBoundary();}
    bool UsesObjectPackage(std::string_view) override{LoadBoundary();}
    bool UsesWeaponPackage(std::string_view) override{LoadBoundary();}
    void LoadObjectPackage() override{LoadBoundary();}
    void LoadWeaponPackage() override{LoadBoundary();}
    std::uint32_t CurrentScenePackage() override{LoadBoundary();}
    void LoadScenePackage(std::uint32_t) override{LoadBoundary();}
    std::uint32_t InitializeMachineScene(std::uint32_t,std::uint32_t,std::string_view,std::uint32_t,std::uint32_t) override{LoadBoundary();}
    void CleanupScriptPackages() override{LoadBoundary();}
    void AttachObstacleHelper(std::uint32_t,std::uint32_t) override{LoadBoundary();}
    void RemoveXmlChild(std::uint32_t,std::uint32_t) override{LoadBoundary();}
    std::uint32_t CreateXmlChild(std::uint32_t,std::string_view) override{LoadBoundary();}
    NativeBaseObjectMemory& memory_;
    const BinaryXmlNode& root_;
    std::uint32_t object_;
    std::function<void()> machine_;
};
}
