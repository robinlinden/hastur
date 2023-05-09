#ifndef VULKAN_CANVAS_H_
#define VULKAN_CANVAS_H_

#include "gfx/icanvas.h"
#include "vulkan/vulkan.h"
#include <memory>
#include <optional>
#include <string_view>
#include <tl/expected.hpp>
#include <vector>

namespace gfx {

enum class VulkanError {
    InvalidValidationLayer,
    NoSuitableDevice,
    CreateInstanceFailed,
};

struct VulkanCanvasOptions {
    int scale;
};

struct Queues {
    VkQueue graphics_queue;
    VkQueue present_queue;
};

class VulkanDevice {
public:
    struct Options {
        VkDevice device;
        VkQueue graphics_queue;
        VkQueue present_queue;
    };

    explicit VulkanDevice(Options options);
    static tl::expected<VulkanDevice, VulkanError> create(VkInstance instance);

private:
    VkDevice device_;
    VkQueue graphics_queue_;
    VkQueue present_queue_;

    void emplace_graphics();
    void emplace_presentation();
};

class VulkanCanvas : public ICanvas {
public:
    VulkanCanvas(int scale, VulkanDevice device, VkApplicationInfo app_info, VkInstance instance);
    static tl::expected<VulkanCanvas, VulkanError> create(std::string_view app_name, VulkanCanvasOptions options);

    void set_viewport_size(int width, int height) override;
    void set_scale(int scale) override { scale_ = scale; }

    void add_translation(int dx, int dy) override {
        tx_ += dx;
        ty_ += dy;
    }

    void fill_rect(geom::Rect const &, Color) override;
    void draw_rect(geom::Rect const &, Color const &, Borders const &, Corners const &) override;
    void draw_text(geom::Position, std::string_view, std::span<Font const>, FontSize, FontStyle, Color) override;
    void draw_text(geom::Position, std::string_view, Font, FontSize, FontStyle, Color) override;

private:
    int scale_;
    int tx_;
    int ty_;
    VulkanDevice device_;
    VkApplicationInfo app_info_;
    VkInstance instance_;

    friend class VulkanCanvasBuilder;
};

} // namespace gfx

#endif // VULKAN_CANVAS_H_
