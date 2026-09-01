#pragma once

#include <Cpeed/common/math.h>

#include "../renderer.h"

VkResult RENDERING_initialize(CpdRenderer* cpeed_renderer);

VkResult RENDERING_resize(CpdRenderer* cpeed_renderer, CpdWindowResizeFlags resize_flags, CpdSize new_size, float new_scale);

VkResult RENDERING_frame(CpdRenderer* cpeed_renderer);

void RENDERING_shutdown(CpdRenderer* cpeed_renderer);
