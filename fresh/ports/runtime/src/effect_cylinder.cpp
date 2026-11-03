#include "pal4/portable/effect_cylinder.h"
#include "pal4/portable/effect_transform.h"
#include "pal4/portable/world.h"
#include "pal4/cylinder_object_config.h"
#include "pal4/cylinder_object_transform.h"
#include "pal4/cylinder_object_color.h"
#include "pal4/cylinder_object_visibility.h"
#include "pal4/cylinder_object_update.h"
#include "pal4/effect_system_object_f_facet.h"
#include "pal4/effect_base_access.h"
#include "pal4/visual_model.h"
#include "pal4/xml_float_to_int.h"
#include <bit>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <limits>
#include <stdexcept>
namespace pal4::portable {
namespace {
void Require(bool v,const char* message){if(!v)throw std::logic_error(message);}
void Finite(const std::array<float,3>& v){for(float f:v)Require(std::isfinite(f),"invalid cylinder transform");}
pal4::VisualModelVector Vector(const std::array<float,3>& v){return {v[0],v[1],v[2]};}
std::array<float,3> Vector(pal4::VisualModelVector v){return {v.x,v.y,v.z};}
}
struct EffectCylinder::Impl final:pal4::ICylinderObjectRuntime,pal4::ICylinderObjectConfigRuntime,pal4::ICylinderObjectTransformRuntime,
    pal4::ICylinderObjectColorRuntime,pal4::ICylinderObjectVisibilityRuntime,pal4::ICylinderObjectPlaybackRuntime,
    pal4::ICylinderObjectUpdateRuntime,pal4::IVisualModelRuntime,pal4::IVisualModelDestroyRuntime {
    EffectObjectCounts& counts;EffectTextures& textures;BinaryXmlNode document;pal4::CylinderObjectState state;
    pal4::VisualModelState model;std::optional<CylinderMesh> mesh;std::shared_ptr<const EffectTexture> texture;
    bool allocated{},frame_alive{},registered{};std::array<float,3> pending_position{},pending_rotation{},pending_scale{1,1,1};
    std::array<float,16> matrix=InstanceMatrix({0,0,0},0);EffectMatrix parent=IdentityEffectMatrix();std::string texture_name;
    explicit Impl(EffectObjectCounts& c,EffectTextures& t):counts(c),textures(t){pal4::CylinderObject::Construct(state,0,*this);registered=true;++counts.created;}
    ~Impl(){Clear();if(registered)pal4::CylinderObject::Destroy(state,*this);}
    void Clear(){if(allocated){DestroyModel(0x1000);FreeModel(0x1000);}texture.reset();state.words[22]=state.words[23]=0;}
    void CheckModel(unsigned m) const{Require(m==0x1000 && allocated,"invalid cylinder model owner");}
    unsigned SystemInstance() override{return 0x5000;}
    unsigned ReadWord(unsigned address) override{
        if(address==0x50a4)return counts.cylinders;
        Require(allocated && address>=0x1000 && address-0x1000+4<=sizeof(model) && address%4==0,"invalid cylinder model field");
        unsigned value;std::memcpy(&value,reinterpret_cast<const std::byte*>(&model)+(address-0x1000),4);return value;
    }
    std::uint8_t ReadByte(unsigned address) override{return static_cast<std::uint8_t>(ReadWord(address&~3u)>>(8*(address&3)));}
    void WriteWord(unsigned address,unsigned value) override{Require(address==0x50a4,"invalid cylinder instance counter");counts.cylinders=value;}
    void FreeBuffer(unsigned address) override{Require(address==0,"cylinder auxiliary buffer is not bound");}
    void FreeObject(pal4::CylinderObjectState&) override{throw std::logic_error("cylinder owner cannot delete itself");}
    unsigned ForwardAction(pal4::CylinderObjectState&,const std::array<unsigned,3>&,unsigned) override{throw std::logic_error("cylinder forward-action facet is not bound");}
    const BinaryXmlAttribute* Attribute(unsigned d,std::string_view key){Require(d==1,"invalid cylinder config handle");return document.FindAttribute(key);}
    std::string_view ReadTextureName(unsigned d,std::string_view key,unsigned fallback) override{Require(fallback==0x8e425c,"invalid cylinder texture default");const auto* a=Attribute(d,key);texture_name.clear();if(a)if(const auto* s=std::get_if<std::string>(&a->value))texture_name=*s;return texture_name;}
    int ReadInt(unsigned d,std::string_view key,int fallback) override{const auto* a=Attribute(d,key);if(!a)return fallback;if(const auto* f=std::get_if<float>(&a->value))return pal4::XmlFloatToInt(std::bit_cast<unsigned>(*f));Require(std::holds_alternative<std::int32_t>(a->value) || std::holds_alternative<std::int8_t>(a->value),"cylinder XML text-number conversion is not bound");return std::bit_cast<int>(a->ToInt(std::bit_cast<unsigned>(fallback)));}
    float ReadFloat(unsigned d,std::string_view key,float fallback) override{const auto* a=Attribute(d,key);if(!a)return fallback;Require(std::holds_alternative<float>(a->value)||std::holds_alternative<std::int32_t>(a->value)||std::holds_alternative<std::int8_t>(a->value),"cylinder XML text-number conversion is not bound");return a->ToFloat(fallback);}
    unsigned FindTexture(std::string_view path) override{Require(!texture,"cylinder texture already owned");texture=textures.Load(path);return 1;}
    unsigned LoadTexture(std::string_view path) override{return FindTexture(path);}
    unsigned AllocateModel(unsigned bytes) override{Require(!allocated && bytes==sizeof(model),"invalid cylinder model allocation");allocated=true;return 0x1000;}
    void InitializeModel(unsigned m) override{CheckModel(m);model={};}
    bool CreateVisualModel(unsigned m,unsigned t,const pal4::CylinderVisualParameters& p) override{
        CheckModel(m);Require(t==1 && texture,"cylinder texture is missing");
        CylinderShape shape{p.border_count,p.angle,p.height,p.top_radius,p.bottom_radius,p.shrink_size,p.rasterize,p.flip_uv,p.center};
        mesh=BuildCylinder(shape,texture->image);model.active=1;model.clump=0x2000;model.frame=0x3000;model.scale={1,1,1};model.material_flags=0xfeffffff;model.property=p.auto_alpha_blend?1:0;frame_alive=true;return true;
    }
    void SetPosition(unsigned m,const std::array<float,3>& v,unsigned mode) override{CheckModel(m);Finite(v);Require(mode<=1,"invalid cylinder position mode");(void)pal4::VisualModel::SetPosition(model,Vector(v),mode!=0,*this);}
    void SetRotation(unsigned m,const std::array<float,3>& v,unsigned mode) override{CheckModel(m);Finite(v);Require(mode<=1,"invalid cylinder rotation mode");(void)pal4::VisualModel::SetRotation(model,Vector(v),mode!=0,*this);}
    void SetScale(unsigned m,const std::array<float,3>& v,unsigned mode) override{CheckModel(m);Finite(v);Require(mode<=1,"invalid cylinder scale mode");(void)pal4::VisualModel::SetScale(model,Vector(v),mode!=0,*this);}
    std::uint8_t SetPosition(unsigned m,const std::array<float,3>& v,std::uint8_t mode) override{SetPosition(m,v,unsigned(mode));return 1;}
    std::uint8_t SetRotation(unsigned m,const std::array<float,3>& v,std::uint8_t mode) override{SetRotation(m,v,unsigned(mode));return 1;}
    std::uint8_t SetScale(unsigned m,const std::array<float,3>& v,std::uint8_t mode) override{SetScale(m,v,unsigned(mode));return 1;}
    void SetColor(unsigned m,unsigned rgba) override{CheckModel(m);(void)pal4::VisualModel::SetMaterialFlags(model,rgba,*this);}
    std::uint8_t SetColor(unsigned m,const std::array<std::uint8_t,4>& color) override{unsigned rgba{};for(unsigned i=0;i<4;++i)rgba|=unsigned(color[i])<<(8*i);SetColor(m,rgba);return 1;}
    unsigned CopyDocument(unsigned d) override{Require(d==1,"invalid cylinder document copy");return 0x4000;}
    void DestroyModel(unsigned m) override{CheckModel(m);pal4::VisualModel::Destroy(model,*this);}
    void FreeModel(unsigned m) override{CheckModel(m);Require(!mesh && !model.active,"cylinder model still live");model={};allocated=false;}
    void ReleaseTexturePath(std::string_view path) override{Require(path.starts_with("gamedata\\effect\\tex\\"),"invalid cylinder temporary texture path");}
    void SetFrameIdentity(unsigned f) override{Require(frame_alive && f==0x3000,"invalid cylinder frame");pending_position={};pending_rotation={};pending_scale={1,1,1};}
    void ScaleFrame(unsigned f,pal4::VisualModelVector v,unsigned mode) override{Require(f==0x3000 && frame_alive && mode==0,"invalid cylinder frame scale");pending_scale=Vector(v);}
    void RotateFrame(unsigned f,unsigned axis,float angle,unsigned mode) override{Require(f==0x3000 && frame_alive && mode==2 && axis>=0x84b380 && (axis-0x84b380)%12==0 && (axis-0x84b380)/12<3,"invalid cylinder rotation axis");pending_rotation[(axis-0x84b380)/12]=angle;}
    void TranslateFrame(unsigned f,pal4::VisualModelVector v,unsigned mode) override{Require(f==0x3000 && frame_alive && mode==2,"invalid cylinder frame translation");pending_position=Vector(v);}
    int UpdateFrameObjects(unsigned f) override{Require(f==0x3000 && frame_alive,"invalid cylinder frame update");auto local=InstanceMatrix(pending_position,pending_rotation);for(unsigned c=0;c<3;++c)for(unsigned r=0;r<3;++r)local[c*4+r]*=pending_scale[c];matrix=MultiplyEffectMatrices(parent,local);return 1;}
    int SetMaterialFlagsForAllAtomics(unsigned c,unsigned) override{Require(c==0x2000 && mesh,"invalid cylinder material owner");return 1;}
    unsigned FindClumpWorld(unsigned c) override{Require(c==0x2000 && mesh,"invalid cylinder world query");return 0;}
    unsigned FindWorld(unsigned c) override{return FindClumpWorld(c);}
    void RemoveClumpFromWorld(unsigned,unsigned) override{throw std::logic_error("cylinder world attachment is not bound");}
    std::uint8_t RemoveClump(unsigned,unsigned) override{throw std::logic_error("cylinder world attachment is not bound");}
    void ClearClumpFrameLink(unsigned c) override{Require(c==0x2000 && mesh,"invalid cylinder clump frame link");}
    void DestroyClump(unsigned c) override{Require(c==0x2000 && mesh,"invalid cylinder clump destruction");mesh.reset();}
    void DestroyFrame(unsigned f) override{Require(f==0x3000 && frame_alive,"invalid cylinder frame destruction");frame_alive=false;}
    std::array<std::uint8_t,4> QueryTimedFacet(pal4::CylinderObjectState& s,unsigned offset) override{Require(&s==&state && offset==44,"invalid cylinder color facet");std::array<std::uint8_t,4> c{};(void)pal4::CylinderObjectColor::Get(state,c,*this);return c;}
    void QueryTaskValues(pal4::CylinderObjectState& s,unsigned offset,std::array<int,4>& v) override{const auto c=QueryTimedFacet(s,offset);for(unsigned i=0;i<4;++i)v[i]=c[i];}
    double Duration(pal4::CylinderObjectState&) override{return pal4::EffectBaseAccess::DefaultFactor();}
    int Ftol(double value) override{if(!std::isfinite(value)||value<-2147483648.0||value>=2147483648.0)return std::numeric_limits<int>::min();return static_cast<int>(value);}
    void ApplyTimedFacet(pal4::CylinderObjectState& s,unsigned offset,const std::array<std::uint8_t,4>& c) override{Require(&s==&state && offset==44,"invalid cylinder timed color");(void)pal4::CylinderObjectColor::Set(state,c,*this);}
};
EffectCylinder::EffectCylinder(const BinaryXmlNode& document,EffectTextures& textures,EffectObjectCounts& counts):impl_(std::make_unique<Impl>(counts,textures)){
    Require(document.name=="CylinderObject","wrong cylinder configuration class");impl_->document=document;Require(pal4::CylinderObjectConfiguration::Load(impl_->state,1,*impl_)!=0,"original cylinder configuration failed");
}
EffectCylinder::~EffectCylinder()=default;
const Scene& EffectCylinder::Mesh() const{Require(impl_->mesh.has_value(),"cylinder is not loaded");return impl_->mesh->scene;}
const std::array<float,16>& EffectCylinder::Transform() const{return impl_->matrix;}
bool EffectCylinder::Additive() const{return impl_->model.property!=0;}
bool EffectCylinder::Visible() const{return (impl_->state.words[1]&0xff)!=0 && (impl_->state.words[1]>>24)!=0;}
double EffectCylinder::Duration() const{return pal4::EffectBaseAccess::DefaultFactor();}
void EffectCylinder::SetParentTransform(const std::array<float,16>& parent){ValidateEffectMatrix(parent);impl_->parent=parent;(void)impl_->UpdateFrameObjects(0x3000);}
void EffectCylinder::Show(bool visible){(void)pal4::CylinderObjectVisibilityFacet::Deactivate(impl_->state,*impl_);if(visible)(void)pal4::EffectSystemObjectFFacet::EnableVisibility(impl_->state.words[1]);}
void EffectCylinder::Reset(){(void)pal4::CylinderObjectConfiguration::Reset(impl_->state,*impl_);}
std::array<float,3> EffectCylinder::Scale() const{std::array<unsigned,3> bits{};(void)pal4::CylinderObjectTransform::GetScale(impl_->state,bits,*impl_);return std::bit_cast<std::array<float,3>>(bits);}
void EffectCylinder::SetScale(std::array<float,3> v,bool mode){(void)pal4::CylinderObjectTransform::SetScale(impl_->state,v,mode,*impl_);}
void EffectCylinder::SetRotation(std::array<float,3> v,bool mode){(void)pal4::CylinderObjectTransform::SetRotation(impl_->state,v,mode,*impl_);}
void EffectCylinder::SetPosition(std::array<float,3> v,bool mode){(void)pal4::CylinderObjectTransform::SetPosition(impl_->state,v,mode,*impl_);}
void EffectCylinder::SetColor(std::array<std::uint8_t,4> v){(void)pal4::CylinderObjectColor::Set(impl_->state,v,*impl_);}
std::array<std::uint8_t,4> EffectCylinder::Color() const{std::array<std::uint8_t,4> c{};(void)pal4::CylinderObjectColor::Get(impl_->state,c,*impl_);return c;}
// 原Cylinder颜色facet+4指向5A6010空回调，不改变材质或定时alpha模式。
void EffectCylinder::SetColorMode(std::uint8_t){}
void EffectCylinder::SetTimedMode(bool enabled){pal4::CylinderObject::SetTimedMode(impl_->state,enabled,*impl_);}
void EffectCylinder::Advance(double elapsed){Require(std::isfinite(elapsed) && elapsed>=0,"invalid cylinder elapsed");pal4::CylinderObjectUpdater::Update(impl_->state,elapsed,*impl_);}
std::vector<DrawAppearance> EffectCylinder::Appearance() const{const auto color=Color();std::array<float,4> c{};for(unsigned i=0;i<4;++i)c[i]=float(color[i])/255.f;return std::vector<DrawAppearance>(Mesh().draws.size(),{Visible(),c});}
void EffectCylinder::WriteReport(std::ostream& out) const{const auto& s=*impl_;const auto color=Color();out<<std::setprecision(9)<<"{\"visible\":"<<(Visible()?"true":"false")<<",\"additive\":"<<(Additive()?"true":"false")<<",\"texture\":"<<std::quoted(s.texture->member)<<",\"texture_bytes\":"<<s.texture->source_bytes<<",\"width\":"<<s.texture->image.width<<",\"height\":"<<s.texture->image.height<<",\"rgba_crc32\":"<<s.texture->rgba_crc32<<",\"rgba\":[";for(unsigned i=0;i<4;++i){if(i)out<<',';out<<unsigned(color[i]);}out<<"],\"matrix\":[";for(unsigned i=0;i<16;++i){if(i)out<<',';out<<s.matrix[i];}out<<"],\"triangles\":"<<s.mesh->source_triangles<<'}';}
}
