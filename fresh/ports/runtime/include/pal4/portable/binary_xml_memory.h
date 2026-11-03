#pragma once
#include "pal4/binary_xml_config.h"
namespace pal4::portable {
// Update the first matching attribute or append a new INT/FLOAT attribute.
// Existing type is preserved through original XmlAttributeRecord setters.
// Other attributes/children are unchanged. Each value update commits only
// after successful conversion; a sequence of updates is not transactional.
void WriteBinaryXmlInt(BinaryXmlNode&,std::string_view key,std::int32_t);
void WriteBinaryXmlFloat(BinaryXmlNode&,std::string_view key,float);
}
