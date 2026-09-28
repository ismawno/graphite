#include "pch.hpp"
#include "graph/shader.hpp"
#include "core.hpp"
#include "tkit/container/stack_array.hpp"
#include "tkit/utils/storage.hpp"
#include "tkit/container/hive.hpp"

namespace Graph
{
struct Spirv_Reflection
{
};

static TKit::Storage<TKit::ArenaHive<Spirv_Reflection>> s_Compilations{};

void Reflection_Initialize(const u32 maxReflections)
{
}

void Reflection_Terminate()
{
}
} // namespace Graph
