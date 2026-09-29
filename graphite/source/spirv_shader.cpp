#include "pch.hpp"
#include "graph/shader.hpp"
#include "core.hpp"
#include "tkit/utils/storage.hpp"
#include "tkit/container/hive.hpp"
#include <spirv_reflect.h>

namespace Graph
{
struct Spirv_Reflection
{
    TKit::TierArray<SpvReflectShaderModule> Modules{};
    TKit::TierArray<DescriptorSetReflectionInfo> Sets{};
    TKit::TierArray<PushConstantRange> PushRanges{};
    TKit::TierArray<ShaderStageInfo> Stages{};
    TKit::StaticArray128<SpecializationEntry> SpecEntries{};
    TKit::TierArray<void *> SpecData{};
    TKit::TierArray<VertexAttributeReflectionInfo> VertexAttributes{};

    DescriptorSetReflectionInfo *FindSet(const u32 set)
    {
        for (DescriptorSetReflectionInfo &s : Sets)
            if (s.Set == set)
                return &s;
        return nullptr;
    }

    PushConstantRange *FindRange(const u32 offset, const u32 size)
    {
        for (PushConstantRange &pr : PushRanges)
            if (pr.Offset == offset && pr.Size == size)
                return &pr;
        return nullptr;
    }
};

static TKit::Storage<TKit::ArenaHive<Spirv_Reflection>> s_Reflections{};

static void reflection_Destroy(Spirv_Reflection &refl)
{
    for (SpvReflectShaderModule &mod : refl.Modules)
        spvReflectDestroyShaderModule(&mod);
    TKit::TierAllocator *tier = TKit::GetTier();
    for (const void *data : refl.SpecData)
        tier->DeallocateWithHeader(data);
}

void Reflection_Initialize(const u32 maxReflections)
{
    s_Reflections.Construct();
    s_Reflections->Reserve(maxReflections);
}

void Reflection_Terminate()
{
    GRAPH_CLEANUP_WITH_WARNING_LAMBDA(s_Reflections, "REFLECTION", "reflections", reflection_Destroy);
    s_Reflections.Destruct();
}

static DescriptorType fromReflect(const SpvReflectDescriptorType type)
{
    switch (type)
    {
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        return Descriptor_StorageBuffer;
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        return Descriptor_UniformBuffer;
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
        return Descriptor_Sampler;
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        return Descriptor_SampledImage;
    case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        return Descriptor_CombinedImageSampler;
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        return Descriptor_StorageImage;
    default:
        TKIT_FATAL("[GRAPH] Unsupported SPIR-V descriptor type: {}", u32(type));
        return Descriptor_StorageBuffer;
    }
}

static ShaderStageFlagBit fromReflect(const SpvReflectShaderStageFlagBits stage)
{
    switch (stage)
    {
    case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
        return ShaderStageFlag_Vertex;
    case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
        return ShaderStageFlag_Fragment;
    case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
        return ShaderStageFlag_Compute;
    default:
        TKIT_FATAL("[GRAPH] Unsupported SPIR-V shader stage: {}", u32(stage));
        return ShaderStageFlag_None;
    }
}

static Format fromReflect(const SpvReflectFormat format)
{
    switch (format)
    {
    case SPV_REFLECT_FORMAT_R16_UINT:
        return Format_R16_UINT;
    case SPV_REFLECT_FORMAT_R16_SINT:
        return Format_R16_SINT;
    case SPV_REFLECT_FORMAT_R16_SFLOAT:
        return Format_R16_SFLOAT;
    case SPV_REFLECT_FORMAT_R16G16_UINT:
        return Format_R16G16_UINT;
    case SPV_REFLECT_FORMAT_R16G16_SINT:
        return Format_R16G16_SINT;
    case SPV_REFLECT_FORMAT_R16G16_SFLOAT:
        return Format_R16G16_SFLOAT;
    case SPV_REFLECT_FORMAT_R16G16B16_UINT:
        return Format_R16G16B16_UINT;
    case SPV_REFLECT_FORMAT_R16G16B16_SINT:
        return Format_R16G16B16_SINT;
    case SPV_REFLECT_FORMAT_R16G16B16_SFLOAT:
        return Format_R16G16B16_SFLOAT;
    case SPV_REFLECT_FORMAT_R16G16B16A16_UINT:
        return Format_R16G16B16A16_UINT;
    case SPV_REFLECT_FORMAT_R16G16B16A16_SINT:
        return Format_R16G16B16A16_SINT;
    case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT:
        return Format_R16G16B16A16_SFLOAT;
    case SPV_REFLECT_FORMAT_R32_UINT:
        return Format_R32_UINT;
    case SPV_REFLECT_FORMAT_R32_SINT:
        return Format_R32_SINT;
    case SPV_REFLECT_FORMAT_R32_SFLOAT:
        return Format_R32_SFLOAT;
    case SPV_REFLECT_FORMAT_R32G32_UINT:
        return Format_R32G32_UINT;
    case SPV_REFLECT_FORMAT_R32G32_SINT:
        return Format_R32G32_SINT;
    case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
        return Format_R32G32_SFLOAT;
    case SPV_REFLECT_FORMAT_R32G32B32_UINT:
        return Format_R32G32B32_UINT;
    case SPV_REFLECT_FORMAT_R32G32B32_SINT:
        return Format_R32G32B32_SINT;
    case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
        return Format_R32G32B32_SFLOAT;
    case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
        return Format_R32G32B32A32_UINT;
    case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
        return Format_R32G32B32A32_SINT;
    case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
        return Format_R32G32B32A32_SFLOAT;
    default:
        TKIT_FATAL("[GRAPH][REFLECTION] Unsupported SPIR-V vertex attribute format: {}", u32(format));
        return Format_Undefined;
    }
}

static DescriptorBindingFlags fromReflectFlags(const SpvReflectBindingArrayTraits &array)
{
    DescriptorBindingFlags flags = 0;
    for (u32 i = 0; i < array.dims_count; ++i)
        if (array.dims[i] == SPV_REFLECT_ARRAY_DIM_RUNTIME)
        {
            flags |= DescriptorBindingFlag_VariableDescriptorCount;
            break;
        }
    return flags;
}

Reflection Reflection_Create(TKit::Span<const SpirvData> spirv)
{
    const Id id = s_Reflections->Insert();
    Spirv_Reflection &refl = s_Reflections->At(id);

    for (const SpirvData &spv : spirv)
    {
        SpvReflectShaderModule &module = refl.Modules.Append();
        TKIT_ASSERT_RETURNS(spvReflectCreateShaderModule(spv.Size, spv.Code, &module), SPV_REFLECT_RESULT_SUCCESS);
        const u32 setCount = module.descriptor_set_count;
        for (u32 i = 0; i < setCount; ++i)
        {
            const SpvReflectDescriptorSet &set = module.descriptor_sets[i];
            DescriptorSetReflectionInfo *setInfo = refl.FindSet(set.set);
            if (!setInfo)
            {
                setInfo = &refl.Sets.Append();
                setInfo->Set = set.set;
                for (u32 j = 0; j < set.binding_count; ++j)
                {
                    const SpvReflectDescriptorBinding *b = set.bindings[j];
                    setInfo->Bindings.Append(b->binding, b->count, fromReflect(b->descriptor_type),
                                             fromReflect(module.shader_stage), fromReflectFlags(b->array));
                }
            }
            else
                for (u32 j = 0; j < set.binding_count; ++j)
                {
                    const SpvReflectDescriptorBinding *b = set.bindings[j];
                    DescriptorBinding *found = setInfo->FindBinding(b->binding);
                    if (!found)
                        setInfo->Bindings.Append(b->binding, b->count, fromReflect(b->descriptor_type),
                                                 fromReflect(module.shader_stage), fromReflectFlags(b->array));
                    else
                    {
                        TKIT_ASSERT(b->count == found->DescriptorCount,
                                    "[GRAPH][REFLECTION] Found descriptor set {} containing binding {} in multiple "
                                    "shader modules with conflicting counts: {} != {}",
                                    set.set, b->binding, b->count, found->DescriptorCount);
                        TKIT_ASSERT(fromReflect(b->descriptor_type) == found->Type,
                                    "[GRAPH][REFLECTION] Found descriptor set {} containing binding {} in multiple "
                                    "shader modules with conflicting types: {} != {}",
                                    set.set, b->binding, u32(fromReflect(b->descriptor_type)), u32(found->Type));
                        TKIT_ASSERT(fromReflectFlags(b->array) == found->Flags,
                                    "[GRAPH][REFLECTION] Found descriptor set {} containing binding {} in multiple "
                                    "shader modules with conflicting flags: {} != {}",
                                    set.set, b->binding, u32(fromReflectFlags(b->array)), u32(found->Flags));

                        found->ShaderStages |= fromReflect(module.shader_stage);
                    }
                }
        }
        for (u32 i = 0; i < module.push_constant_block_count; ++i)
        {
            const SpvReflectBlockVariable &pc = module.push_constant_blocks[i];
            PushConstantRange *range = refl.FindRange(pc.offset, pc.size);
            if (!range)
            {
                range = &refl.PushRanges.Append();
                range->Size = pc.size;
                range->Offset = pc.offset;
                range->Stages = fromReflect(module.shader_stage);
            }
            else
                range->Stages |= fromReflect(module.shader_stage);
        }

        TKIT_ASSERT(module.entry_point_count == 1,
                    "[GRAPH][REFLECTION] More than one entry point per module is not supported");
        ShaderStageInfo &sinfo = refl.Stages.Append();
        sinfo.EntryPoint = module.entry_point_name;
        sinfo.Shader = NullHandle;

        u32 totSize = 0;
        u32 entryOffset = refl.SpecEntries.GetSize();
        for (u32 i = 0; i < module.spec_constant_count; ++i)
        {
            const SpvReflectSpecializationConstant &spec = module.spec_constants[i];
            refl.SpecEntries.Append(spec.constant_id, totSize, spec.default_value_size);
            totSize += spec.default_value_size;
        }

        if (totSize != 0)
        {
            sinfo.Specialization.Entries = TKit::Span<const SpecializationEntry>{refl.SpecEntries.begin() + entryOffset,
                                                                                 module.spec_constant_count};
            TKit::TierAllocator *tier = TKit::GetTier();
            u8 *data = scast<u8 *>(refl.SpecData.Append(tier->AllocateWithHeader(totSize)));
            u32 offset = 0;
            for (u32 i = 0; i < module.spec_constant_count; ++i)
            {
                const SpvReflectSpecializationConstant &spec = module.spec_constants[i];
                TKit::ForwardCopy(data + offset, spec.default_value, spec.default_value_size);
                offset += spec.default_value_size;
            }
        }

        if (module.shader_stage == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
        {
            const SpvReflectEntryPoint &ep = module.entry_points[0];
            for (u32 i = 0; i < ep.input_variable_count; ++i)
            {
                VertexAttributeReflectionInfo &info = refl.VertexAttributes.Append();
                const SpvReflectInterfaceVariable *var = ep.input_variables[i];
                if (var->built_in != -1)
                    continue;
                info.Format = fromReflect(var->format);
                info.Location = var->location;
            }
        }
    }

    return Handle_Create(Handle_Reflection, id);
}

} // namespace Graph
