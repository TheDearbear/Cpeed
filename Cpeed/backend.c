#include "backend.h"

#if CPD_DIRECTX_ENABLED
#include <CpeedDirectX/backend.h>
#endif

#if CPD_VULKAN_ENABLED
#include <CpeedVulkan/backend.h>
#endif

#if CPD_RSX_ENABLED
#include <CpeedRSX/backend.h>
#endif

bool get_backend_implementation(CpdPlatformBackendFlags backend, CpdBackendImplementation* implementation) {
    switch (backend) {
#if CPD_DIRECTX_ENABLED
        case CpdPlatformBackendFlags_DirectX:
            get_directx_backend_implementation(implementation);
            return true;
#endif

#if CPD_VULKAN_ENABLED
        case CpdPlatformBackendFlags_Vulkan:
            get_vulkan_backend_implementation(implementation);
            return true;
#endif

#if CPD_RSX_ENABLED
        case CpdPlatformBackendFlags_RSX:
            get_rsx_backend_implementation(implementation);
            return true;
#endif

        default:
            return false;
    }
}
