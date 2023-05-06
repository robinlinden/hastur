#include "vulkan_canvas.h"

#include "vulkan/vulkan.h"
#include <GL/glew.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
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

namespace {

tl::expected<std::tuple<VkApplicationInfo, VkInstance>, VulkanError> build_vk_instance(
        std::string_view app_name, std::vector<std::string_view> const &) {
    VkApplicationInfo app_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = app_name.data(),
            .applicationVersion = 1,
            .pEngineName = app_name.data(),
            .engineVersion = 1,
            .apiVersion = VK_API_VERSION_1_0,
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

bool device_supports_swapchain(const VkPhysicalDevice &device) {
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    if (extension_count == 0) {
        return false;
    }

    std::vector<VkExtensionProperties> device_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, device_extensions.data());

    for (const auto &extension : device_extensions) {
        if (std::strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            return true;
        }
    }

    return false;
}

void get_available_validation_layers(std::vector<VkLayerProperties> &validation_layers_out) {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> avai(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, validation_layers_out.data());
}

std::optional<VkPhysicalDevice> get_suitable_device(VkInstance &instance) {
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

        if (device_supports_swapchain(device)) return device;
    }

    return std::nullopt;
}

} // namespace

VulkanCanvasBuilder::VulkanCanvasBuilder() : validation_layers_{} {}

VulkanCanvasBuilder &VulkanCanvasBuilder::validation_layer(const std::string_view validation_layer) {
    validation_layers_.push_back(validation_layer);
    return *this;
}

VulkanCanvasBuilder &VulkanCanvasBuilder::validation_layers(std::vector<std::string_view> const &validation_layers) {
    validation_layers_.insert(validation_layers_.end(), validation_layers.begin(), validation_layers.end());
    return *this;
}

tl::expected<void, VulkanError> VulkanCanvasBuilder::check_validation_layers() {
    std::vector<VkLayerProperties> available_layers = {};
    get_available_validation_layers(available_layers);

    for (std::string_view layer : validation_layers_) {
        auto const layer_loc = std::find_if(
                available_layers.begin(), available_layers.end(), [&layer](VkLayerProperties const &properties) {
                    return !std::strcmp(properties.layerName, layer.data());
                });

        if (layer_loc == available_layers.end()) {
            return tl::make_unexpected(VulkanError::InvalidValidationLayer);
        }
    }

    return {};
}

tl::expected<VulkanCanvas, VulkanError> VulkanCanvasBuilder::build(std::string_view app_name) {
    if (auto res = check_validation_layers(); !res) {
        return tl::make_unexpected(res.error());
    }

    // ugly
    auto res = build_vk_instance(app_name, validation_layers_);
    if (!res) {
        return tl::make_unexpected(res.error());
    }
    auto [app_info, instance] = res.value();

    VulkanCanvas canvas(app_info, instance);

    auto device = get_suitable_device(instance);
    if (!device) {
        return tl::make_unexpected(VulkanError::NoSuitableDevice);
    }

    return canvas;
}

VulkanCanvas::VulkanCanvas(VkApplicationInfo app_info, VkInstance instance)
    : scale_(0), tx_(0), ty_(0), app_info_(app_info), instance_(instance) {}

void VulkanCanvas::set_viewport_size(int, int) {}

void VulkanCanvas::draw_rect(geom::Rect const &, Color const &, Borders const &, Corners const &) {}

void VulkanCanvas::fill_rect(geom::Rect const &, Color) {
    // instance_;
}
void VulkanCanvas::draw_text(geom::Position, std::string_view, std::span<Font const>, FontSize, FontStyle, Color) {}
void VulkanCanvas::draw_text(geom::Position, std::string_view, Font, FontSize, FontStyle, Color) {}

} // namespace gfx
