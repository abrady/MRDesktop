#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <cstdio>

int main(int argc, char** argv) {
    XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
    snprintf(createInfo.applicationInfo.applicationName, XR_MAX_APPLICATION_NAME_SIZE, "MRDesktopOpenXR");
    createInfo.applicationInfo.applicationVersion = 1;
    snprintf(createInfo.applicationInfo.engineName, XR_MAX_ENGINE_NAME_SIZE, "MRDesktop");
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    XrInstance instance{XR_NULL_HANDLE};
    XrResult result = xrCreateInstance(&createInfo, &instance);
    if (XR_FAILED(result)) {
        printf("Failed to create OpenXR instance: %d\n", result);
        return -1;
    }

    XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId systemId;
    result = xrGetSystem(instance, &systemInfo, &systemId);
    if (XR_FAILED(result)) {
        printf("Failed to get OpenXR system: %d\n", result);
        xrDestroyInstance(instance);
        return -1;
    }

    printf("OpenXR initialized, system id %llu\n", (unsigned long long)systemId);

    xrDestroyInstance(instance);
    return 0;
}
