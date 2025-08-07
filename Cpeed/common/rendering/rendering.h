#pragma once

#include "../../vulkan.h"
#include "../general.h"
#include "../renderer.h"

VkResult RENDERING_initialize(CpdRenderer* cpeed_renderer);

VkResult RENDERING_resize(CpdRenderer* cpeed_renderer, CpdSize new_size);

void RENDERING_input(CpdRenderer* cpeed_renderer, CpdWindow window, const CpdInputEvent* events, uint32_t event_count);

VkResult RENDERING_frame(CpdRenderer* cpeed_renderer, bool wait_for_next_image);

void RENDERING_shutdown(CpdRenderer* cpeed_renderer);
