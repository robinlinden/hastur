#ifndef VULKAN_CANVAS_H_
#define VULKAN_CANVAS_H_

#include <list>

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

class VulkanCanvas;

class VulkanCanvasBuilder {
public:
    explicit VulkanCanvasBuilder();

    VulkanCanvasBuilder &validation_layer(std::string_view);
    VulkanCanvasBuilder &validation_layers(std::vector<std::string_view> const &);

    tl::expected<VulkanCanvas, VulkanError> build(std::string_view);

private:
    std::vector<std::string_view> validation_layers_;

    tl::expected<void, VulkanError> check_validation_layers();
};

class VulkanCanvas : public ICanvas {
public:
    explicit VulkanCanvas(VkApplicationInfo, VkInstance);

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
    VkApplicationInfo app_info_;
    VkInstance instance_;

    friend class VulkanCanvasBuilder;
};

} // namespace gfx

#endif // VULKAN_CANVAS_H_
