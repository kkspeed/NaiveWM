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

class ShellSurface;

namespace wm {

class Window : public SurfaceObserver {
 public:
  Window();
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
  void set_visible(bool visible) {
    if (parent_ != nullptr) {
      TRACE("ERROR: shouldn't set visibility on non toplevel surface!");
      return;
    }
    visible_ = visible;
  }

  // SurfaceObserver overrides
  void OnCommit() override;

  void Raise();

  Surface* surface() { return surface_; }
  void set_surface(Surface* surface) {
    LOG_ERROR << "Set Surface " << surface << std::endl;
    surface_ = surface;
    surface_->AddSurfaceObserver(this);
  }

  void SetShellSurface(ShellSurface* shell_surface) {
    shell_surface_ = shell_surface;
  }

  void SetPosition(int32_t x, int32_t y) {
    TRACE("%d, %d", x, y);
    pending_state_.geometry.x_ = x;
    pending_state_.geometry.y_ = y;
  }

  void SetVisibleRegion(const base::geometry::Rect& rect) {
    TRACE("visible region %p: %d %d %d %d", this, rect.x(), rect.y(),
          rect.width(), rect.height());
    pending_state_.visible_region = rect;
  }

  void SetGeometry(const base::geometry::Rect& rect) {
    TRACE("geometry: %p: %d %d %d %d", this, rect.x(), rect.y(), rect.width(),
          rect.height());
    pending_state_.geometry = rect;
  }

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
    if (state_.visible_region.Empty())
      return base::geometry::Rect(0, 0, state_.geometry.width(),
                                  state_.geometry.height());
    return state_.visible_region;
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
  base::geometry::Rect visible_region() { return state_.visible_region; }
  base::geometry::Rect pending_geometry() { return pending_state_.geometry; }
  base::geometry::Rect geometry() { return state_.geometry; }
  int32_t wm_x() { return wm_x_; }
  int32_t wm_y() { return wm_y_; }

 private:
  bool managed_ = false, to_be_managed_ = false;
  bool focused_ = false;
  bool visible_ = true;

  struct WindowState {
    base::geometry::Rect geometry;
    base::geometry::Rect visible_region;
    WindowState() : geometry({0, 0, 0, 0}), visible_region({0, 0, 0, 0}) {}
  };

  bool is_popup_ = false, is_transient_ = false;
  WindowState pending_state_, state_;
  std::string title_, clazz_, app_id_;
  std::vector<Window*> children_;
  Window* parent_ = nullptr;
  Surface* surface_;
  ShellSurface* shell_surface_;
  int32_t wm_x_ = 0, wm_y_ = 0;
};

}  // namespace wm
}  // namespace naive

#endif  // NAIVEWM_WINDOW_H
