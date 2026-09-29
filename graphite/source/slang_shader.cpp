#include "pch.hpp"
#include "graph/shader.hpp"
#include "core.hpp"
#include "tkit/container/stack_array.hpp"
#include "tkit/utils/storage.hpp"
#include "tkit/container/hive.hpp"
#include <slang.h>
#include <slang-com-ptr.h>

namespace Graph
{
struct CompiledSpirv
{
    const char *Module;
    EntryPoint EntryPoint;
    SpirvData Spirv;
};
using Slang_Compilation = TKit::TierArray<CompiledSpirv>;

static TKit::Storage<TKit::ArenaHive<Slang_Compilation>> s_Compilations{};

static void compilation_Destroy(const Slang_Compilation &comp)
{
    TKit::TierAllocator *tier = TKit::GetTier();
    for (const CompiledSpirv &spr : comp)
        tier->Deallocate(scast<const void *>(spr.Spirv.Code), spr.Spirv.Size);
}

void Compilation_Initialize(const u32 maxCompilations)
{
    s_Compilations.Construct();
    s_Compilations->Reserve(maxCompilations);
}

void Compilation_Terminate()
{
    GRAPH_CLEANUP_WITH_WARNING_LAMBDA(s_Compilations, "SHADER", "compilations", compilation_Destroy);

    s_Compilations.Destruct();
}

using Slang::ComPtr;
static SlangStage toSlang(const ShaderStageFlagBit stage)
{
    switch (stage)
    {
    case ShaderStageFlag_Vertex:
        return SLANG_STAGE_VERTEX;
    case ShaderStageFlag_Fragment:
        return SLANG_STAGE_FRAGMENT;
    case ShaderStageFlag_Compute:
        return SLANG_STAGE_COMPUTE;
    default:
        return SLANG_STAGE_NONE;
    }
}
static ShaderStageFlagBit fromSlang(const SlangStage stage)
{
    switch (stage)
    {
    case SLANG_STAGE_VERTEX:
        return ShaderStageFlag_Vertex;
    case SLANG_STAGE_FRAGMENT:
        return ShaderStageFlag_Fragment;
    case SLANG_STAGE_COMPUTE:
        return ShaderStageFlag_Compute;
    default:
        return ShaderStageFlag_None;
    }
}

static slang::CompilerOptionName toSlang(const ShaderArgumentName arg)
{
    using SO = slang::CompilerOptionName;

    switch (arg)
    {
    case ShaderArgument_MacroDefine:
        return SO::MacroDefine;
    case ShaderArgument_DepFile:
        return SO::DepFile;
    case ShaderArgument_EntryPointName:
        return SO::EntryPointName;
    case ShaderArgument_Specialize:
        return SO::Specialize;
    case ShaderArgument_Help:
        return SO::Help;
    case ShaderArgument_HelpStyle:
        return SO::HelpStyle;
    case ShaderArgument_Include:
        return SO::Include;
    case ShaderArgument_Language:
        return SO::Language;
    case ShaderArgument_MatrixLayoutColumn:
        return SO::MatrixLayoutColumn;
    case ShaderArgument_MatrixLayoutRow:
        return SO::MatrixLayoutRow;
    case ShaderArgument_ZeroInitialize:
        return SO::ZeroInitialize;
    case ShaderArgument_IgnoreCapabilities:
        return SO::IgnoreCapabilities;
    case ShaderArgument_RestrictiveCapabilityCheck:
        return SO::RestrictiveCapabilityCheck;
    case ShaderArgument_ModuleName:
        return SO::ModuleName;
    case ShaderArgument_Output:
        return SO::Output;
    case ShaderArgument_Profile:
        return SO::Profile;
    case ShaderArgument_Stage:
        return SO::Stage;
    case ShaderArgument_Target:
        return SO::Target;
    case ShaderArgument_Version:
        return SO::Version;
    case ShaderArgument_WarningsAsErrors:
        return SO::WarningsAsErrors;
    case ShaderArgument_DisableWarnings:
        return SO::DisableWarnings;
    case ShaderArgument_EnableWarning:
        return SO::EnableWarning;
    case ShaderArgument_DisableWarning:
        return SO::DisableWarning;
    case ShaderArgument_DumpWarningDiagnostics:
        return SO::DumpWarningDiagnostics;
    case ShaderArgument_InputFilesRemain:
        return SO::InputFilesRemain;
    case ShaderArgument_EmitIr:
        return SO::EmitIr;
    case ShaderArgument_ReportDownstreamTime:
        return SO::ReportDownstreamTime;
    case ShaderArgument_ReportPerfBenchmark:
        return SO::ReportPerfBenchmark;
    case ShaderArgument_ReportCheckpointIntermediates:
        return SO::ReportCheckpointIntermediates;
    case ShaderArgument_SkipSPIRVValidation:
        return SO::SkipSPIRVValidation;
    case ShaderArgument_SourceEmbedStyle:
        return SO::SourceEmbedStyle;
    case ShaderArgument_SourceEmbedName:
        return SO::SourceEmbedName;
    case ShaderArgument_SourceEmbedLanguage:
        return SO::SourceEmbedLanguage;
    case ShaderArgument_DisableShortCircuit:
        return SO::DisableShortCircuit;
    case ShaderArgument_MinimumSlangOptimization:
        return SO::MinimumSlangOptimization;
    case ShaderArgument_DisableNonEssentialValidations:
        return SO::DisableNonEssentialValidations;
    case ShaderArgument_DisableSourceMap:
        return SO::DisableSourceMap;
    case ShaderArgument_UnscopedEnum:
        return SO::UnscopedEnum;
    case ShaderArgument_PreserveParameters:
        return SO::PreserveParameters;

    case ShaderArgument_Capability:
        return SO::Capability;
    case ShaderArgument_DefaultImageFormatUnknown:
        return SO::DefaultImageFormatUnknown;
    case ShaderArgument_DisableDynamicDispatch:
        return SO::DisableDynamicDispatch;
    case ShaderArgument_DisableSpecialization:
        return SO::DisableSpecialization;
    case ShaderArgument_FloatingPointMode:
        return SO::FloatingPointMode;
    case ShaderArgument_DebugInformation:
        return SO::DebugInformation;
    case ShaderArgument_LineDirectiveMode:
        return SO::LineDirectiveMode;
    case ShaderArgument_Optimization:
        return SO::Optimization;
    case ShaderArgument_Obfuscate:
        return SO::Obfuscate;

    case ShaderArgument_VulkanBindShift:
        return SO::VulkanBindShift;
    case ShaderArgument_VulkanBindGlobals:
        return SO::VulkanBindGlobals;
    case ShaderArgument_VulkanInvertY:
        return SO::VulkanInvertY;
    case ShaderArgument_VulkanUseDxPositionW:
        return SO::VulkanUseDxPositionW;
    case ShaderArgument_VulkanUseEntryPointName:
        return SO::VulkanUseEntryPointName;
    case ShaderArgument_VulkanUseGLLayout:
        return SO::VulkanUseGLLayout;
    case ShaderArgument_VulkanEmitReflection:
        return SO::VulkanEmitReflection;

    case ShaderArgument_GLSLForceScalarLayout:
        return SO::GLSLForceScalarLayout;
    case ShaderArgument_EnableEffectAnnotations:
        return SO::EnableEffectAnnotations;

    case ShaderArgument_EmitSpirvViaGLSL:
        return SO::EmitSpirvViaGLSL;
    case ShaderArgument_EmitSpirvDirectly:
        return SO::EmitSpirvDirectly;
    case ShaderArgument_SPIRVCoreGrammarJSON:
        return SO::SPIRVCoreGrammarJSON;
    case ShaderArgument_IncompleteLibrary:
        return SO::IncompleteLibrary;

    case ShaderArgument_CompilerPath:
        return SO::CompilerPath;
    case ShaderArgument_DefaultDownstreamCompiler:
        return SO::DefaultDownstreamCompiler;
    case ShaderArgument_DownstreamArgs:
        return SO::DownstreamArgs;
    case ShaderArgument_PassThrough:
        return SO::PassThrough;

    case ShaderArgument_DumpRepro:
        return SO::DumpRepro;
    case ShaderArgument_DumpReproOnError:
        return SO::DumpReproOnError;
    case ShaderArgument_ExtractRepro:
        return SO::ExtractRepro;
    case ShaderArgument_LoadRepro:
        return SO::LoadRepro;
    case ShaderArgument_LoadReproDirectory:
        return SO::LoadReproDirectory;
    case ShaderArgument_ReproFallbackDirectory:
        return SO::ReproFallbackDirectory;

    case ShaderArgument_DumpAst:
        return SO::DumpAst;
    case ShaderArgument_DumpIntermediatePrefix:
        return SO::DumpIntermediatePrefix;
    case ShaderArgument_DumpIntermediates:
        return SO::DumpIntermediates;
    case ShaderArgument_DumpIr:
        return SO::DumpIr;
    case ShaderArgument_DumpIrIds:
        return SO::DumpIrIds;
    case ShaderArgument_PreprocessorOutput:
        return SO::PreprocessorOutput;
    case ShaderArgument_OutputIncludes:
        return SO::OutputIncludes;
    case ShaderArgument_ReproFileSystem:
        return SO::ReproFileSystem;
    case ShaderArgument_SkipCodeGen:
        return SO::SkipCodeGen;
    case ShaderArgument_ValidateIr:
        return SO::ValidateIr;
    case ShaderArgument_VerbosePaths:
        return SO::VerbosePaths;
    case ShaderArgument_VerifyDebugSerialIr:
        return SO::VerifyDebugSerialIr;
    case ShaderArgument_NoCodeGen:
        return SO::NoCodeGen;

    case ShaderArgument_FileSystem:
        return SO::FileSystem;
    case ShaderArgument_Heterogeneous:
        return SO::Heterogeneous;
    case ShaderArgument_NoMangle:
        return SO::NoMangle;
    case ShaderArgument_NoHLSLBinding:
        return SO::NoHLSLBinding;
    case ShaderArgument_NoHLSLPackConstantBufferElements:
        return SO::NoHLSLPackConstantBufferElements;
    case ShaderArgument_ValidateUniformity:
        return SO::ValidateUniformity;
    case ShaderArgument_AllowGLSL:
        return SO::AllowGLSL;
    case ShaderArgument_EnableExperimentalPasses:
        return SO::EnableExperimentalPasses;
    case ShaderArgument_BindlessSpaceIndex:
        return SO::BindlessSpaceIndex;

    case ShaderArgument_ArchiveType:
        return SO::ArchiveType;
    case ShaderArgument_CompileCoreModule:
        return SO::CompileCoreModule;
    case ShaderArgument_Doc:
        return SO::Doc;

    case ShaderArgument_IrCompression:
        return SO::IrCompression;

    case ShaderArgument_LoadCoreModule:
        return SO::LoadCoreModule;
    case ShaderArgument_ReferenceModule:
        return SO::ReferenceModule;
    case ShaderArgument_SaveCoreModule:
        return SO::SaveCoreModule;
    case ShaderArgument_SaveCoreModuleBinSource:
        return SO::SaveCoreModuleBinSource;
    case ShaderArgument_TrackLiveness:
        return SO::TrackLiveness;
    case ShaderArgument_LoopInversion:
        return SO::LoopInversion;

    case ShaderArgument_ParameterBlocksUseRegisterSpaces:
        return SO::ParameterBlocksUseRegisterSpaces;
    case ShaderArgument_LanguageVersion:
        return SO::LanguageVersion;
    case ShaderArgument_TypeConformance:
        return SO::TypeConformance;
    case ShaderArgument_EnableExperimentalDynamicDispatch:
        return SO::EnableExperimentalDynamicDispatch;
    case ShaderArgument_EmitReflectionJSON:
        return SO::EmitReflectionJSON;

    case ShaderArgument_DebugInformationFormat:
        return SO::DebugInformationFormat;
    case ShaderArgument_VulkanBindShiftAll:
        return SO::VulkanBindShiftAll;
    case ShaderArgument_GenerateWholeProgram:
        return SO::GenerateWholeProgram;
    case ShaderArgument_UseUpToDateBinaryModule:
        return SO::UseUpToDateBinaryModule;
    case ShaderArgument_EmbedDownstreamIR:
        return SO::EmbedDownstreamIR;
    case ShaderArgument_ForceDXLayout:
        return SO::ForceDXLayout;

    case ShaderArgument_EmitSpirvMethod:
        return SO::EmitSpirvMethod;
    case ShaderArgument_SaveGLSLModuleBinSource:
        return SO::SaveGLSLModuleBinSource;
    case ShaderArgument_SkipDownstreamLinking:
        return SO::SkipDownstreamLinking;
    case ShaderArgument_DumpModule:
        return SO::DumpModule;
    case ShaderArgument_GetModuleInfo:
        return SO::GetModuleInfo;
    case ShaderArgument_GetSupportedModuleVersions:
        return SO::GetSupportedModuleVersions;
    case ShaderArgument_EmitSeparateDebug:
        return SO::EmitSeparateDebug;

    case ShaderArgument_DenormalModeFp16:
        return SO::DenormalModeFp16;
    case ShaderArgument_DenormalModeFp32:
        return SO::DenormalModeFp32;
    case ShaderArgument_DenormalModeFp64:
        return SO::DenormalModeFp64;

    case ShaderArgument_UseMSVCStyleBitfieldPacking:
        return SO::UseMSVCStyleBitfieldPacking;
    case ShaderArgument_ForceCLayout:
        return SO::ForceCLayout;
    case ShaderArgument_ExperimentalFeature:
        return SO::ExperimentalFeature;
    case ShaderArgument_REMOVED_SerialIR:
        return SO::REMOVED_SerialIR;
    case ShaderArgument_CountOfParsableOptions:
        return SO::CountOfParsableOptions;
    }

    return SO::CountOf;
}

static TKit::TierString fromSlang(slang::IBlob *diagnostics)
{
    if (!diagnostics)
        return "No diagnostics available";

    const char *text = scast<const char *>(diagnostics->getBufferPointer());
    const size_t size = diagnostics->getBufferSize();
    const TKit::TierString message{text, size};
    return message;
}

using Result = TKit::Result<Compilation, TKit::TierString>;
Result Compilation_Create(const CompilationSpecs &specs, const CompilationFlags flags)
{
    ComPtr<slang::IGlobalSession> gsession = nullptr;
    SlangGlobalSessionDesc desc{};
    desc.enableGLSL = flags & CompilationFlag_EnableGlsl;

    SlangResult result = slang::createGlobalSession(&desc, gsession.writeRef());
    TKIT_ASSERT(SLANG_SUCCEEDED(result), "[GRAPH][SHADERS] Slang global session creation failed");

    ComPtr<slang::ISession> session = nullptr;
    slang::TargetDesc tdesc{};
    tdesc.format = SLANG_SPIRV;
    tdesc.profile = gsession->findProfile("spirv_1_5");

    slang::SessionDesc cdesc{};
    cdesc.targets = &tdesc;
    cdesc.targetCount = 1;

    TKit::StackArray<slang::PreprocessorMacroDesc> defines;
    defines.Reserve(specs.Macros.GetSize());
    for (const ShaderMacro &def : specs.Macros)
        defines.Append(def.Name, def.Value);

    if (!defines.IsEmpty())
    {
        cdesc.preprocessorMacroCount = specs.Macros.GetSize();
        cdesc.preprocessorMacros = defines.GetData();
    }

    TKit::StackArray<slang::CompilerOptionEntry> coptions{};
    coptions.Reserve(specs.Arguments.GetSize());

    for (const ShaderArgument &sa : specs.Arguments)
    {
        slang::CompilerOptionEntry entry{};
        entry.name = toSlang(sa.Name);
        entry.value.intValue0 = sa.Value.Value0;
        entry.value.intValue1 = sa.Value.Value1;
        entry.value.stringValue0 = sa.Value.String0;
        entry.value.stringValue1 = sa.Value.String1;

        coptions.Append(entry);
    }

    if (!coptions.IsEmpty())
    {
        cdesc.compilerOptionEntries = coptions.GetData();
        cdesc.compilerOptionEntryCount = coptions.GetSize();
    }

    if (!specs.SearchPaths.IsEmpty())
    {
        cdesc.searchPaths = specs.SearchPaths.GetData();
        cdesc.searchPathCount = specs.SearchPaths.GetSize();
    }

    cdesc.enableEffectAnnotations = flags & CompilationFlag_EnableEffectAnnotations;
    cdesc.allowGLSLSyntax = flags & CompilationFlag_AllowGlslSyntax;
    cdesc.skipSPIRVValidation = flags & CompilationFlag_SkipSpirvValidation;

    result = gsession->createSession(cdesc, session.writeRef());
    if (SLANG_FAILED(result))
        return Result::Error("[GRAPH][SHADERS] Slang compile session creation failed");

    ComPtr<slang::IBlob> diagnostics = nullptr;
    Slang_Compilation compilation{};

    for (const ShaderModule &munit : specs.Modules)
    {
        TKit::StackArray<ComPtr<slang::IComponentType>> components{};
        components.Reserve(munit.EntryPoints.GetSize() + 1);

        ComPtr<slang::IModule> module = nullptr;
        if (munit.SourceCode)
            module =
                session->loadModuleFromSourceString(munit.Name, munit.Path, munit.SourceCode, diagnostics.writeRef());
        else
            module = session->loadModule(munit.Name, diagnostics.writeRef());

        if (!module)
        {
            compilation_Destroy(compilation);
            return Result::Error(TKit::TierString::Format("[GRAPH][SHADERS] Failed to load shader module '{}': {}",
                                                          munit.Name, fromSlang(diagnostics)));
        }

        components.Append(module);
        TKIT_LOG_WARNING_IF(diagnostics,
                            "[GRAPH][SHADERS] Shader module '{}' loaded with the following diagnostics: {}", munit.Name,
                            fromSlang(diagnostics));
        TKIT_LOG_INFO_IF(!diagnostics, "[GRAPH][SHADERS] Successfully loaded module '{}'", munit.Name);

        for (const EntryPoint &ep : munit.EntryPoints)
        {
            ComPtr<slang::IEntryPoint> epoint = nullptr;
            result =
                module->findAndCheckEntryPoint(ep.Name, toSlang(ep.Stage), epoint.writeRef(), diagnostics.writeRef());
            if (SLANG_FAILED(result))
            {
                compilation_Destroy(compilation);
                return Result::Error(
                    TKit::TierString::Format("[GRAPH][SHADERS] Failed to check entry point '{}' from module '{}': {}",
                                             ep.Name, munit.Name, fromSlang(diagnostics)));
            }

            TKIT_LOG_WARNING_IF(
                diagnostics,
                "[GRAPH][SHADERS] Entry point '{}' from module '{}' checked with the following diagnostics: {}",
                ep.Name, munit.Name, fromSlang(diagnostics));

            components.Append(epoint);
        }
        ComPtr<slang::IComponentType> program = nullptr;
        TKit::StackArray<slang::IComponentType *> rawComponents;
        rawComponents.Reserve(components.GetSize());
        for (const auto &cmp : components)
            rawComponents.Append(cmp);

        result = session->createCompositeComponentType(rawComponents.GetData(), rawComponents.GetSize(),
                                                       program.writeRef(), diagnostics.writeRef());
        if (SLANG_FAILED(result))
        {
            compilation_Destroy(compilation);
            return Result::Error(TKit::TierString::Format(
                "[GRAPH][SHADERS] Failed to create composite component type for module '{}': {}", munit.Name,
                fromSlang(diagnostics)));
        }

        TKIT_LOG_WARNING_IF(
            diagnostics,
            "[GRAPH][SHADERS] Created composite component type for module '{}' with the following diagnostics: {}",
            munit.Name, fromSlang(diagnostics));

        slang::IComponentType *linkedProgram;
        result = program->link(&linkedProgram, diagnostics.writeRef());
        if (SLANG_FAILED(result))
        {
            compilation_Destroy(compilation);
            return Result::Error(
                TKit::TierString::Format("[GRAPH][SHADERS] Failed to link final program for module '{}': {}",
                                         munit.Name, fromSlang(diagnostics)));
        }

        TKIT_LOG_WARNING_IF(diagnostics,
                            "[GRAPH][SHADERS] Linked final program for module '{}' with the following diagnostics: {}",
                            munit.Name, fromSlang(diagnostics));

        slang::ProgramLayout *layout = linkedProgram->getLayout();
        for (u32 i = 0; i < layout->getEntryPointCount(); ++i)
        {
            EntryPoint ep{};
            slang::EntryPointReflection *epr = layout->getEntryPointByIndex(i);

            ep.Stage = fromSlang(epr->getStage());
            for (const EntryPoint &mep : munit.EntryPoints)
                if (strcmp(mep.Name, epr->getName()) == 0)
                {
                    ep.Name = mep.Name;
                    break;
                }
            TKIT_ASSERT(ep.Name, "[GRAPH][SHADERS] Failed to recover entry point named '{}'", epr->getName());

            ComPtr<slang::IBlob> code = nullptr;
            result = linkedProgram->getEntryPointCode(i, 0, code.writeRef(), diagnostics.writeRef());

            if (SLANG_FAILED(result))
            {
                compilation_Destroy(compilation);
                return Result::Error(TKit::TierString::Format(
                    "[GRAPH][SHADERS] Failed to retrieve final code from entry point '{}' and module '{}': {}", ep.Name,
                    munit.Name, fromSlang(diagnostics)));
            }

            TKIT_LOG_WARNING_IF(diagnostics,
                                "[GRAPH][SHADERS] Retrieved final code for entry point '{}' and module '{}' with the "
                                "following diagnostics: {}",
                                ep.Name, munit.Name, fromSlang(diagnostics));

            const size_t size = code->getBufferSize();

            TKit::TierAllocator *tier = TKit::GetTier();
            void *mem = tier->Allocate(u32(size));
            TKit::ForwardCopy(mem, code->getBufferPointer(), size);

            CompiledSpirv sp;
            sp.EntryPoint = ep;
            sp.Module = munit.Name;
            sp.Spirv.Code = scast<u32 *>(mem);
            sp.Spirv.Size = u32(size);

            compilation.Append(sp);
        }
    }

    return Handle_Create(Handle_Compilation, s_Compilations->Insert(compilation));
}

void Compilation_Destroy(Compilation comp)
{
    GRAPH_CHECK_HANDLE(comp, Handle_Compilation);
    GRAPH_DESTROY_FUNCTION_BODY_LAMBDA(s_Compilations, comp, compilation_Destroy);
}

SpirvData Compilation_GetSpirv(const Compilation comp, const char *entryPoint, const char *module,
                               const ShaderStageFlagBit stage)
{
    GRAPH_CHECK_HANDLE(comp, Handle_Compilation);
    const Slang_Compilation &compilation = s_Compilations->At(Handle_GetId(comp));

    u32 index = TKIT_U32_MAX;
    for (u32 i = 0; i < compilation.GetSize(); ++i)
    {
        const CompiledSpirv &spr = compilation[index];
        bool match = strcmp(entryPoint, spr.EntryPoint.Name) == 0;
        match &= !module || strcmp(module, spr.Module) == 0;
        match &= stage == spr.EntryPoint.Stage;
        if (match)
        {
            TKIT_ASSERT(index == TKIT_U32_MAX,
                        "Found multiple endpoints named '{}'. If you have endpoints with the same name, the "
                        "module name, the stage or both must be provided as well to resolve the ambiguity",
                        entryPoint);
            index = i;
        }
    }
    TKIT_ASSERT(index != TKIT_U32_MAX, "[GRAPH][SHADERS] Entry point named '{}' was not found", entryPoint);
    return compilation[index].Spirv;
}

SpirvData Compilation_GetSpirv(const Compilation comp, const char *entryPoint, const char *module)
{
    GRAPH_CHECK_HANDLE(comp, Handle_Compilation);
    const Slang_Compilation &compilation = s_Compilations->At(Handle_GetId(comp));

    u32 index = TKIT_U32_MAX;
    for (u32 i = 0; i < compilation.GetSize(); ++i)
    {
        const CompiledSpirv &spr = compilation[i];
        bool match = strcmp(entryPoint, spr.EntryPoint.Name) == 0;
        match &= !module || strcmp(module, spr.Module) == 0;
        if (match)
        {
            TKIT_ASSERT(index == TKIT_U32_MAX,
                        "Found multiple endpoints named '{}'. If you have endpoints with the same name, the "
                        "module name, the stage or both must be provided as well to resolve the ambiguity",
                        entryPoint);
            index = i;
        }
    }
    TKIT_ASSERT(index != TKIT_U32_MAX, "[GRAPH][SHADERS] Entry point named '{}' was not found", entryPoint);
    return compilation[index].Spirv;
}

bool Compilation_IsHandleValid(const Compilation comp)
{
    GRAPH_CHECK_HANDLE(comp, Handle_Compilation);
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_Compilations, comp, Handle_Compilation);
}

} // namespace Graph
