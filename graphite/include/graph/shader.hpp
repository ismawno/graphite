#pragma once

#include "graph/handle.hpp"
#include "tkit/container/span.hpp"
#include "tkit/container/tier_array.hpp"
#include "tkit/utils/result.hpp"

namespace Graph
{
struct SpirvData
{
    const u32 *Code;
    usz Size;
};

Shader Shader_Create(const SpirvData &data);
Shader Shader_Create(TKit::StringView path);
Shader Shader_Create(const u32 *code, usz size)
{
    return Shader_Create({code, size});
}

void Shader_Destroy(Shader sh);

void Shader_SetName(Shader sh, const char *name);
bool Shader_IsHandleValid(Shader sh);

#ifdef GRAPH_HAS_SHADER_COMPILATION_BACKEND
enum ShaderArgumentName : u8
{
    ShaderArgument_MacroDefine, // stringValue0: macro name;  stringValue1: macro value
    ShaderArgument_DepFile,
    ShaderArgument_EntryPointName,
    ShaderArgument_Specialize,
    ShaderArgument_Help,
    ShaderArgument_HelpStyle,
    ShaderArgument_Include, // stringValue: additional include path.
    ShaderArgument_Language,
    ShaderArgument_MatrixLayoutColumn,         // bool
    ShaderArgument_MatrixLayoutRow,            // bool
    ShaderArgument_ZeroInitialize,             // bool
    ShaderArgument_IgnoreCapabilities,         // bool
    ShaderArgument_RestrictiveCapabilityCheck, // bool
    ShaderArgument_ModuleName,                 // stringValue0: module name.
    ShaderArgument_Output,
    ShaderArgument_Profile, // intValue0: profile
    ShaderArgument_Stage,   // intValue0: stage
    ShaderArgument_Target,  // intValue0: CodeGenTarget
    ShaderArgument_Version,
    ShaderArgument_WarningsAsErrors, // stringValue0: "all" or comma separated list of warning codes or names.
    ShaderArgument_DisableWarnings,  // stringValue0: comma separated list of warning codes or names.
    ShaderArgument_EnableWarning,    // stringValue0: warning code or name.
    ShaderArgument_DisableWarning,   // stringValue0: warning code or name.
    ShaderArgument_DumpWarningDiagnostics,
    ShaderArgument_InputFilesRemain,
    ShaderArgument_EmitIr,                        // bool
    ShaderArgument_ReportDownstreamTime,          // bool
    ShaderArgument_ReportPerfBenchmark,           // bool
    ShaderArgument_ReportCheckpointIntermediates, // bool
    ShaderArgument_SkipSPIRVValidation,           // bool
    ShaderArgument_SourceEmbedStyle,
    ShaderArgument_SourceEmbedName,
    ShaderArgument_SourceEmbedLanguage,
    ShaderArgument_DisableShortCircuit,            // bool
    ShaderArgument_MinimumSlangOptimization,       // bool
    ShaderArgument_DisableNonEssentialValidations, // bool
    ShaderArgument_DisableSourceMap,               // bool
    ShaderArgument_UnscopedEnum,                   // bool
    ShaderArgument_PreserveParameters,             // bool: preserve all resource parameters in the output code.
                                                   // Target

    ShaderArgument_Capability,                // intValue0: CapabilityName
    ShaderArgument_DefaultImageFormatUnknown, // bool
    ShaderArgument_DisableDynamicDispatch,    // bool
    ShaderArgument_DisableSpecialization,     // bool
    ShaderArgument_FloatingPointMode,         // intValue0: FloatingPointMode
    ShaderArgument_DebugInformation,          // intValue0: DebugInfoLevel
    ShaderArgument_LineDirectiveMode,
    ShaderArgument_Optimization, // intValue0: OptimizationLevel
    ShaderArgument_Obfuscate,    // bool

    ShaderArgument_VulkanBindShift,         // intValue0 (higher 8 bits): kind; intValue0(lower bits): set; intValue1:
                                            // shift
    ShaderArgument_VulkanBindGlobals,       // intValue0: index; intValue1: set
    ShaderArgument_VulkanInvertY,           // bool
    ShaderArgument_VulkanUseDxPositionW,    // bool
    ShaderArgument_VulkanUseEntryPointName, // bool
    ShaderArgument_VulkanUseGLLayout,       // bool
    ShaderArgument_VulkanEmitReflection,    // bool

    ShaderArgument_GLSLForceScalarLayout,   // bool
    ShaderArgument_EnableEffectAnnotations, // bool

