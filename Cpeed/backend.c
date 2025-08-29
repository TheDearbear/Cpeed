#include "backend.h"

#if CPD_VULKAN_ENABLED
#include "common/backend/vulkan/backend.h"
#endif

bool get_backend_implementation(CpdPlatformBackendFlags backend, CpdBackendImplementation* implementation) {
    switch (backend) {
#if CPD_VULKAN_ENABLED
        case CpdPlatformBackendFlags_Vulkan:
            get_vulkan_backend_implementation(implementation);
            return true;
#endif

        default:
            return false;
    }
}
