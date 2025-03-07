#pragma once

#define _CHECK_RESULT_IMPL(func, var_suffix) \
	{ \
		vk::Result local_result_##var_suffix = (func); \
		if (local_result_##var_suffix != vk::Result::eSuccess) { \
			LOG_FATAL("Vulkan Error: %s in %s", vk::to_string(local_result_##var_suffix).c_str(), #func); \
		} \
	}

// For __LINE__ macro expansion
#define _FORWARD_CHECK_RESULT(x, y) _CHECK_RESULT_IMPL(x, y)

#define CHECK_RESULT(func) _FORWARD_CHECK_RESULT(func, __LINE__)
