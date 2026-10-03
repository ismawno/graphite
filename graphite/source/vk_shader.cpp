#include "pch.hpp"
#include "graph/shader.hpp"
#include "vk_core.hpp"
#include "vk_error.hpp"
#include "vkit/state/shader.hpp"
#include "tkit/utils/storage.hpp"
#include "tkit/container/hive.hpp"

namespace Graph
{
static TKit::Storage<TKit::ArenaHive<VKit::Shader>> s_Shaders{};

void Shader_Initialize(const u32 maxShaders)
{
    s_Shaders.Construct();
    s_Shaders->Reserve(maxShaders);
}

void Shader_Terminate()
{
    GRAPH_CLEANUP_WITH_WARNING(s_Shaders, "SHADERS", "shaders");
    s_Shaders.Destruct();
}

Shader Shader_Create(const SpirvData &data)
{
    const VKit::Shader sh = GRAPH_CHECK_VKIT_RESULT(VKit::Shader::Create(GetDevice(), data.Code, data.Size));
    return Handle_Create(Handle_Shader, s_Shaders->Insert(sh));
}
Shader Shader_Create(const TKit::StringView path)
{
    const VKit::Shader sh = GRAPH_CHECK_VKIT_RESULT(VKit::Shader::Create(GetDevice(), path));
    return Handle_Create(Handle_Shader, s_Shaders->Insert(sh));
}

void Shader_Destroy(const Shader sh)
{
    GRAPH_CHECK_HANDLE(sh, Handle_Shader);
    GRAPH_DESTROY_FUNCTION_BODY(s_Shaders, sh);
}

void Shader_SetName(const Shader sh, const char *name)
{
    GRAPH_CHECK_HANDLE(sh, Handle_Shader);
    TKIT_ASSERT(IsValidationEnabled(), "[GRAPH][SHADERS] To name objects, the validation capability must be enabled");
    GRAPH_CHECK_VKIT_RESULT(s_Shaders->At(Handle_GetId(sh)).SetName(name));
}
bool Shader_IsHandleValid(const Shader sh)
{
    GRAPH_IS_HANDLE_VALID_FUNCTION_BODY(s_Shaders, sh, Handle_Shader);
}

VKit::Shader &GetShader(const Shader sh)
{
    GRAPH_CHECK_HANDLE(sh, Handle_Shader);
    return s_Shaders->At(Handle_GetId(sh));
}
} // namespace Graph
