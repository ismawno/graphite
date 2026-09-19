#pragma once

#include "graph/handle.hpp"
#include "graph/core.hpp"

namespace Graph
{
using Surface = Handle;

Surface Surface_Create(Window win);
void Surface_Destroy(Surface surf);

bool Surface_IsHandleValid(Surface surf);
} // namespace Graph
