#include "vulkan_canvas.h"

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

#include <vulkan/vulkan.hpp>
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
#include <vector>

#define try_unwrap(...) ({ auto m_ = (__VA_ARGS__);
#define or_return                                                                                                      \
    if (!m_) {                                                                                                         \
        return tl::make_unexpected(m_.error());                                                                        \
    }                                                                                                                  \
    m_.value();                                                                                                        \
    })

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

namespace gfx {

struct QueueIndices {
    uint32_t graphics_index;
    uint32_t present_index;
};

namespace {

const std::vector<std::string> validation_layers = {
        "VK_LAYER_GOOGLE_threading",
        "VK_LAYER_LUNARG_parameter_validation",
        "VK_LAYER_LUNARG_device_limits",
        "VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_image",
        "VK_LAYER_LUNARG_core_validation",
        "VK_LAYER_LUNARG_swapchain",
        "VK_LAYER_GOOGLE_unique_objects",
};

bool check_validation_layers(std::vector<std::string> const &layers) {
    std::vector<vk::LayerProperties> available_layers = vk::enumerateInstanceLayerProperties();

    for (std::string_view layer : layers) {
        auto const layer_loc = std::find_if(
                available_layers.begin(), available_layers.end(), [&layer](VkLayerProperties const &properties) {
                    printf("%s\n", properties.layerName);
                    return !std::strcmp(properties.layerName, layer.data());
                });

        if (layer_loc == available_layers.end()) {
            printf("ERROR: %s is not supported.\n", layer.data());
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

    VkInstance instance = vk::createInstance(inst_info, nullptr);

    return std::tuple{
            app_info,
            instance,
    };
}

bool device_supports_swapchain(vk::PhysicalDevice device) {
    std::vector<vk::ExtensionProperties> device_extensions = device.enumerateDeviceExtensionProperties();
    if (device_extensions.empty()) {
        return false;
    }

    for (auto const &extension : device_extensions) {
        if (std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            return true;
        }
    }

    return false;
}

std::optional<QueueIndices> find_device_queue_families(vk::PhysicalDevice device) {
    auto queue_families = device.getQueueFamilyProperties();

    std::optional<uint32_t> graphics_index = {};
    std::optional<uint32_t> present_index = 0;

    uint32_t index = 0;
    for (auto const &queue_family : queue_families) {
        if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
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

std::optional<vk::Device> build_logical_device(vk::PhysicalDevice device) {
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

    return device.createDevice(create_info, nullptr);
}

std::optional<VkPhysicalDevice> get_suitable_device(vk::Instance instance) {
    std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
    if (devices.empty()) {
        return std::nullopt;
    }

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

    VkQueue graphics_queue = vk_device->getQueue(indices->graphics_index, 0);
    VkQueue present_queue = vk_device->getQueue(indices->present_index, 0);

    return VulkanDevice({
            .device = vk_device.value(),
            .graphics_queue = graphics_queue,
            .present_queue = present_queue,
    });
}

VulkanCanvas::VulkanCanvas(int scale, VulkanDevice device, VkApplicationInfo app_info, VkInstance instance)
    : scale_(scale), device_(device), app_info_(app_info), instance_(instance) {}

tl::expected<VulkanCanvas, VulkanError> VulkanCanvas::create(std::string_view app_name, VulkanCanvasOptions options) {
    vk::DynamicLoader dl;
    auto *vk_get_instance_proc_addr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vk_get_instance_proc_addr);

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

    VULKAN_HPP_DEFAULT_DISPATCHER.init( instance );

    auto device = VulkanDevice::create(instance);
    if (!device) {
        return tl::make_unexpected(device.error());
    }

    // NOTE: device, app_info and instance are pointers.
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
