#pragma once

#include "vkit/vulkan/vulkan.hpp"

#define GRAPH_CHECK_VKIT_RESULT(expression) Graph::CheckVKitError(expression)

namespace Graph
{
void HandleVulkanResult(const VkResult result);

template <typename T> auto CheckVKitError(TKit::Result<T, VKit::Error> &&result)
{
#ifdef TKIT_ENABLE_ENSURE
    if (!result)
    {
        const auto &error = result.GetError();
        if (error.GetCode() == VKit::Error_VulkanError)
            HandleVulkanResult(error.GetVulkanResult());

        TKIT_PANIC("{}", error.ToString());
    }
#else
    TKIT_ASSERT(result, "{}", result.GetError().ToString());
#endif

    if constexpr (!std::same_as<T, void>)
        return *result;
}

inline void CheckVKitError(const VkResult result)
{
#ifdef TKIT_ENABLE_ENSURE
    HandleVulkanResult(result);
#endif
    VKIT_CHECK_RESULT(result);
}
} // namespace Graph
