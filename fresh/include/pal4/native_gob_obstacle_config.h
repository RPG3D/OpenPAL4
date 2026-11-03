#pragma once
#include "pal4/gob_obstacle_config.h"
#include "pal4/native_gob_local_float_memory.h"
#include "pal4/native_gob_random_delay.h"
#include "pal4/native_scene_gob_catalog.h"
#include "pal4/native_event_document_runtime.h"
#include <cmath>
#include <cstring>
#include <functional>
#include <stdexcept>

namespace pal4 {
// Only reads the retained original XND. Package/model loading is owned by the
// scene's staged loader; it must not be initiated again by a config reset.
class NativeGobObstacleConfig final : public IGobObstacleConfigRuntime {
public:
    NativeGobObstacleConfig(NativeBaseObjectMemory& memory,NativeEventDocumentRuntime& documents,
        std::uint32_t object,std::function<void()> machine)
        :memory_(memory),documents_(documents),object_(object),machine_(std::move(machine)){}
    void LoadMachineConfig(std::uint32_t p) override{Require(p);machine_();}
    std::uint32_t ReadWord(std::uint32_t p) override{
        return memory_.ReadDword(p);
    }
    void WriteWord(std::uint32_t p,std::uint32_t v) override{memory_.WriteDword(p,v);}
    void WriteFloat(std::uint32_t p,float v) override{WriteWord(p,std::bit_cast<std::uint32_t>(v));}
    std::uint32_t FindXmlChild(std::uint32_t p,std::string_view name) override{
        return documents_.FindChild(p,name);
    }
    std::int32_t ReadConfigInt(std::uint32_t p,std::string_view key,std::int32_t fallback) override{
        const auto a=documents_.FindNumericAttribute(p,key);return a?documents_.AttributeToInt(a,fallback):fallback;
    }
    float ReadConfigFloat(std::uint32_t p,std::string_view key,float fallback) override{
        const auto a=documents_.FindNumericAttribute(p,key);return a?documents_.AttributeToFloat(a,fallback):fallback;
    }
    std::string ReadConfigString(std::uint32_t p,std::string_view key,std::string_view fallback) override{
        const auto a=documents_.FindNumericAttribute(p,key);return a?documents_.AttributeToString(a):std::string(fallback);
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
    NativeEventDocumentRuntime& documents_;
    std::uint32_t object_;
    std::function<void()> machine_;
};
}
