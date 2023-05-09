#include "vulkan_canvas.h"

#include "vulkan/vulkan.h"
#include <GL/glew.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <tuple>
#include <unistd.h>
#include <vector>

#define try_unwrap(...) ({ auto m_ = (__VA_ARGS__);
#define or_return                                                                                                      \
    if (!m_) {                                                                                                         \
        return tl::make_unexpected(m_.error());                                                                        \
    }                                                                                                                  \
    m_.value();                                                                                                        \
    })

namespace gfx {

struct QueueIndices {
    uint32_t graphics_index;
    uint32_t present_index;
};

namespace {

const std::vector<std::string> validation_layers = {
        // "HELLO",
};

void get_available_validation_layers(std::vector<VkLayerProperties> &validation_layers_out) {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    vkEnumerateInstanceLayerProperties(&layer_count, validation_layers_out.data());
}

bool check_validation_layers(std::vector<std::string> const &layers) {
    std::vector<VkLayerProperties> available_layers = {};
    get_available_validation_layers(available_layers);

    for (std::string_view layer : layers) {
        auto const layer_loc = std::find_if(
                available_layers.begin(), available_layers.end(), [&layer](VkLayerProperties const &properties) {
                    return !std::strcmp(properties.layerName, layer.data());
                });

        if (layer_loc == available_layers.end()) {
            return false;
        }
    }

    return true;
}

tl::expected<std::tuple<VkApplicationInfo, VkInstance>, VulkanError> build_instance(
        std::string_view app_name, std::vector<std::string> const &) {
    VkApplicationInfo app_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = app_name.data(),
            .applicationVersion = 1,
            .pEngineName = app_name.data(),
            .engineVersion = 1,
            .apiVersion = VK_API_VERSION_1_1,
    };

    VkInstanceCreateInfo inst_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &app_info,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = 0,
            .ppEnabledExtensionNames = nullptr,
    };

    VkInstance instance;

    if (auto res = vkCreateInstance(&inst_info, nullptr, &instance); res != VK_SUCCESS) {
        return tl::unexpected{VulkanError::CreateInstanceFailed};
    }

    return std::tuple{
            app_info,
            instance,
    };
}

bool device_supports_swapchain(VkPhysicalDevice device) {
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    if (extension_count == 0) {
        return false;
    }

    std::vector<VkExtensionProperties> device_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, device_extensions.data());

    for (auto const &extension : device_extensions) {
        if (std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            return true;
        }
    }

    return false;
}

std::optional<QueueIndices> find_device_queue_families(VkPhysicalDevice device) {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    std::optional<uint32_t> graphics_index = {};
    std::optional<uint32_t> present_index = 0;

    uint32_t index = 0;
    for (auto const &queue_family : queue_families) {
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_index = index;
        }

        // VkBool32 presentSupport = false;
        // vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        ++index;
    }

    if (!graphics_index.has_value() || !present_index.has_value()) {
        return std::nullopt;
    }

    return QueueIndices{
            .graphics_index = graphics_index.value(),
            .present_index = present_index.value(),
    };
}

bool is_device_suitable(VkPhysicalDevice device) {
    if (!device_supports_swapchain(device)) {
        return false;
    }

    if (!find_device_queue_families(device)) {
        return false;
    }

    return true;
}

std::optional<VkDevice> build_logical_device(VkPhysicalDevice device) {
    float queue_priority = 1.0f;

    auto indices = find_device_queue_families(device);
    if (!indices) {
        return std::nullopt;
    }

    VkDeviceQueueCreateInfo queue_create_info{
            .queueFamilyIndex = indices->graphics_index,
            .pQueuePriorities = &queue_priority,
    };

    VkPhysicalDeviceFeatures device_features{};

    VkDeviceCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queue_create_info,
            .enabledExtensionCount = 0,
            .pEnabledFeatures = &device_features,
    };

    VkDevice device_out;

    if (vkCreateDevice(device, &create_info, nullptr, &device_out) == VK_SUCCESS) {
        return device_out;
    }

    return std::nullopt;
}

std::optional<VkPhysicalDevice> get_suitable_device(VkInstance instance) {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    if (device_count == 0) {
        return std::nullopt;
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    for (auto const &device : devices) {
        if (device == VK_NULL_HANDLE) {
            continue;
        }

        if (is_device_suitable(device)) {
            return device;
        }
    }

    return std::nullopt;
}

} // namespace

VulkanDevice::VulkanDevice(VulkanDevice::Options options)
    : device_(options.device), graphics_queue_(options.graphics_queue), present_queue_(options.present_queue) {}

tl::expected<VulkanDevice, VulkanError> VulkanDevice::create(VkInstance instance) {
    auto physical_device = get_suitable_device(instance);
    if (!physical_device) {
        return tl::make_unexpected(VulkanError::NoSuitableDevice);
    }

    auto vk_device = build_logical_device(physical_device.value());
    if (!vk_device) {
        return tl::make_unexpected(VulkanError::NoSuitableDevice);
    }

    auto indices = find_device_queue_families(physical_device.value());
    if (!indices) {
        return tl::make_unexpected(VulkanError::NoSuitableDevice);
    }

    VkQueue graphics_queue;
    VkQueue present_queue;

    vkGetDeviceQueue(vk_device.value(), indices->graphics_index, 0, &graphics_queue);
    vkGetDeviceQueue(vk_device.value(), indices->present_index, 0, &present_queue);

    return VulkanDevice({
            .device = vk_device.value(),
            .graphics_queue = graphics_queue,
            .present_queue = present_queue,
    });
}

VulkanCanvas::VulkanCanvas(int scale, VulkanDevice device, VkApplicationInfo app_info, VkInstance instance)
    : scale_(scale), tx_(0), ty_(0), device_(device), app_info_(app_info), instance_(instance) {}

tl::expected<VulkanCanvas, VulkanError> VulkanCanvas::create(std::string_view app_name, VulkanCanvasOptions options) {
    if (!check_validation_layers(validation_layers)) {
        return tl::make_unexpected(VulkanError::InvalidValidationLayer);
    }

    // NOTE(ghostway): auto [app_info, instance] = try_unwrap(build_instance(app_name, validation_layers)) or_return;
    // ingtriguing. isn't it?
    auto instance_res = build_instance(app_name, validation_layers);
    if (!instance_res) {
        return tl::make_unexpected(instance_res.error());
    }

    auto [app_info, instance] = instance_res.value();

    auto device = VulkanDevice::create(instance);
    if (!device) {
        return tl::make_unexpected(device.error());
    }

    return VulkanCanvas{
            options.scale,
            device.value(),
            app_info,
            instance,
    };
}

void VulkanCanvas::set_viewport_size(int width, int height) {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    // vkCmdSetViewport(command_buffer_, 0, 1, &viewport);
}

void VulkanCanvas::draw_rect(geom::Rect const &, Color const &, Borders const &, Corners const &) {}

void VulkanCanvas::fill_rect(geom::Rect const &, Color) {}
void VulkanCanvas::draw_text(geom::Position, std::string_view, std::span<Font const>, FontSize, FontStyle, Color) {}
void VulkanCanvas::draw_text(geom::Position, std::string_view, Font, FontSize, FontStyle, Color) {}

} // namespace gfx