    ShaderArgument_EmitSpirvViaGLSL,     // bool (will be deprecated)
    ShaderArgument_EmitSpirvDirectly,    // bool (will be deprecated)
    ShaderArgument_SPIRVCoreGrammarJSON, // stringValue0: json path
    ShaderArgument_IncompleteLibrary,    // bool, when set, will not issue an error when the linked program has
                                         // unresolved extern function symbols.

    // Downstream

    ShaderArgument_CompilerPath,
    ShaderArgument_DefaultDownstreamCompiler,
    ShaderArgument_DownstreamArgs, // stringValue0: downstream compiler name. stringValue1: argument list, one
                                   // per line.
    ShaderArgument_PassThrough,

    // Repro

    ShaderArgument_DumpRepro,
    ShaderArgument_DumpReproOnError,
    ShaderArgument_ExtractRepro,
    ShaderArgument_LoadRepro,
    ShaderArgument_LoadReproDirectory,
    ShaderArgument_ReproFallbackDirectory,

    // Debugging

    ShaderArgument_DumpAst,
    ShaderArgument_DumpIntermediatePrefix,
    ShaderArgument_DumpIntermediates, // bool
    ShaderArgument_DumpIr,            // bool
    ShaderArgument_DumpIrIds,
    ShaderArgument_PreprocessorOutput,
    ShaderArgument_OutputIncludes,
    ShaderArgument_ReproFileSystem,
    ShaderArgument_REMOVED_SerialIR, // deprecated and removed
    ShaderArgument_SkipCodeGen,      // bool
    ShaderArgument_ValidateIr,       // bool
    ShaderArgument_VerbosePaths,
    ShaderArgument_VerifyDebugSerialIr,
    ShaderArgument_NoCodeGen, // Not used.

    // Experimental

    ShaderArgument_FileSystem,
    ShaderArgument_Heterogeneous,
    ShaderArgument_NoMangle,
    ShaderArgument_NoHLSLBinding,
    ShaderArgument_NoHLSLPackConstantBufferElements,
    ShaderArgument_ValidateUniformity,
    ShaderArgument_AllowGLSL,
    ShaderArgument_EnableExperimentalPasses,
    ShaderArgument_BindlessSpaceIndex, // int

    // Internal

    ShaderArgument_ArchiveType,
    ShaderArgument_CompileCoreModule,
    ShaderArgument_Doc,

    ShaderArgument_IrCompression, //< deprecated

    ShaderArgument_LoadCoreModule,
    ShaderArgument_ReferenceModule,
    ShaderArgument_SaveCoreModule,
    ShaderArgument_SaveCoreModuleBinSource,
    ShaderArgument_TrackLiveness,
    ShaderArgument_LoopInversion, // bool, enable loop inversion optimization

    ShaderArgument_ParameterBlocksUseRegisterSpaces, // Deprecated
    ShaderArgument_LanguageVersion,                  // intValue0: SlangLanguageVersion
    ShaderArgument_TypeConformance, // stringValue0: additional type conformance to link, in the format of
                                    // "<TypeName>:<IInterfaceName>[=<sequentialId>]", for example
                                    // "Impl:IFoo=3" or "Impl:IFoo".
    ShaderArgument_EnableExperimentalDynamicDispatch, // bool, experimental
    ShaderArgument_EmitReflectionJSON,                // bool

    ShaderArgument_CountOfParsableOptions,

    // Used in parsed options only.
    ShaderArgument_DebugInformationFormat,  // intValue0: DebugInfoFormat
    ShaderArgument_VulkanBindShiftAll,      // intValue0: kind; intValue1: shift
    ShaderArgument_GenerateWholeProgram,    // bool
    ShaderArgument_UseUpToDateBinaryModule, // bool, when set, will only load
                                            // precompiled modules if it is up-to-date with its source.
    ShaderArgument_EmbedDownstreamIR,       // bool
    ShaderArgument_ForceDXLayout,           // bool

    // Add this new option to the end of the list to avoid breaking ABI as much as possible.
    // Setting of EmitSpirvDirectly or EmitSpirvViaGLSL will turn into this option internally.
    ShaderArgument_EmitSpirvMethod, // enum SlangEmitSpirvMethod

    ShaderArgument_SaveGLSLModuleBinSource,

    ShaderArgument_SkipDownstreamLinking, // bool, experimental
    ShaderArgument_DumpModule,

    ShaderArgument_GetModuleInfo,              // Print serialized module version and name
    ShaderArgument_GetSupportedModuleVersions, // Print the min and max module versions this compiler supports

