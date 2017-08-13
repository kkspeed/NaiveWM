#ifndef WM_WINDOW_H_
#define WM_WINDOW_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/geometry.h"
#include "base/logging.h"
#include "compositor/shell_surface.h"
#include "compositor/surface.h"

namespace naive {
namespace ui {
class Widget;
}  // namespace ui

class ShellSurface;

namespace wm {

class WindowImpl;
enum class WindowType { NORMAL, CONTAINER, WIDGET, ROOT };

class Window {
 public:
  explicit Window(Surface* surface);
  explicit Window(ui::Widget* widget);
  ~Window();

  bool IsManaged() const { return managed_; }
  bool is_visible() const { return visible_; }
  Window* top_level() {
    Window* result = this;
    while (result->parent() != nullptr)
      result = result->parent();
    return result;
  }
  void set_parent(Window* parent) { parent_ = parent; }
  void set_transient(bool transient) { is_transient_ = transient; }
  void set_fullscreen(bool fullscreen);
  void set_maximized(bool maximized);
  void set_title(std::string title) { title_ = title; }
  void set_class(std::string clazz) { clazz_ = clazz; }
  void set_appid(std::string app_id) { app_id_ = app_id; }
  void set_popup(bool popup) { is_popup_ = popup; }
  void set_visible(bool visible);

  void MaybeMakeTopLevel();

  void Raise();

  Surface* surface();

  void SetShellSurface(ShellSurface* shell_surface);

  void PushProperty(const base::geometry::Rect& geometry,
                    const base::geometry::Rect& visible_region);
  void PushProperty(bool is_position, int32_t v0, int32_t v1);

  void Resize(int32_t width, int32_t height);

  void AddChild(Window* child);
  void RemoveChild(Window* child);
  bool HasChild(const Window* child) const;
  void PlaceAbove(Window* window, Window* target);
  void PlaceBelow(Window* window, Window* target);
  void BeginMove() { /* TODO: implement this */
  }

  void GrabDone();

  bool focused() { return focused_; }
  // Sets the window size via window manager.
  // TODO: This needs to commit as well.
  void WmSetSize(int32_t width, int32_t height);
  void WmSetPosition(int32_t x, int32_t y) {
    wm_x_ = x;
    wm_y_ = y;
    surface_->force_commit();
  }
  base::geometry::Rect GetToDrawRegion() {
    if (visible_region_.Empty())
      return base::geometry::Rect(0, 0, geometry_.width(), geometry_.height());
    return visible_region_;
  }
  void LoseFocus();
  void TakeFocus();
  void Close();

  bool is_popup() { return is_popup_; }
  bool is_transient() { return is_transient_; }
  void set_managed(bool managed) { managed_ = managed; }
  void set_to_be_managed(bool tobe) { to_be_managed_ = true; }
  std::vector<Window*>& children() { return children_; }
  Window* parent() { return parent_; }
  base::geometry::Rect visible_region() { return visible_region_; }
  base::geometry::Rect geometry() { return geometry_; }
  int32_t wm_x() { return wm_x_; }
  int32_t wm_y() { return wm_y_; }
  base::geometry::Rect global_bound() {
    auto rect = geometry();
    rect.x_ += wm_x();
    rect.y_ += wm_y();
    auto* p = parent_;
    while (p) {
      // TODO: wm_x / wm_y might have other meaning in the future.
      rect.x_ += p->geometry().x() + p->wm_x();
      rect.y_ += p->geometry().y() + p->wm_y();
      p = p->parent();
    }
    return rect;
  }

  void set_type(WindowType type);
  WindowType window_type() const;
  void NotifyFrameCallback();

  WindowImpl* window_impl() { return window_impl_.get(); }
  void enable_border(bool border) { has_border_ = border; }
  bool has_border() { return has_border_; }

  pid_t GetPid();

  int32_t mouse_event_scale_override() { return mouse_event_scale_override_; }
  void set_mouse_event_scale_override(int32_t override) {
    mouse_event_scale_override_ = override;
  }

 private:
  bool managed_ = false, to_be_managed_ = false;
  bool focused_ = false;
  bool visible_ = true;
  bool has_border_ = false;

  base::geometry::Rect geometry_, visible_region_;

  bool is_popup_ = false, is_transient_ = false;
  std::string title_, clazz_, app_id_;
  std::vector<Window*> children_;
  Window* parent_ = nullptr;
  Surface* surface_;
  ShellSurface* shell_surface_;
  int32_t wm_x_ = 0, wm_y_ = 0;
  WindowType type_;
  int32_t mouse_event_scale_override_{0};

  std::unique_ptr<WindowImpl> window_impl_;
  ui::Widget* widget_;
};

}  // namespace wm
}  // namespace naive

#endif  // NAIVEWM_WINDOW_H
