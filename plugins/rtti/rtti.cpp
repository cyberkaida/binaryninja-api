#include "rtti.h"

using namespace BinaryNinja;
using namespace BinaryNinja::RTTI;


std::optional<std::string> RTTI::DemangleNameMS(BinaryView* view, bool allowMangled, const std::string &mangledName)
{
    QualifiedName demangledName = {};
    Ref<Type> outType = {};
    if (!DemangleMS(view->GetDefaultArchitecture(), mangledName, outType, demangledName, true))
        return DemangleNameLLVM(allowMangled, mangledName);
    return demangledName.GetString();
}


std::optional<std::string> RTTI::DemangleNameGNU3(BinaryView* view, bool allowMangled, const std::string &mangledName)
{
    QualifiedName demangledName = {};
    Ref<Type> outType = {};
    if (!DemangleGNU3(view->GetDefaultArchitecture(), mangledName, outType, demangledName, true))
        return DemangleNameLLVM(allowMangled, mangledName);
    return demangledName.GetString();
}


std::optional<std::string> RTTI::DemangleNameLLVM(bool allowMangled, const std::string &mangledName)
{
    QualifiedName demangledName = {};
    Ref<Type> outType = {};
    if (!DemangleLLVM(mangledName, demangledName, true))
        return allowMangled ? std::optional(mangledName) : std::nullopt;
    auto demangledNameStr = demangledName.GetString();
    size_t beginFind = demangledNameStr.find_first_of(' ');
    if (beginFind != std::string::npos)
        demangledNameStr.erase(0, beginFind + 1);
    size_t endFind = demangledNameStr.find(" `RTTI Type Descriptor Name'");
    if (endFind != std::string::npos)
        demangledNameStr.erase(endFind, demangledNameStr.length());
    return demangledNameStr;
}


Ref<Metadata> ClassInfo::SerializedMetadata()
{
    std::map<std::string, Ref<Metadata> > classInfoMeta;
    classInfoMeta["className"] = new Metadata(className);
    if (baseClassName.has_value())
        classInfoMeta["baseClassName"] = new Metadata(baseClassName.value());
    if (classOffset.has_value())
        classInfoMeta["classOffset"] = new Metadata(classOffset.value());
    if (vft.has_value())
        classInfoMeta["vft"] = vft->SerializedMetadata();
    // NOTE: We omit baseVft as it can be resolved manually and just bloats the size.
    return new Metadata(classInfoMeta);
}


ClassInfo ClassInfo::DeserializedMetadata(const Ref<Metadata> &metadata)
{
    std::map<std::string, Ref<Metadata> > classInfoMeta = metadata->GetKeyValueStore();
    ClassInfo info = {classInfoMeta["className"]->GetString()};
    if (classInfoMeta.find("baseClassName") != classInfoMeta.end())
        info.baseClassName = classInfoMeta["baseClassName"]->GetString();
    if (classInfoMeta.find("classOffset") != classInfoMeta.end())
        info.classOffset = classInfoMeta["classOffset"]->GetUnsignedInteger();
    if (classInfoMeta.find("vft") != classInfoMeta.end())
        info.vft = VirtualFunctionTableInfo::DeserializedMetadata(classInfoMeta["vft"]);
    return info;
}


Ref<Metadata> VirtualFunctionTableInfo::SerializedMetadata()
{
    std::vector<Ref<Metadata> > funcsMeta;
    funcsMeta.reserve(virtualFunctions.size());
    for (auto &vFunc: virtualFunctions)
        funcsMeta.emplace_back(vFunc.SerializedMetadata());
    std::map<std::string, Ref<Metadata> > vftMeta;
    vftMeta["address"] = new Metadata(address);
    vftMeta["functions"] = new Metadata(funcsMeta);
    return new Metadata(vftMeta);
}


VirtualFunctionTableInfo VirtualFunctionTableInfo::DeserializedMetadata(const Ref<Metadata> &metadata)
{
    std::map<std::string, Ref<Metadata> > vftMeta = metadata->GetKeyValueStore();
    VirtualFunctionTableInfo vftInfo = {vftMeta["address"]->GetUnsignedInteger()};
    if (vftMeta.find("functions") != vftMeta.end())
    {
        for (auto &entry: vftMeta["functions"]->GetArray())
            vftInfo.virtualFunctions.emplace_back(VirtualFunctionInfo::DeserializedMetadata(entry));
    }
    return vftInfo;
}


Ref<Metadata> VirtualFunctionInfo::SerializedMetadata()
{
    std::map<std::string, Ref<Metadata> > vFuncMeta;
    vFuncMeta["address"] = new Metadata(funcAddr);
    return new Metadata(vFuncMeta);
}


VirtualFunctionInfo VirtualFunctionInfo::DeserializedMetadata(const Ref<Metadata> &metadata)
{
    std::map<std::string, Ref<Metadata> > vFuncMeta = metadata->GetKeyValueStore();
    VirtualFunctionInfo vFuncInfo = {vFuncMeta["address"]->GetUnsignedInteger()};
    return vFuncInfo;
}