    ShaderArgument_EmitSeparateDebug, // bool

    // Floating point denormal handling modes
    ShaderArgument_DenormalModeFp16,
    ShaderArgument_DenormalModeFp32,
    ShaderArgument_DenormalModeFp64,

    // Bitfield options
    ShaderArgument_UseMSVCStyleBitfieldPacking, // bool
    ShaderArgument_ForceCLayout,                // bool
    ShaderArgument_ExperimentalFeature,         // bool, enable experimental features
};

enum ShaderArgumentType : u8
{
    ShaderArgument_Integer,
    ShaderArgument_String,
};

struct ShaderArgumentValue
{
    ShaderArgumentType Type;
    const char *String0;
    const char *String1;
    i32 Value0;
    i32 Value1;
};

struct ShaderMacro
{
    const char *Name;
    const char *Value;
};

struct ShaderArgument
{
    ShaderArgumentValue Value;
    ShaderArgumentName Name;

    static constexpr ShaderArgument Create(const ShaderArgumentName name, const char *val0, const char *val1 = nullptr)
    {
        ShaderArgumentValue val;
        val.Type = ShaderArgument_String;
        val.String0 = val0;
        val.String1 = val1;
        return {val, name};
    }
    static constexpr ShaderArgument Create(const ShaderArgumentName name, const i32 val0, const i32 val1 = 0)
    {
        ShaderArgumentValue val;
        val.Type = ShaderArgument_Integer;
        val.Value0 = val0;
        val.Value1 = val1;
        return {val, name};
    }
    static constexpr ShaderArgument Create(const ShaderArgumentName name)
    {
        ShaderArgumentValue val;
        val.Type = ShaderArgument_Integer;
        val.Value0 = 1;
        return {val, name};
    }
};

struct EntryPoint
{
    const char *Name;
    ShaderStageFlagBit Stage;
};

struct ShaderModule
{
    const char *Name;
    const char *SourceCode;
    const char *Path;
    TKit::Span<const EntryPoint> EntryPoints;

    static constexpr ShaderModule Create(const char *name, const TKit::Span<const EntryPoint> eps)
    {
        return ShaderModule{name, nullptr, nullptr, eps};
    }
    static constexpr ShaderModule Create(const char *name, const char *code, const char *path,
                                         const TKit::Span<const EntryPoint> eps)
    {
        return ShaderModule{name, code, path, eps};
    }
};

struct CompilationSpecs
{
    TKit::Span<const ShaderModule> Modules{};
    TKit::Span<const ShaderArgument> Arguments{};
    TKit::Span<const ShaderMacro> Macros{};
    TKit::Span<const char *const> SearchPaths{};
};

using CompilationFlags = u8;
enum CompilationFlagBit : CompilationFlags
{
    CompilationFlag_EnableEffectAnnotations = 1U << 0,
    CompilationFlag_AllowGlslSyntax = 1U << 1,
    CompilationFlag_SkipSpirvValidation = 1U << 2,
    CompilationFlag_EnableGlsl = 1U << 3,
};

TKit::Result<Compilation, TKit::TierString> Compilation_Create(const CompilationSpecs &specs,
                                                               CompilationFlags flags = 0);
void Compilation_Destroy(Compilation comp);

SpirvData Compilation_GetSpirv(Compilation comp, const char *entryPoint, const char *module, ShaderStageFlagBit stage);
SpirvData Compilation_GetSpirv(Compilation comp, const char *entryPoint, const char *module = nullptr);

inline SpirvData Compilation_GetSpirv(const Compilation comp, const char *entryPoint, const ShaderStageFlagBit stage,
                                      const char *module = nullptr)
{
    return Compilation_GetSpirv(comp, entryPoint, module, stage);
}

inline Shader Compilation_CreateShader(const Compilation comp, const char *entryPoint, const char *module,
                                       const ShaderStageFlagBit stage)
{
    const SpirvData data = Compilation_GetSpirv(comp, entryPoint, module, stage);
    return Shader_Create(data);
}
inline Shader Compilation_CreateShader(const Compilation comp, const char *entryPoint, const char *module = nullptr)
{
    const SpirvData data = Compilation_GetSpirv(comp, entryPoint, module);
    return Shader_Create(data);
}
inline Shader Compilation_CreateShader(const Compilation comp, const char *entryPoint, const ShaderStageFlagBit stage,
                                       const char *module = nullptr)
{
    return Compilation_CreateShader(comp, entryPoint, module, stage);
}

bool Compilation_IsHandleValid(Compilation comp);

#endif
} // namespace Graph
