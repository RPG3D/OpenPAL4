#pragma once
#include "pal4/binary_xml_config.h"
#include <array>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
namespace pal4::portable {
struct GobEffectParameters {
    std::string name;
    std::uint32_t times{},saver{},distance_clip{};
};
// 缺配置节点与存在节点但缺属性的原语义不同；前者保留原对象状态。
std::optional<GobEffectParameters> ReadGobEffectParameters(const BinaryXmlConfig& document);
struct EffectDocument {
    std::string name,member;
    std::size_t source_bytes{};
    BinaryXmlConfig xml;
};
struct EffectPlan {
    std::vector<std::string> object_ids;
    std::vector<std::int32_t> task_ids;
    std::vector<BinaryXmlNode> task_parameters;
    std::map<std::string,BinaryXmlNode> templates;
    std::map<std::string,std::array<float,3>> targets;
    bool distance_clip{};
    float camera_start{};
};
// 复用原Dialog配置/模板装载；不创建效果对象或启动任务。
EffectPlan ReadEffectPlan(const EffectDocument& document);
using EffectMemberReader=std::function<std::optional<std::vector<std::uint8_t>>(std::string_view)>;
// 共享仍被使用的解析文档；实例、任务、变换与GPU由播放所有者管理。
class EffectDocuments {
public:
    explicit EffectDocuments(EffectMemberReader reader);
    std::shared_ptr<const EffectDocument> Load(std::string_view name);
    std::size_t Size() const;
private:
    EffectMemberReader read_;
    std::map<std::string,std::weak_ptr<const EffectDocument>> documents_;
};
void WriteEffectDocumentReport(std::ostream& output,const EffectDocument& document);
}
